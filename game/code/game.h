#if !defined(GAME_H)

/*
  GAME_INTERNAL:
    0 - Build for public release
    1 - Build for developer only

  GAME_SLOW:
    0 - No slow code allowed
    1 - Slow code
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

};

internal void
InitialiseArena(memory_arena *Arena, memory_index Size, uint8_t *Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
void *
PushSize_(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return(Result);
}

#include "game_math.h"
#include "game_intrinsics.h"
#include "game_world.h"

struct loaded_bitmap
{
    int32_t  Width;
    int32_t  Height;
    uint32_t *Pixels;
};

struct hero_bitmaps
{
    int32_t AlignX;
    int32_t AlignY;
    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

struct high_entity
{
    v2 P;
    v2 dP;
    uint32_t ChunkZ;
    uint32_t FacingDirection;

    real32 Z;
    real32 dZ;

    uint32_t LowEntityIndex;
};

enum entity_type
{
    EntityType_Null,

    EntityType_Hero,
    EntityType_Wall,
    EntityType_Familiar,
    EntityType_Monster,
};

struct low_entity
{
    entity_type Type;

    world_position P;
    real32 Width, Height;

    // This is for the stairs or whatever the "stairs"
    // are going to be
    bool32 Collides;
    int32_t dAbsTileZ;

    uint32_t HighEntityIndex;
};

struct entity
{
    uint32_t LowIndex;
    low_entity *Low;
    high_entity *High;
};

struct game_state
{
    memory_arena WorldArena;
    world *World;

    // TODO: Allow split-screen?
    uint32_t CameraFollowingEntityIndex;
    world_position CameraP;

    uint32_t PlayerIndexForController[ArrayCount(((game_input *)0)->Controllers)];

    uint32_t LowEntityCount;
    low_entity LowEntities[100000];

    uint32_t HighEntityCount;
    high_entity HighEntities_[256];

    loaded_bitmap Backdrop;
    loaded_bitmap Shadow;
    hero_bitmaps HeroBitmaps[4];

    loaded_bitmap Tree;
};

#define GAME_H
#endif
