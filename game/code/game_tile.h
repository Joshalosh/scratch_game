#if !defined(GAME_TILE_H)

struct tile_map_difference
{
    real32 dX;
    real32 dY;
    real32 dZ;
}

struct tile_map_position
{
    // NOTE: These are fixed point tile locations. The high
    // bits are the tile_chunk index, and the low bits are
    // the tile index in the chunk.
    uint32_t AbsTileX;
    uint32_t AbsTileY;
    uint32_t AbsTileZ;

    real32 OffsetX;
    real32 OffsetY;
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
    uint32_t *Tiles;
};

struct tile_map
{
    uint32_t ChunkShift;
    uint32_t ChunkMask;
    uint32_t ChunkDim;

    real32 TileSideInMeters;

    uint32_t TileChunkCountX;
    uint32_t TileChunkCountY;
    uint32_t TileChunkCountZ;

    tile_chunk *TileChunks;
};

#define GAME_TILE_H
#endif
