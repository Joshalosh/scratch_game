
inline entity_traversable_point *
GetTraversable(entity *Entity, u32 Index)
{
    entity_traversable_point *Result = 0;
    if(Entity)
    {
        Assert(Index < Entity->TraversableCount);
        Result = Entity->Traversables + Index;
    }
    return(Result);
}

inline entity_traversable_point *
GetTraversable(traversable_reference Reference)
{
    entity_traversable_point *Result = GetTraversable(Reference.Entity.Ptr, Reference.Index);
    return(Result);
}

inline entity_traversable_point
GetSimSpaceTraversable(entity *Entity, u32 Index)
{
    entity_traversable_point Result = {};
    if(Entity)
    {
        Result.P = Entity->P;

        entity_traversable_point *Point = GetTraversable(Entity, Index);
        if(Point)
        {
            // TODO: This wants to be rotated eventuially
            Result.P += Point->P;
            Result.Occupier = Point->Occupier;
        }
    }

    return(Result);
}

inline entity_traversable_point
GetSimSpaceTraversable(traversable_reference Reference)
{
    entity_traversable_point Result = GetSimSpaceTraversable(Reference.Entity.Ptr, Reference.Index);
    return(Result);
}

internal void
MarkBit(u64 *Array, umm Index)
{
    umm OccIndex = Index / 64;
    umm BitIndex = Index % 64;
    Array[OccIndex] |= ((u64)1 << BitIndex);
}

internal b32
IsEmpty(u64 *Array, umm Index)
{
    umm OccIndex = Index / 64;
    umm BitIndex = Index % 64;
    b32 Result = !(Array[OccIndex] & ((u64)1 << BitIndex));
    return(Result);
}

internal void
MarkOccupied(sim_region *SimRegion, entity_hash *Entry)
{
    umm Index = Entry - SimRegion->EntityHash;
    MarkBit(SimRegion->EntityHashOccupancy, Index);
}

internal void
MarkOccupied(sim_region *SimRegion, brain_hash *Entry)
{
    umm Index = Entry - SimRegion->BrainHash;
    MarkBit(SimRegion->BrainHashOccupancy, Index);
}


internal entity_hash *
GetHashFromID(sim_region *SimRegion, entity_id StorageIndex)
{
    Assert(StorageIndex.Value);

    entity_hash *Result = 0;

    uint32_t HashValue = StorageIndex.Value;
    for(uint32_t Offset = 0; Offset < ArrayCount(SimRegion->EntityHash); ++Offset)
    {
        uint32_t HashMask  = (ArrayCount(SimRegion->EntityHash) - 1);
        uint32_t HashIndex = ((HashValue + Offset) & HashMask);
        entity_hash *Entry = SimRegion->EntityHash + HashIndex;
        if(IsEmpty(SimRegion->EntityHashOccupancy, HashIndex))
        {
            Result = Entry;
            Result->Ptr = 0;
            break;
        }
        else if(Entry->Ptr->ID.Value == StorageIndex. Value)
        {
            Result = Entry;
            break;
        }
    }

    Assert(Result);
    return(Result);
}

internal brain_hash *
GetHashFromID(sim_region *SimRegion, brain_id StorageIndex)
{
    Assert(StorageIndex.Value);

    brain_hash *Result = 0;

    u32 HashValue = StorageIndex.Value;
    for(u32 Offset = 0; Offset < ArrayCount(SimRegion->BrainHash); ++Offset)
    {
        u32 HashMask = (ArrayCount(SimRegion->BrainHash) - 1);
        u32 HashIndex = ((HashValue + Offset) & HashMask);
        brain_hash *Entry = SimRegion->BrainHash + HashIndex;
        if(IsEmpty(SimRegion->BrainHashOccupancy, HashIndex))
        {
            Result = Entry;
            Result->Ptr = 0;
            break;
        }
        else if (Entry->Ptr->ID.Value == StorageIndex.Value)
        {
            Result = Entry;
            break;
        }
    }

    Assert(Result);
    return(Result);
}

inline entity *
GetEntityByID(sim_region *SimRegion, entity_id ID)
{
    entity_hash *Entry = GetHashFromID(SimRegion, ID);
    entity *Result = Entry ? Entry->Ptr : 0;
    return(Result);
}

inline void
LoadEntityReference(sim_region *SimRegion, entity_reference *Ref)
{
    if(Ref->Index.Value)
    {
        Ref->Ptr = GetEntityByID(SimRegion, Ref->Index);
    }
}

inline void
LoadTraversableReference(sim_region *SimRegion, traversable_reference *Ref)
{
    LoadEntityReference(SimRegion, &Ref->Entity);
}

inline bool32
EntityOverlapsRectangle(v3 P, entity_collision_volume Volume, rectangle3 Rect)
{
    rectangle3 Grown = AddRadiusTo(Rect, 0.5f*Volume.Dim);
    bool32 Result = IsInRectangle(Grown, P + Volume.OffsetP);
    return(Result);
}

internal brain *
GetOrAddBrain(sim_region *SimRegion, brain_id ID, brain_type Type)
{
    TIMED_FUNCTION();

    brain *Result = 0;

    brain_hash *Hash = GetHashFromID(SimRegion, ID);
    Result = Hash->Ptr;

    if(!Result)
    {
        Assert(SimRegion->BrainCount < SimRegion->MaxBrainCount);
        Assert(IsEmpty(SimRegion->BrainHashOccupancy, Hash - SimRegion->BrainHash));
        Result = SimRegion->Brains + SimRegion->BrainCount++;
        ZeroStruct(*Result);
        Result->ID = ID;
        Result->Type = Type;

        Hash->Ptr = Result;

        MarkOccupied(SimRegion, Hash);
    }

    return(Result);
}

internal void
ConnectEntityPointers(sim_region *SimRegion)
{
    TIMED_FUNCTION();

    for(u32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
    {
        entity *Entity = SimRegion->Entities + EntityIndex;

        LoadTraversableReference(SimRegion, &Entity->Occupying);
        if(Entity->Occupying.Entity.Ptr)
        {
            Entity->Occupying.Entity.Ptr->Traversables[Entity->Occupying.Index].Occupier = Entity;
        }

        LoadTraversableReference(SimRegion, &Entity->CameFrom);
        LoadTraversableReference(SimRegion, &Entity->AutoBoostTo);
    }
}

inline void
AddEntityToHash(sim_region *Region, entity *Entity)
{
    TIMED_FUNCTION();

    entity_hash *Entry = GetHashFromID(Region, Entity->ID);
    Assert(IsEmpty(Region->EntityHashOccupancy, Entry - Region->EntityHash));
    Entry->Ptr = Entity;
    MarkOccupied(Region, Entry);
}

inline void
PackEntityReference(sim_region *SimRegion, entity_reference *Ref)
{
    if(Ref->Ptr)
    {
        if(IsDeleted(Ref->Ptr))
        {
            Ref->Index.Value = 0;
        }
        else
        {
            Ref->Index = Ref->Ptr->ID;
        }
    }
    else if(Ref->Index.Value)
    {
        if(SimRegion && GetHashFromID(SimRegion, Ref->Index))
        {
            Ref->Index.Value = 0;
        }
    }
}

inline void
PackTraversableReference(sim_region *SimRegion, traversable_reference *Ref)
{
    // TODO: Need to pack this
    PackEntityReference(SimRegion, &Ref->Entity);
}

internal sim_region *
BeginWorldChange(memory_arena *SimArena, world *World, world_position Origin, rectangle3 Bounds, real32 dt)
{
    TIMED_FUNCTION();

    BEGIN_BLOCK("SimArenaAlloc");
    sim_region *SimRegion = PushStruct(SimArena, sim_region, NoClear());
    END_BLOCK();

    BEGIN_BLOCK("SimArenaClear");
    ZeroStruct(SimRegion->EntityHashOccupancy);
    ZeroStruct(SimRegion->BrainHashOccupancy);
    ZeroStruct(SimRegion->NullEntity);
    END_BLOCK();

    SimRegion->World = World;

    SimRegion->Origin = Origin;
    SimRegion->Bounds = Bounds;
    SimRegion->UpdatableBounds = SimRegion->Bounds;

    // TODO: Need to be more specific about entity counts.
    SimRegion->MaxEntityCount = 4096;
    SimRegion->EntityCount = 0;
    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, entity, NoClear());

    SimRegion->MaxBrainCount = 256;
    SimRegion->BrainCount = 0;
    SimRegion->Brains = PushArray(SimArena, SimRegion->MaxBrainCount, brain, NoClear());

    world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));

    DEBUG_VALUE(SimRegion->Origin.ChunkX);
    DEBUG_VALUE(SimRegion->Origin.ChunkY);
    DEBUG_VALUE(SimRegion->Origin.ChunkZ);
    DEBUG_VALUE(SimRegion->Origin.Offset_);

    for(int32_t ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
    {
        for(int32_t ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
        {
            for(int32_t ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
            {
                world_chunk *Chunk = RemoveWorldChunk(World, ChunkX, ChunkY, ChunkZ);

                if(Chunk)
                {
                    Assert(Chunk->ChunkX == ChunkX);
                    Assert(Chunk->ChunkY == ChunkY);
                    Assert(Chunk->ChunkZ == ChunkZ);
                    world_position ChunkPosition = {ChunkX, ChunkY, ChunkZ};
                    v3 ChunkDelta = Subtract(SimRegion->World, &ChunkPosition, &SimRegion->Origin);
                    world_entity_block *FirstBlock = Chunk->FirstBlock;
                    world_entity_block *LastBlock = FirstBlock;
                    world_entity_block *Block = FirstBlock;
                    for(world_entity_block *Block = FirstBlock; Block; Block = Block->Next)
                    {
                        LastBlock = Block;

                        for(uint32_t EntityIndex = 0;
                            (EntityIndex < Block->EntityCount);
                            ++EntityIndex)
                        {
                            if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
                            {
                                entity *Source = (entity *)Block->EntityData + EntityIndex;
                                entity_id ID = Source->ID;

                                entity *Dest = SimRegion->Entities + SimRegion->EntityCount++;

                                Assert(Source);
                                {
                                    // TODO: This should really be a decompression step, not a copy.
                                    *Dest = *Source;

                                    Dest->ID = ID;
                                    Dest->ZLayer = ChunkZ;

                                    manual_sort_key Zero = {};
                                    Dest->ManualSort = Zero;
                                }

                                AddEntityToHash(SimRegion, Dest);
                                Dest->P += ChunkDelta;

                                if(EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume, SimRegion->UpdatableBounds))
                                {
                                    Dest->Flags |= EntityFlag_Active;
                                }

                                if(Dest->BrainID.Value)
                                {
                                    brain *Brain = GetOrAddBrain(SimRegion, Dest->BrainID, (brain_type)Dest->BrainSlot.Type);
                                    u8 *Ptr = (u8 *)&Brain->Array;
                                    Ptr += sizeof(entity *)*Dest->BrainSlot.Index;
                                    Assert(Ptr <= ((u8 *)Brain + sizeof(brain) - sizeof(entity *)));
                                    *((entity **)Ptr) = Dest;
                                }
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                    }

                    AddToFreeList(World, Chunk, FirstBlock, LastBlock);
                }
            }
        }
    }

    ConnectEntityPointers(SimRegion);

    DEBUG_VALUE(SimRegion->EntityCount);

    return(SimRegion);
}

internal void
EndWorldChange(sim_region *Region, world *World)
{
    TIMED_FUNCTION();

    entity *Entity = Region->Entities;
    for(uint32_t EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
    {
        if(!(Entity->Flags & EntityFlag_Deleted))
        {
            world_position EntityP = MapIntoChunkSpace(World, Region->Origin, Entity->P);
            world_position ChunkP = EntityP;
            ChunkP.Offset_ = V3(0, 0, 0);

            v3 ChunkDelta = EntityP.Offset_ - Entity->P;

            // TODO: Camera code to go here

            v3 OldEntityP = Entity->P;
            Entity->P += ChunkDelta;
            entity *DestE = (entity *)UseChunkSpace(World, sizeof(*Entity), ChunkP);
            *DestE = *Entity;
            PackTraversableReference(Region, &DestE->Occupying);
            PackTraversableReference(Region, &DestE->CameFrom);
            PackTraversableReference(Region, &DestE->AutoBoostTo);

            DestE->ddP = V3(0, 0, 0);
            DestE->ddtBob = 0.0f;

            //v3 ReverseChunkDelta = Subtract(Region->World, &ChunkP, &Region->Origin);
            //v3 TestP = Entity->P + ReverseChunkDelta;
            //Assert(OldEntityP.z == TestP.z);
        }
    }
}

inline entity *
CreateEntity(sim_region *Region, entity_id ID)
{
    entity *Result = &Region->NullEntity;
    if(Region->EntityCount < Region->MaxEntityCount)
    {
        Result = Region->Entities + Region->EntityCount++;
    }
    else 
    {
        InvalidCodePath;
    }

    // TODO: Worry about this taking a while once the entities are large
    ZeroStruct(*Result);

    Result->ID = ID;
    AddEntityToHash(Region, Result);

    return(Result);
}

inline void
DeleteEntity(sim_region *Region, entity *Entity)
{
    if(Entity)
    {
        Entity->Flags |= EntityFlag_Deleted;
    }
}

internal void
UpdateCameraForEntityMovement(game_camera *Camera, world *World, entity *Entity)
{
    Assert(Entity->ID.Value == Camera->FollowingEntityIndex.Value)
    world_position NewCameraP = Camera->P;

    v3 RoomDelta = {24.0f, 12.5f, 3.0f};
    v3 hRoomDelta = 0.5f * RoomDelta;
    r32 ApronSize = 0.7f;
    r32 BounceHeight = 0.5f;
    v3 hRoomApron = {hRoomDelta.x - ApronSize, hRoomDelta.y - ApronSize, hRoomDelta.z - ApronSize};

    Camera->Offset = V3(0, 0, 0);

    v3 AppliedDelta = {};
    for(u32 E = 0; E < 3; ++E)
    {
        if(Entity->P.E[E] > hRoomDelta.E[E])
        {
            AppliedDelta.E[E] = RoomDelta.E[E];
            NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
        }
        if(Entity->P.E[E] < -hRoomDelta.E[E])
        {
            AppliedDelta.E[E] = -RoomDelta.E[E];
            NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
        }
    }

    v3 EntityP = Entity->P - AppliedDelta;

    if(EntityP.y > hRoomApron.y)
    {
        r32 t = Clamp01MapToRange(hRoomApron.y, EntityP.y, hRoomDelta.y);
        Camera->Offset = V3(0, t*hRoomDelta.y, (-(t*t)+2.0f*t)*BounceHeight);
    }
    if(EntityP.y < -hRoomApron.y)
    {
        r32 t = Clamp01MapToRange(-hRoomApron.y, EntityP.y, -hRoomDelta.y);
        Camera->Offset = V3(0, -t*hRoomDelta.y, (-(t*t)+2.0f*t)*BounceHeight);
    }
    if(EntityP.x > hRoomApron.x)
    {
        r32 t = Clamp01MapToRange(hRoomApron.x, EntityP.x, hRoomDelta.x);
        Camera->Offset = V3(t*hRoomDelta.x, 0, (-(t*t)+2.0f*t)*BounceHeight);
    }
    if(EntityP.x < -hRoomApron.x)
    {
        r32 t = Clamp01MapToRange(-hRoomApron.x, EntityP.x, -hRoomDelta.x);
        Camera->Offset = V3(-t*hRoomDelta.x, 0, (-(t*t)+2.0f*t)*BounceHeight);
    }
    if(EntityP.z > hRoomApron.z)
    {
        r32 t = Clamp01MapToRange(hRoomApron.z, EntityP.z, hRoomDelta.z);
        Camera->Offset = V3(0, 0, t*hRoomDelta.z);
    }
    if(EntityP.z < -hRoomApron.z)
    {
        r32 t = Clamp01MapToRange(-hRoomApron.z, EntityP.z, -hRoomDelta.z);
        Camera->Offset = V3(0, 0, -t*hRoomDelta.z);
    }

    Camera->P = NewCameraP;
}

struct test_wall
{
    real32 X;
    real32 RelX;
    real32 RelY;
    real32 DeltaX;
    real32 DeltaY;
    real32 MinY;
    real32 MaxY;
    v3 Normal;
};
internal bool32
TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY,
         real32 *tMin, real32 MinY, real32 MaxY)
{
    bool32 Hit = false;

    real32 tEpsilon = 0.001f;
    if(PlayerDeltaX != 0.0f)
    {
        real32 tResult = (WallX - RelX) / PlayerDeltaX;
        real32 Y = RelY + tResult*PlayerDeltaY;
        if((tResult >= 0.0f) && (*tMin > tResult))
        {
            if((Y >= MinY) && (Y <= MaxY))
            {
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                Hit = true;
            }
        }
    }

    return(Hit);
}

internal bool32
CanCollide(entity *A, entity *B)
{
    b32 Result = false;

    if(A != B)
    {
        if(A->ID.Value > B->ID.Value)
        {
            entity *Temp = A;
            A = B;
            B = Temp;
        }

        if(IsSet(A, EntityFlag_Collides) && IsSet(B, EntityFlag_Collides))
        {
            Result = true;
        }
    }

    return(Result);
}

internal bool32
HandleCollision(entity *A, entity *B)
{
    bool32 StopsOnCollision = true;

#if 0
    if(A->Type > B->Type)
    {
        entity *Temp = A;
        A = B;
        B = Temp;
    }
#endif

    return(StopsOnCollision);
}

internal bool32
CanOverlap(entity *Mover, entity *Region)
{
    bool32 Result = false;

    if(Mover != Region)
    {
#if 0
        if(Region->Type == EntityType_Stairwell)
        {
            Result = true;
        }
#endif
    }

    return(Result);
}

internal bool32
SpeculativeCollide(entity *Mover, entity *Region, v3 TestP)
{
    bool32 Result = true;

#if 0
    if(Region->Type == EntityType_Stairwell)
    {
        real32 StepHeight = 0.1f;
#if 0
        Result = ((AbsoluteValue(GetEntityGroundPoint(Mover).z - Ground) > StepHeight) ||
                  ((Bary.y > 0.1f) && (Bary.y < 0.9f)));
#endif
        v3 MoverGroundPoint = GetEntityGroundPoint(Mover, TestP);
        real32 Ground = GetStairGround(Region, MoverGroundPoint);
        Result = (AbsoluteValue(MoverGroundPoint.z - Ground) > StepHeight);
    }
#endif

    return(Result);
}

internal bool32
EntitiesOverlap(entity *Entity, entity *TestEntity, v3 Epsilon = V3(0, 0, 0))
{
    bool32 Result = false;

    for(uint32_t VolumeIndex = 0;
        !Result && (VolumeIndex < Entity->Collision->VolumeCount);
        ++VolumeIndex)
    {
        entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;

        for(uint32_t TestVolumeIndex = 0;
            !Result && (TestVolumeIndex < TestEntity->Collision->VolumeCount);
            ++TestVolumeIndex)
        {
            entity_collision_volume *TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex;

            rectangle3 EntityRect = RectCenterDim(Entity->P + Volume->OffsetP, Volume->Dim + Epsilon);
            rectangle3 TestEntityRect = RectCenterDim(TestEntity->P + TestVolume->OffsetP, TestVolume->Dim);
            Result = RectanglesIntersect(EntityRect, TestEntityRect);
        }
    }

    return(Result);
}

inline b32
IsOccupied(traversable_reference Ref)
{
    b32 Result = true;

    entity_traversable_point *Dest = GetTraversable(Ref);
    if(Dest)
    {
        Result = (Dest->Occupier != 0);
    }

    return(Result);
}

internal b32
TransactionalOccupy(entity *Entity, traversable_reference *DestRef, traversable_reference DesiredRef)
{
    b32 Result = false;

    entity_traversable_point *Desired = GetTraversable(DesiredRef);
    if(Desired && !Desired->Occupier)
    {
        entity_traversable_point *Dest = GetTraversable(*DestRef);
        if(Dest)
        {
            Dest->Occupier = 0;
        }
        *DestRef = DesiredRef;
        Desired->Occupier = Entity;
        Result = true;
    }

    return(Result);
}

internal void
MoveEntity(sim_region *SimRegion, entity *Entity, r32 dt, v3 ddP)
{
    world *World = SimRegion->World;

    v3 PlayerDelta = (0.5f*ddP*Square(dt) + Entity->dP*dt);
    Entity->dP = ddP*dt + Entity->dP;
    // TODO Upgrade physical motion routinges to handle capping
    // the maximum velocity?
    //Assert(LengthSq(Entity->dP) <= Square(SimRegion->MaxEntityVelocity));

    r32 DistanceRemaining = Entity->DistanceLimit;
    if(DistanceRemaining == 0.0f)
    {
        // TODO: Do I want to formalise this number?
        DistanceRemaining = 10000.0f;
    }

    for(uint32_t Iteration = 0; Iteration < 4; ++Iteration)
    {
        real32 tMin = 1.0f;
        real32 tMax = 1.0f;

        real32 PlayerDeltaLength = Length(PlayerDelta);
        if(PlayerDeltaLength > 0.0f)
        {
            if(PlayerDeltaLength > DistanceRemaining)
            {
                tMin = (DistanceRemaining / PlayerDeltaLength);
            }

            v3 WallNormalMin = {};
            v3 WallNormalMax = {};
            entity *HitEntityMin = 0;
            entity *HitEntityMax = 0;

            v3 DesiredPosition = Entity->P + PlayerDelta;

            for(uint32_t TestHighEntityIndex = 0; TestHighEntityIndex < SimRegion->EntityCount; ++TestHighEntityIndex)
            {
                entity *TestEntity = SimRegion->Entities + TestHighEntityIndex;

                real32 OverlapEpsilon = 0.001f;

                if(CanCollide(Entity, TestEntity))
                {
                    for(uint32_t VolumeIndex = 0;
                        VolumeIndex < Entity->Collision->VolumeCount;
                        ++VolumeIndex)
                    {
                        entity_collision_volume *Volume =
                            Entity->Collision->Volumes + VolumeIndex;

                        for(uint32_t TestVolumeIndex = 0;
                            TestVolumeIndex < TestEntity->Collision->VolumeCount;
                            ++TestVolumeIndex)
                        {
                            entity_collision_volume *TestVolume =
                                TestEntity->Collision->Volumes + TestVolumeIndex;

                            v3 MinkowskiDiameter = {TestVolume->Dim.x + Volume->Dim.x,
                                                    TestVolume->Dim.y + Volume->Dim.y,
                                                    TestVolume->Dim.z + Volume->Dim.z};

                            v3 MinCorner = -0.5f*MinkowskiDiameter;
                            v3 MaxCorner = 0.5f*MinkowskiDiameter;

                            v3 Rel = ((Entity->P + Volume->OffsetP) -
                                      (TestEntity->P + TestVolume->OffsetP));

                            if((Rel.z >= MinCorner.z) && (Rel.z < MaxCorner.z))
                            {
                                test_wall Walls[] =
                                {
                                    {MinCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y, MinCorner.y, MaxCorner.y, V3(-1, 0, 0)},
                                    {MaxCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y, MinCorner.y, MaxCorner.y, V3(1, 0, 0)},
                                    {MinCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x, MinCorner.x, MaxCorner.x, V3(0, -1, 0)},
                                    {MaxCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x, MinCorner.x, MaxCorner.x, V3(0, 1, 0)},
                                };

                                real32 tMinTest = tMin;
                                bool32 HitThis = false;

                                v3 TestWallNormal = {};
                                for(uint32_t WallIndex = 0; WallIndex < ArrayCount(Walls); ++WallIndex)
                                {
                                    test_wall *Wall = Walls + WallIndex;

                                    real32 tEpsilon = 0.001f;
                                    if(Wall->DeltaX != 0.0f)
                                    {
                                        real32 tResult = (Wall->X - Wall->RelX) / Wall->DeltaX;
                                        real32 Y = Wall->RelY + tResult*Wall->DeltaY;
                                        if((tResult >= 0.0f) && (tMinTest > tResult))
                                        {
                                            if((Y >= Wall->MinY) && (Y <= Wall->MaxY))
                                            {
                                                tMinTest = Maximum(0.0f, tResult - tEpsilon);
                                                TestWallNormal = Wall->Normal;
                                                HitThis = true;
                                            }
                                        }
                                    }
                                }

                                if(HitThis)
                                {
                                    v3 TestP = Entity->P + tMinTest*PlayerDelta;
                                    if(SpeculativeCollide(Entity, TestEntity, TestP))
                                    {
                                        tMin = tMinTest;
                                        WallNormalMin = TestWallNormal;
                                        HitEntityMin = TestEntity;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            v3 WallNormal;
            entity *HitEntity;
            real32 tStop;
            if(tMin < tMax)
            {
                tStop = tMin;
                HitEntity = HitEntityMin;
                WallNormal = WallNormalMin;
            }
            else
            {
                tStop = tMax;
                HitEntity = HitEntityMax;
                WallNormal = WallNormalMax;
            }

            Entity->P += tStop*PlayerDelta;
            DistanceRemaining -= tStop*PlayerDeltaLength;
            if(HitEntity)
            {
                PlayerDelta = DesiredPosition - Entity->P;

                bool32 StopsOnCollision = HandleCollision(Entity, HitEntity);
                if(StopsOnCollision)
                {
                    PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
                    Entity->dP = Entity->dP - 1*Inner(Entity->dP, WallNormal)*WallNormal;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if(Entity->DistanceLimit != 0.0f)
    {
        Entity->DistanceLimit = DistanceRemaining;
    }

#if 0
    if((Entity->dP.x == 0.0f) && (Entity->dP.y == 0.0f))
    {
        // NOTE: Leave facing direction as whatever it was.
    }
    else
    {
        Entity->FacingDirection = ATan2(Entity->dP.y, Entity->dP.x);
    }
#endif
}

enum traversable_search_flag
{
    TraversableSearch_Unoccupied = 0x1,
};
internal b32
GetClosestTraversable(sim_region *SimRegion, v3 FromP, traversable_reference *Result, u32 Flags = 0)
{
    TIMED_FUNCTION();

    b32 Found = false;

    r32 ClosestDistanceSq = Square(1000.0f);
    entity *TestEntity = SimRegion->Entities;
    for(u32 TestEntityIndex = 0; 
        TestEntityIndex < SimRegion->EntityCount;
        ++TestEntityIndex, ++TestEntity)
    {
        for(u32 PIndex = 0; PIndex < TestEntity->TraversableCount; ++PIndex)
        {
            entity_traversable_point P = GetSimSpaceTraversable(TestEntity, PIndex);
            if(!(Flags & TraversableSearch_Unoccupied) || (P.Occupier == 0))
            {
                v3 ToPoint = P.P - FromP;
                // TODO: What should this value be?
                //ToPoint.z = ClampAboveZero(AbsoluteValue(ToPoint.z) - 1.0f);

                r32 TestDSq = LengthSq(ToPoint);
                if(ClosestDistanceSq > TestDSq)
                {
                    // P.P;
                    Result->Entity.Ptr = TestEntity;
                    Result->Entity.Index = TestEntity->ID;
                    Result->Index = PIndex;
                    ClosestDistanceSq = TestDSq;
                    Found = true;
                }
            }
        }
    }

    if(!Found)
    {
        Result->Entity.Ptr = 0;
        Result->Index = 0;
    }

    return(Found);
}

internal b32
GetClosestTraversableAlongRay(sim_region *SimRegion, v3 FromP, v3 Dir, traversable_reference Skip,
                              traversable_reference *Result, u32 Flags = 0)
{
    TIMED_FUNCTION();

    b32 Found = false;

    for(u32 ProbeIndex = 0; ProbeIndex < 5; ++ProbeIndex)
    {
        v3 SampleP = FromP + 0.5f*(r32)ProbeIndex*Dir;
        if(GetClosestTraversable(SimRegion, SampleP, Result, Flags))
        {
            if(!IsEqual(Skip, *Result))
            {
                Found = true;
                break;
            }
        }
    }

    return(Found);
}

struct closest_entity
{
    entity *Entity;
    v3 Delta;
    r32 DistanceSq;
};
internal closest_entity
GetClosestEntityWithBrain(sim_region *SimRegion, v3 P, brain_type Type, r32 MaxRadius = 20.0f)
{
    closest_entity Result = {};
    Result.DistanceSq = Square(MaxRadius);

    entity *TestEntity = SimRegion->Entities;
    for(u32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity)
    {
        if(IsType(TestEntity->BrainSlot, Type))
        {
            v3 TestDelta = TestEntity->P - P;
            r32 TestDSq = LengthSq(TestDelta);
            if(Result.DistanceSq > TestDSq)
            {
                Result.Entity = TestEntity;
                Result.DistanceSq = TestDSq;
                Result.Delta = TestDelta;
            }
        }
    }

    return(Result);
}
