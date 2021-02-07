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
#include "game_tile.h"

struct world
{
    tile_map *TileMap;
};

struct loaded_bitmap
{
    int32_t Width;
    int32_t Height;
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

struct game_state
{
    memory_arena WorldArena;
    world *World;

    tile_map_position CameraP;
    tile_map_position PlayerP;

    loaded_bitmap Backdrop;
    uint32_t HeroFacingDirection;
    hero_bitmaps HeroBitmaps[4];
};

#define GAME_H
#endif
