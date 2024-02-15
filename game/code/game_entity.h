#if !defined(GAME_ENTITY_H)

struct entity;

enum entity_type
{
    EntityType_Null,

    EntityType_HeroBody,
    EntityType_HeroHead,

    EntityType_Wall,
    EntityType_Floor,
    EntityType_FloatyThingForNow,
    EntityType_Familiar,
    EntityType_Monster,
    EntityType_Stairwell,
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
    uint8_t Flags;
    uint8_t FilledAmount;
};

struct entity_id 
{
    u32 Value;
};

struct entity_reference
{
    entity *Ptr;
    entity_id Index;
};
inline b32 ReferencesAreEqual(entity_reference A, entity_reference B)
{
    b32 Result = ((A.Ptr == B.Ptr) &&
                  (A.Index.Value == B.Index.Value));
    return(Result);
};

struct traversable_reference
{
    entity_reference Entity;
    u32 Index;
};
inline b32 IsEqual(traversable_reference A, traversable_reference B)
{
    b32 Result = (ReferencesAreEqual(A.Entity, B.Entity) &&
                  (A.Index == B.Index));

    return(Result);
}

enum entity_flags
{
    // TODO Collides and ZSupported can prabably be removed.
    EntityFlag_Collides = (1 << 0),
    EntityFlag_Moveable = (1 << 1),
    EntityFlag_Deleted = (1 << 2),
};

struct entity_collision_volume
{
    v3 OffsetP;
    v3 Dim;
};

struct entity_collision_volume_group
{
    entity_collision_volume TotalVolume;

    // TODO VolumeCount is always expected to be greater than 0 if the entity
    // has any volume. In the future, this could be compressed if necessary
    // to say that the volumecount can be 0 if the totalvolume should be
    // used as the only collision volume for the entity.
    u32 VolumeCount;
    entity_collision_volume *Volumes;
};

struct entity_traversable_point
{
    v3 P;
    entity *Occupier;
};

enum brain_type
{
    Brain_Hero,
    Brain_Snake,

    Brain_Count,
};
struct brain_slot
{
    u32 Index;
};
struct brain_id
{
    u32 Value;
};

enum entity_movement_mode
{
    MovementMode_Planted,
    MovementMode_Hopping,
};
struct entity
{
    entity_id ID;
    b32 Updatable;

    //
    // NOTE: Things I've thought about for just a modicum
    //

    //
    // NOTE: Everything below here is not worked out
    //

    brain_type BrainType;
    brain_slot BrainSlot;
    brain_id BrainID;

    entity_type Type;
    u32 Flags;

    v3 P;
    v3 dP;
    v3 ddP;

    r32 DistanceLimit;

    entity_collision_volume_group *Collision;

    r32 FacingDirection;
    r32 tBob;
    r32 dtBob;

    s32 dAbsTileZ;

    u32 HitPointMax;
    hit_point HitPoint[16];

    // TODO Only for Stairwells
    v2 WalkableDim;
    r32 WalkableHeight;

    entity_movement_mode MovementMode;
    r32 tMovement;
    traversable_reference Occupying;
    traversable_reference CameFrom;

    v2 XAxis;
    v2 YAxis;

    v2 FloorDisplace;

    u32 TraversableCount;
    entity_traversable_point Traversables[16];

    // TODO: Generation index so I know how "up to date" this entity is
};

struct brain_hero_parts
{
    entity *Head;
    entity *Body;
};

struct brain
{
    brain_id ID;
    brain_type Type;

    union 
    {
        brain_hero_parts Hero;
        entity *Array[16];
    };
};

enum reserved_brain_id
{
    ReservedBrainID_FirstHero = 1,
    ReservedBrainID_LastHero = (ReservedBrainID_FirstHero + MAX_CONTROLLER_COUNT - 1),

    ReservedBrainID_FirstFree,
};

#define InvalidP V3(100000.0f, 100000.0f, 100000.0f)

inline bool32
IsSet(entity *Entity, uint32_t Flag)
{
    bool32 Result = Entity->Flags & Flag;

    return(Result);
}

inline b32 IsDeleted(entity *E) {b32 Result = IsSet(E, EntityFlag_Deleted); return(Result);}

inline void 
AddFlags(entity *Entity, uint32_t Flag)
{
    Entity->Flags |= Flag;
}

inline void
ClearFlags(entity *Entity, uint32_t Flag)
{
    Entity->Flags &= ~Flag;
}

inline void
MakeEntitySpatial(entity *Entity, v3 P, v3 dP)
{
    Entity->P = P;
    Entity->dP = dP;
}

inline v3
GetEntityGroundPoint(entity *Entity, v3 ForEntityP)
{
    v3 Result = ForEntityP;

    return(Result);
}

inline v3
GetEntityGroundPoint(entity *Entity)
{
    v3 Result = GetEntityGroundPoint(Entity, Entity->P);

    return(Result);
}

inline real32
GetStairGround(entity *Entity, v3 AtGroundPoint)
{
    Assert(Entity->Type == EntityType_Stairwell);

    rectangle2 RegionRect = RectCenterDim(Entity->P.xy, Entity->WalkableDim);
    v2 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint.xy));
    real32 Result = Entity->P.z + Bary.y*Entity->WalkableHeight;

    return(Result);
}

#define GAME_ENTITY_H
#endif
