#if !defined(GAME_WORLD_H)

struct world_position
{
    // It seems like we have to store ChunkX/Y/Z with each
    // Entity because even though the sim region gather doesn't
    // need it at first, and we could get by without it, entity
    // references pull in entities WITHOUT going through their
    // world_chunk, and so still need to know the ChunkX/Y/Z.

    int32_t ChunkX;
    int32_t ChunkY;
    int32_t ChunkZ;

    v3 Offset_;
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
    v3 ChunkDimInMeters;

    world_entity_block *FirstFree;

    world_chunk ChunkHash[4096];
};

#define GAME_WORLD_H
#endif
