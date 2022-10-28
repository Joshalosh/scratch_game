#if !defined(GAME_H)

/*
 TODO

 - Audio
   - Sound effect triggers
   - Ambient sounds
   - Music

 - Asset streaming
   - Fileformat
   - Memory management

 - Rendering
   - Straighten out all of the coordinate systems.
     - Screen
     - World
     - Texture
   - Particle systems
   - Lighting
   - Final Optimisation

 - Debug code
   - Logging
   - Diagramming
   - A little GUI, switches / sliders / etc.
   - Draw tile chunks so we can verify that things are aligned /
     in the chunks we want them to be in

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
 - Implement multiple sim regions per fram
   - Per-entity clocking
   - Sim region merging? For multiple players?
   - Simple zoomed-out view for testing

 - Metagame / save game
   - How do you enter a 'save slot'?
   - Persistent unlocks/etc
   - Do we allow saved games? Probably yes, just for 'pausing',
   * Continuous save for crash recovery?
 - Rudimentary world gen (no quality, just 'what sorts of things' we do)
   - Placement of background things
   - Connectivity?
   - Non-overlapping?
   - Map display
     - Magnets - how do they work?
 - AI
   - Rudimentary monster example
   * Pathfinding
   - AI 'storage'

 * Animtaion, should probably lead into rendering
   - Skeletal animation
   - Particle systems

 PRODUCTION
 -> GAME
   - Entity system
   - World generation
*/

#include "game_platform.h"

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

//
//
//

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
#define PushStruct(Arena, type, ...) (type *)PushSize_(Arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(Arena, Count, type, ...) (type *)PushSize_(Arena, (Count)*sizeof(type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)
inline void *
PushSize_(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = 4)
{
    memory_index Size = SizeInit;

    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Size += AlignmentOffset;

    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;

    Assert(Size >= SizeInit);

    return(Result);
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
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    uint8_t *Byte = (uint8_t *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

#include "game_intrinsics.h"
#include "game_math.h"
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

    random_series GeneralEntropy;
    real32 tSine;

    audio_state AudioState;
    playing_sound *Music;
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

global_variable platform_add_entry *PlatformAddEntry;
global_variable platform_complete_all_work *PlatformCompleteAllWork;
global_variable debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;

internal task_with_memory *BeginTaskWithMemory(transient_state *TranState);
internal void EndTaskWithMemory(task_with_memory *Task);

#define GAME_H
#endif
