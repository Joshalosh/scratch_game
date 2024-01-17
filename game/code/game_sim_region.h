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

    EntityType_HeroBody,
    EntityType_HeroHead,

    EntityType_Wall,
    EntityType_Floor,
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
    // TODO Collides and ZSupported can prabably be removed.
    EntityFlag_Collides = (1 << 0),
    EntityFlag_Nonspatial = (1 << 1),
    EntityFlag_Moveable = (1 << 2),

    EntityFlag_Simming = (1 << 30),
};

struct sim_entity_collision_volume
{
    v3 OffsetP;
    v3 Dim;
};

struct sim_entity_traversable_point
{
    v3 P;
};

struct sim_entity_collision_volume_group
{
    sim_entity_collision_volume TotalVolume;

    // TODO VolumeCount is always expected to be greater than 0 if the entity
    // has any volume. In the future, this could be compressed if necessary
    // to say that the volumecount can be 0 if the totalvolume should be
    // used as the only collision volume for the entity.
    u32 VolumeCount;
    sim_entity_collision_volume *Volumes;

    u32 TraversableCount;
    sim_entity_traversable_point *Traversables;
};

enum sim_movement_mode
{
    MovementMode_Planted,
    MovementMode_Hopping,
};
struct sim_entity
{
    // Note: These are only for the sim region.
    world_chunk *OldChunk;
    u32 StorageIndex;
    b32 Updatable;

    entity_type Type;
    u32 Flags;

    v3 P;
    v3 dP;

    r32 DistanceLimit;

    sim_entity_collision_volume_group *Collision;

    r32 FacingDirection;
    r32 tBob;
    r32 dtBob;

    s32 dAbsTileZ;

    u32 HitPointMax;
    hit_point HitPoint[16];

    entity_reference Sword;
    entity_reference Head;

    // TODO Only for Stairwells
    v2 WalkableDim;
    r32 WalkableHeight;

    sim_movement_mode MovementMode;
    r32 tMovement;
    v3 MovementFrom;
    v3 MovementTo;

    // TODO: Generation index so I know how "up to date" this entity is
};

struct sim_entity_hash
{
    sim_entity *Ptr;
    uint32_t Index;
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
    sim_entity *Entities;

    sim_entity_hash Hash[4096];
};

#define GAME_SIM_REGION_H
#endif
