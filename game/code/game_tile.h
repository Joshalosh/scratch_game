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
    uint32_t AbsTileX;
    uint32_t AbsTileY;
    uint32_t AbsTileZ;

    v2 Offset_;
};

struct tile_chunk_position
{
    uint32_t TileChunkX;
    uint32_t TileChunkY;
    uint32_t TileChunkZ;

    uint32_t RelTileX;
    uint32_t RelTileY;
};

struct tile_chunk
{
    uint32_t TileChunkX;
    uint32_t TileChunkY;
    uint32_t TileChunkZ;

    uint32_t *Tiles;

    tile_chunk *NextInHash;
};

struct tile_map
{
    uint32_t ChunkShift;
    uint32_t ChunkMask;
    uint32_t ChunkDim;

    real32 TileSideInMeters;

    tile_chunk *TileChunkHash[4096];
};

#define GAME_TILE_H
#endif
