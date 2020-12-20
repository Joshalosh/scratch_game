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

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#if GAME_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *) 0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32_t
SafeTruncateUInt64(uint64_t Value)
{
    // TODO: Define maximum values i.e UInt32Max
    Assert(Value <= 0xFFFFFFFF);
    uint32_t Result = (uint32_t)Value;
    return(Result);
}

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

struct tile_chunk_position
{
    uint32_t TileChunkX;
    uint32_t TileChunkY;

    uint32_t RelTileX;
    uint32_t RelTileY;
};
struct world_position
{
    uint32_t AbsTileX;
    uint32_t AbsTileY;

    real32 TileRelX;
    real32 TileRelY;
};

struct tile_chunk
{
    uint32_t *Tiles;
};

struct world
{
    uint32_t ChunkShift;
    uint32_t ChunkMask;
    uint32_t ChunkDim;

    real32 TileSideInMeters;
    int32_t TileSideInPixels;
    real32 MetersToPixels;

    int32_t TileChunkCountX;
    int32_t TileChunkCountY;

    tile_chunk *TileChunks;
};

struct game_state
{
    world_position PlayerP;
};

#define GAME_H
#endif
