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

#include "game_intrinsics.h"
#include "game_math.h"
#include "game_world.h"

struct loaded_bitmap
{
    int32_t  Width;
    int32_t  Height;
    uint32_t *Pixels;
};

struct hero_bitmaps
{
    v2 Align;
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

    real32 tBob;

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
    EntityType_Sword,
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
    uint8_t Flags;
    uint8_t FilledAmount;
};

struct low_entity
{
    entity_type Type;

    world_position P;
    real32 Width, Height;

    bool32 Collides;
    int32_t dAbsTileZ;

    uint32_t HighEntityIndex;

    uint32_t HitPointMax;
    hit_point HitPoint[16];

    uint32_t SwordLowIndex;
    real32 DistanceRemaining;
};

struct entity
{
    uint32_t LowIndex;
    low_entity *Low;
    high_entity *High;
};

struct entity_visible_piece
{
    loaded_bitmap *Bitmap;
    v2 Offset;
    real32 OffsetZ;
    real32 EntityZC;

    real32 R, G, B, A;
    v2 Dim;
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

    loaded_bitmap Backdrop;
    loaded_bitmap Shadow;
    hero_bitmaps HeroBitmaps[4];

    loaded_bitmap Tree;
    loaded_bitmap Sword;
    real32 MetersToPixels;
};

struct entity_visible_piece_group
{
    game_state *GameState;
    uint32_t PieceCount;
    entity_visible_piece Pieces[32];
};

#define GAME_H
#endif
