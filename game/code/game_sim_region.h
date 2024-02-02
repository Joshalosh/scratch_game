#if !defined(GAME_SIM_REGION_H)

struct move_spec
{
    bool32 UnitMaxAccelVector;
    real32 Speed;
    real32 Drag;
};

struct entity_hash
{
    entity *Ptr;
    entity_id Index;
};

struct sim_region
{
    world *World;
    r32 MaxEntityRadius;
    r32 MaxEntityVelocity;

    world_position Origin;
    rectangle3 Bounds;
    rectangle3 UpdatableBounds;

    u32 MaxEntityCount;
    u32 EntityCount;
    entity *Entities;

    entity_hash Hash[4096];
};

#define GAME_SIM_REGION_H
#endif
