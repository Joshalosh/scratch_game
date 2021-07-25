#if !defined(GAME_SIM_REGION_H)

struct sim_entity
{
    uint32_t StorageIndex;

    v2 P;
    uint32_t ChunkZ;

    real32 Z;
    real32 dZ;
};

struct sim_region
{
    world *World;

    world_position Origin;
    rectangle2 Bounds;

    uint32_t MaxEntityCount;
    uint32_t EntityCount;
    entity *Entities;
};

#define GAME_SIM_REGION_H
#endif
