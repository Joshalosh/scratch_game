
struct entity;

struct entity_id
{
    u32 Value;
};
inline b32 IsEqual(entity_id A, entity_id B)
{
    b32 Result = (A.Value == B.Value);

    return(Result);
}

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
    // TODO: Bake this down into one variable
    uint8_t Flags;
    uint8_t FilledAmount;
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
}

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
    EntityFlag_Collides = (1 << 0),
    EntityFlag_Deleted = (1 << 1),
    EntityFlag_Active = (1 << 2),
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

struct move_spec
{
    bool32 UnitMaxAccelVector;
    real32 Speed;
    real32 Drag;
};

enum entity_movement_mode
{
    MovementMode_Planted,
    MovementMode_Hopping,
    MovementMode_Floating,

    MovementMode_AngleOffset,
    MovementMode_AngleAttackSwipe,
};

enum entity_visible_piece_flag
{
    PieceMove_AxesDeform = 0x1,
    PieceMove_BobOffset = 0x2,
};

struct entity_visible_piece
{
    v4 Color;
    v3 Offset;
    asset_type_id AssetType;
    r32 Height;
    u32 Flags;
};

struct entity
{
    //
    // NOTE: Things I've thought about for just a modicum
    //

    entity_id ID;

    brain_slot BrainSlot;
    brain_id BrainID;

    s32 ZLayer;

    //
    // NOTE: Tansient
    //

    manual_sort_key ManualSort;

    //
    // NOTE: Everything below here is not worked out
    //

    //entity_type Type;
    u32 Flags;

    v3 P;
    v3 dP;
    v3 ddP; // TODO: Do not pack this

    r32 DistanceLimit;

    entity_collision_volume_group *Collision;

    r32 FacingDirection;
    r32 tBob;
    r32 dtBob;
    r32 ddtBob; // TODO: Do not pack this

    s32 dAbsTileZ;

    // TODO: Should hitpoints themselves be entities?
    u32 HitPointMax;
    hit_point HitPoint[16];

    // TODO Only for Stairwells
    v2 WalkableDim;
    r32 WalkableHeight;

    entity_movement_mode MovementMode;
    r32 tMovement;
    traversable_reference Occupying;
    traversable_reference CameFrom;

    v3 AngleBase;
    r32 AngleCurrent;
    r32 AngleStart;
    r32 AngleTarget;
    r32 AngleCurrentDistance;
    r32 AngleBaseDistance;
    r32 AngleSwipeDistance;

    v2 XAxis;
    v2 YAxis;

    v2 FloorDisplace;

    u32 TraversableCount;
    entity_traversable_point Traversables[16];

    u32 PieceCount;
    entity_visible_piece Pieces[4]; // NOTE: 0 is the most "on top" piece, N is the "on bottom" piece

    // TODO: Generation index so I know how "up to date" this entity is

    traversable_reference AutoBoostTo;
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
    rectangle2 RegionRect = RectCenterDim(Entity->P.xy, Entity->WalkableDim);
    v2 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint.xy));
    real32 Result = Entity->P.z + Bary.y*Entity->WalkableHeight;

    return(Result);
}
