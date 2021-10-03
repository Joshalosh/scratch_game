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
    EntityType_Stairwell,
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
    EntityFlag_Collides = (1 << 0),
    EntityFlag_Nonspatial = (1 << 1),
    EntityFlag_Moveable = (1 << 2),
    EntityFlag_ZSupported = (1 << 4),

    EntityFlag_Simming = (1 << 30),
};

struct sim_entity
{
    // Note: These are only for the sim region.
    uint32_t StorageIndex;
    bool32 Updatable;

    entity_type Type;
    uint32_t Flags;

    v3 P;
    v3 dP;

    real32 DistanceLimit;

    v3 Dim;

    uint32_t FacingDirection;
    real32 tBob;

    int32_t dAbsTileZ;

    uint32_t HitPointMax;
    hit_point HitPoint[16];

    entity_reference Sword;

    // TODO Only for Stairwells
    real32 WalkableHeight;
};

struct sim_entity_hash
{
    sim_entity *Ptr;
    uint32_t Index;
};

struct sim_region
{
    world *World;
    real32 MaxEntityRadius;
    real32 MaxEntityVelocity;

    world_position Origin;
    rectangle3 Bounds;
    rectangle3 UpdatableBounds;

    uint32_t MaxEntityCount;
    uint32_t EntityCount;
    sim_entity *Entities;

    sim_entity_hash Hash[4096];
};

#define GAME_SIM_REGION_H
#endif
