#if !defined(GAME_SIM_REGION_H)

struct sim_entity
{
    uint32_t StorageIndex;

    entity_type Type;

    v2 P;
    uint32_t ChunkZ;

    real32 Z;
    real32 dZ;

    world_position P;
    v2 dP;
    real32 Width, Height;

    uint32_t FacingDirection;
    real32 tBob;

    bool32 Collides;
    int32 dAbsTileZ;

    uint32_t HitPointMax;
    hit_point HitPoint[16];

    uint32_t SwordLowIndex;
    real32 DistanceRemaining;
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
