#if !defined(GAME_TILE_H)

struct tile_map_difference
{
    v2 dXY;
    real32 dZ;
};

struct tile_map_position
{
    // NOTE: These are fixed point tile locations. The high
    // bits are the tile_chunk index, and the low bits are
    // the tile index in the chunk.
    int32_t AbsTileX;
    int32_t AbsTileY;
    int32_t AbsTileZ;

    v2 Offset_;
};

struct tile_chunk_position
{
    int32_t TileChunkX;
    int32_t TileChunkY;
    int32_t TileChunkZ;

    int32_t RelTileX;
    int32_t RelTileY;
};

struct tile_chunk
{
    int32_t TileChunkX;
    int32_t TileChunkY;
    int32_t TileChunkZ;

    uint32_t *Tiles;

    tile_chunk *NextInHash;
};

struct tile_map
{
    int32_t ChunkShift;
    int32_t ChunkMask;
    int32_t ChunkDim;

    real32 TileSideInMeters;

    tile_chunk TileChunkHash[4096];
};

#define GAME_TILE_H
#endif
