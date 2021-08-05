#if !defined(GAME_SIM_REGION_H)

struct move_spec
{
    bool UnitMaxAccelVector;
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

struct sim_entity
{
    uint32_t StorageIndex;

    entity_type Type;

    v2 P;
    uint32_t ChunkZ;

    real32 Z;
    real32 dZ;

    v2 dP;
    real32 Width, Height;

    uint32_t FacingDirection;
    real32 tBob;

    bool32 Collides;
    int32_t dAbsTileZ;

    uint32_t HitPointMax;
    hit_point HitPoint[16];

    entity_reference Sword;
    real32 DistanceRemaining;
};

struct sim_region
{
    world *World;

    world_position Origin;
    rectangle2 Bounds;

    uint32_t MaxEntityCount;
    uint32_t EntityCount;
    sim_entity *Entities;

    sim_entity_hash Hash[4096];
};

#define GAME_SIM_REGION_H
#endif
