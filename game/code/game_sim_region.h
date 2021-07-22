#if !defined(GAME_SIM_REGION_H)

struct sim_region
{
    uint32_t MaxEntityCount;
    uint32_t EntityCount;
    entity *Entities;
};

#define GAME_SIM_REGION_H
#endif
