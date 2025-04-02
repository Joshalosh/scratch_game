
struct entity_id
{
    u32 Value;
};
inline b32 IsEqual(entity_id A, entity_id B)
{
    b32 Result = (A.Value == B.Value);

    return(Result);
}

struct world_position
{
    // It seems like we have to store ChunkX/Y/Z with each
    // Entity because even though the sim region gather doesn't
    // need it at first, and we could get by without it, entity
    // references pull in entities WITHOUT going through their
    // world_chunk, and so still need to know the ChunkX/Y/Z.

    s32 ChunkX;
    s32 ChunkY;
    s32 ChunkZ;

    v3 Offset_;
};

struct world_entity_block
{
    u32 EntityCount;
    u32 LowEntityIndex[16];
    world_entity_block *Next;

    u32 EntityDataSize;
    u8 EntityData[1 << 16];
};

struct world_chunk
{
    int32_t ChunkX;
    int32_t ChunkY;
    int32_t ChunkZ;

    world_entity_block *FirstBlock;

    world_chunk *NextInHash;
};

struct world
{
    v3 ChunkDimInMeters;

    world_entity_block *FirstFree;

    world_chunk *ChunkHash[4096];

    memory_arena Arena;

    world_chunk *FirstFreeChunk;
    world_entity_block *FirstFreeBlock;
};
