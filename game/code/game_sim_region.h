struct entity_hash
{
    entity *Ptr;
    entity_id Index; // TODO: Why is this being stored in the hash?
};

struct brain_hash
{
    brain *Ptr;
    brain_id ID; // TODO: Why is this being stored in the hash?
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

    u32 MaxBrainCount;
    u32 BrainCount;
    brain *Brains;

    entity_hash EntityHash[4096];
    brain_hash BrainHash[256];
};

internal entity_hash *GetHashFromID(sim_region *SimRegion, entity_id StorageIndex);
