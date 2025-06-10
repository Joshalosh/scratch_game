struct entity_hash
{
    entity *Ptr;
};

struct brain_hash
{
    brain *Ptr;
};

struct sim_region
{
    world *World;

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

    u64 EntityHashOccupancy[4096/64];
    u64 BrainHashOccupancy[256/64];

    entity NullEntity;
};

internal entity_hash *GetHashFromID(sim_region *SimRegion, entity_id StorageIndex);
