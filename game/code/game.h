#if !defined(GAME_H)

/*
 TODO

 - Flush all thread queues before loading DLL.

 - Debug code
   - Fonts
   - Logging
   - Diagramming
   - A little GUI, switches / sliders / etc.
   - Draw tile chunks so we can verify that things are aligned /
     in the chunks we want them to be in

 - Audio
   - Fix the clicking bug at the end of samples.

 - Rendering
   - Get rid of "even" scan line notion.
   - Real projections with solid concept of project/unproject
   - Straighten out all of the coordinate systems.
     - Screen
     - World
     - Texture
   - Particle systems
   - Lighting
   - Final Optimisation

 ARCHITECTURE EXPLORATION
 - Z
   - Need to make a solid concept of ground levels so the camera can
     be freely placed in Z and have multiple ground levels in one sim
     region.
   - Concept of ground in the collision loop so it can handle
     collisions coming onto _and off of_ stairwells, for example.
   - Make sure flying things can go over low walls
   - Frinstances
     ZFUDGE
 - Collision detection
   - Fix sword collisions
   - Clean up predicate proliferation. Can we make a nice clean
     set of flags/rules so that it's easy to understand how
     things work in terms of special handling? This may involve
     making the iteration handle everything instead of handling
     overlap outside and so on.
   - Transient collision rules, clear based on flag
     - Allow non-stransient rules to overide transient ones
   - Entry / exit?
   - What's the plan for robustness / shape definition?
   - Implement reprojection to handle interpenetration
   - Things pushing other things
 - Animtaion
   - Skeletal animation
   - Particle systems
 - Implement multiple sim regions per fram
   - Per-entity clocking
   - Sim region merging? For multiple players?
   - Simple zoomed-out view for testing
 - AI
   - Rudimentary monster example
   * Pathfinding
   - AI 'storage'

 PRODUCTION
 -> GAME
   - Entity system
   - Rudimentary world gen (no quality, just 'what sorts of things' we do)
     - Placement of background things
     - Connectivity?
     - Non-overlapping?
     - Map display
       - Magnets - how do they work?
 - Metagame / save game
   - How do you enter a 'save slot'?
   - Persistent unlocks/etc
   - Do we allow saved games? Probably yes, just for 'pausing',
   * Continuous save for crash recovery?
*/

#include "game_platform.h"
#include "game_intrinsics.h"
#include "game_math.h"
#include "game_file_formats.h"
#include "game_meta.h"

#define DLIST_INSERT(Sentinel, Element)     \
    (Element)->Next = (Sentinel)->Next;     \
    (Element)->Prev = (Sentinel);           \
    (Element)->Next->Prev = (Element);      \
    (Element)->Prev->Next = (Element);

#define DLIST_INIT(Sentinel)       \
    (Sentinel)->Next = (Sentinel); \
    (Sentinel)->Prev = (Sentinel);

#define FREELIST_ALLOCATE(Result, FreeListPointer, AllocationCode) \
    (Result) = (FreeListPointer); \
    if(Result) {FreeListPointer = (Result)->NextFree;} else {Result = AllocationCode;}
#define FREELIST_DEALLOCATE(Pointer, FreeListPointer) \
    if(Pointer) {(Pointer)->NextFree = (FreeListPointer); (FreeListPointer) = (Pointer);}

struct memory_arena
{
    memory_index Size;
    uint8_t *Base;
    memory_index Used;

    int32_t TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

inline b32 
StringsAreEqual(char *A, char *B)
{
    b32 Result = (A == B);

    if(A && B)
    {
        while(*A && *B && (*A == *B))
        {
            ++A;
            ++B;
        }
        
        b32 Result = ((*A == 0) && (*B == 0));
    }
    return Result;
}

inline b32
StringsAreEqual(memory_index ALength, char *A, memory_index BLength, char *B)
{
    b32 Result = (ALength == BLength);

    if(Result)
    {
        Result = true;
        for(u32 Index = 0; Index < ALength; ++Index)
        {
            if(A[Index] != B[Index])
            {
                Result = false;
                break;
            }
        }
    }

    return(Result);
}

//
//
//

inline void
InitialiseArena(memory_arena *Arena, memory_index Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (uint8_t *)Base;
    Arena->Used = 0;
    Arena->TempCount = 0;
}

inline memory_index
GetAlignmentOffset(memory_arena *Arena, memory_index Alignment)
{
    memory_index AlignmentOffset = 0;

    memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
    memory_index AlignmentMask = Alignment - 1;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
    }

    return(AlignmentOffset);
}

inline memory_index
GetArenaSizeRemaining(memory_arena *Arena, memory_index Alignment = 4)
{
    memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment));

    return(Result);
}

// TODO Optional "clear" parameter.
#define DEFAULT_MEMORY_ALIGNMENT 4
#define PushStruct(Arena, type, ...) (type *)PushSize_(Arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(Arena, Count, type, ...) (type *)PushSize_(Arena, (Count)*sizeof(type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)
#define PushCopy(Arena, Size, Source, ...) Copy(Size, Source, PushSize_(Arena, Size, ## __VA_ARGS__))
inline memory_index
GetEffectiveSizeFor(memory_arena *Arena, memory_index SizeInit, memory_index Alignment)
{
    memory_index Size = SizeInit;

    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Size += AlignmentOffset;

    return(Size);
}

inline b32
ArenaHasRoomFor(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = DEFAULT_MEMORY_ALIGNMENT)
{
    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Alignment);
    b32 Result = ((Arena->Used + Size) <= Arena->Size);
    return(Result);
}

inline void *
PushSize_(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = DEFAULT_MEMORY_ALIGNMENT)
{
    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Alignment);

    Assert((Arena->Used + Size) <= Arena->Size);

    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;

    Assert(Size >= SizeInit);

    return(Result);
}

// This isn't for production use, it's probably only
// really something I'll need during testing, but you never know.
inline char *
PushString(memory_arena *Arena, char *Source)
{
    u32 Size = 1;
    for(char *At = Source; *At; ++At)
    {
        ++Size;
    }

    char *Dest = (char *)PushSize_(Arena, Size);
    for(u32 CharIndex = 0; CharIndex < Size; ++CharIndex)
    {
        Dest[CharIndex] = Source[CharIndex];
    }

    return(Dest);
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;

    Result.Arena = Arena;
    Result.Used = Arena->Used;

    ++Arena->TempCount;

    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    Assert(Arena->Used >= TempMem.Used);
    Arena->Used = TempMem.Used;
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

inline void
SubArena(memory_arena *Result, memory_arena *Arena, memory_index Size, memory_index Alignment = 16)
{
    Result->Size = Size;
    Result->Base = (uint8_t *)PushSize_(Arena, Size, Alignment);
    Result->Used = 0;
    Result->TempCount = 0;
}

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    uint8_t *Byte = (uint8_t *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

inline void *
Copy(memory_index Size, void *SourceInit, void *DestInit)
{
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    while(Size--) {*Dest++ = *Source++;}

    return(DestInit);
}

#include "game_world.h"
#include "game_sim_region.h"
#include "game_entity.h"
#include "game_render_group.h"
#include "game_asset.h"
#include "game_random.h"
#include "game_audio.h"

struct low_entity
{
    world_position P;
    sim_entity Sim;
};

struct controlled_hero
{
    uint32_t EntityIndex;

    v2 ddP;
    v2 dSword;
    real32 dZ;
};

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32_t StorageIndexA;
    uint32_t StorageIndexB;

    pairwise_collision_rule *NextInHash;
};
struct game_state;
internal void AddCollisionRule(game_state *GameState, uint32_t StorageIndexA, uint32_t StorageIndexB, bool32 ShouldCollide);
internal void ClearCollisionRulesFor(game_state *GameState, uint32_t StorageIndex); 

struct ground_buffer
{
    world_position P;
    loaded_bitmap Bitmap;
};

struct hero_bitmap_ids
{
    bitmap_id Head;
    bitmap_id Cape;
    bitmap_id Torso;
};

struct particle_cel
{
    r32 Density;
    v3 VelocityTimesDensity;
};
struct particle
{
    bitmap_id BitmapID;
    v3 P;
    v3 dP;
    v3 ddP;
    v4 Color;
    v4 dColor;
};

struct game_state
{
    bool32 IsInitialised;
    
    memory_arena MetaArena;
    memory_arena WorldArena;
    world *World;

    real32 TypicalFloorHeight;

    // TODO: Allow split-screen?
    uint32_t CameraFollowingEntityIndex;
    world_position CameraP;

    controlled_hero ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];

    uint32_t LowEntityCount;
    low_entity LowEntities[100000];

    // Must be a power of two.
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    sim_entity_collision_volume_group *NullCollision;
    sim_entity_collision_volume_group *SwordCollision;
    sim_entity_collision_volume_group *StairCollision;
    sim_entity_collision_volume_group *PlayerCollision;
    sim_entity_collision_volume_group *MonsterCollision;
    sim_entity_collision_volume_group *FamiliarCollision;
    sim_entity_collision_volume_group *WallCollision;
    sim_entity_collision_volume_group *StandardRoomCollision;

    real32 Time;

    loaded_bitmap TestDiffuse; // TODO: Re-fill this bad boy with grey.
    loaded_bitmap TestNormal;

    random_series EffectsEntropy; // This is entropy that doesn't affect the gameplay.
    real32 tSine;

    audio_state AudioState;
    playing_sound *Music;

#define PARTICLE_CEL_DIM 32
    u32 NextParticle;
    particle Particles[256];
    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];
};

struct task_with_memory
{
    bool32 BeingUsed;
    memory_arena Arena;

    temporary_memory MemoryFlush;
};

struct transient_state
{
    bool32 IsInitialised;
    memory_arena TranArena;

    task_with_memory Tasks[4];

    game_assets *Assets;

    uint32_t GroundBufferCount;
    ground_buffer *GroundBuffers;
    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;
    uint64_t Pad;

    uint32_t EnvMapWidth;
    uint32_t EnvMapHeight;
    // 0 is bottom, 1 is middle and 2 is top.
    environment_map EnvMaps[3];
};

inline low_entity *
GetLowEntity(game_state *GameState, uint32_t Index)
{
    low_entity *Result = 0;

    if((Index > 0) && (Index < GameState->LowEntityCount))
    {
        Result = GameState->LowEntities + Index;
    }

    return(Result);
}

global_variable platform_api Platform;

internal task_with_memory *BeginTaskWithMemory(transient_state *TranState);
internal void EndTaskWithMemory(task_with_memory *Task);

#define GAME_H
#endif
