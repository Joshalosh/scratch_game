#if !defined(GAME_SIM_REGION_H)

struct move_spec
{
    bool32 UnitMaxAccelVector;
    real32 Speed;
    real32 Drag;
};

enum entity_type
{
    EntityType_Null,

    EntityType_Hero,
    EntityType_Wall,
    EntityType_Familiar,
    EntityType_Monster,
    EntityType_Sword,
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
    uint8_t Flags;
    uint8_t FilledAmount;
};

struct sim_entity;
union entity_reference
{
    sim_entity *Ptr;
    uint32_t Index;
};

enum sim_entity_flags
{
    EntityFlag_Collides   = (1 << 1),
    EntityFlag_Nonspatial = (1 << 2),

    EntityFlag_Simming = (1 << 30),
};

struct sim_entity
{
    // Note: These are only for the sim region.
    uint32_t StorageIndex;
    bool32 Updatable;

    entity_type Type;
    uint32_t Flags;

    v2 P;
    v2 dP;

    real32 Z;
    real32 dZ;

    uint32_t ChunkZ;

    real32 Width, Height;

    uint32_t FacingDirection;
    real32 tBob;

    int32_t dAbsTileZ;

    uint32_t HitPointMax;
    hit_point HitPoint[16];

    entity_reference Sword;
    real32 DistanceRemaining;
};

struct sim_entity_hash
{
    sim_entity *Ptr;
    uint32_t Index;
};

struct sim_region
{
    world *World;

    world_position Origin;
    rectangle2 Bounds;
    rectangle2 UpdatableBounds;

    uint32_t MaxEntityCount;
    uint32_t EntityCount;
    sim_entity *Entities;

    sim_entity_hash Hash[4096];
};

#define GAME_SIM_REGION_H
#endif
