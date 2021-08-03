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
#include "game_sim_region.h"

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

struct low_entity
{
    world_position P;
    sim_entity Stored;
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

struct controlled_hero
{
    uint32_t EntityIndex;

    v2 ddP;
    v2 dSword;
    real32 dZ;
};

struct game_state
{
    memory_arena WorldArena;
    world *World;

    // TODO: Allow split-screen?
    uint32_t CameraFollowingEntityIndex;
    world_position CameraP;

    controlled_hero ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];

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

#define GAME_H
#endif
