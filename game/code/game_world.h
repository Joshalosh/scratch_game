#if !defined(GAME_WORLD_H)

struct world_position
{
    // NOTE: These are fixed point tile locations. The high
    // bits are the tile_chunk index, and the low bits are
    // the tile index in the chunk.
    int32_t ChunkX;
    int32_t ChunkY;
    int32_t ChunkZ;

    v2 Offset_;
};

struct world_entity_block
{
    uint32_t EntityCount;
    uint32_t LowEntityIndex[16];
    world_entity_block *Next;
};

struct world_chunk
{
    int32_t ChunkX;
    int32_t ChunkY;
    int32_t ChunkZ;

    world_entity_block FirstBlock;

    world_chunk *NextInHash;
};

struct world
{
    real32 TileSideInMeters;
    v3 ChunkDimInMeters;

    world_entity_block *FirstFree;

    world_chunk ChunkHash[4096];
};

#define GAME_WORLD_H
#endif
