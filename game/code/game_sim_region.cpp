
inline entity_traversable_point
GetSimSpaceTraversable(entity *Entity, u32 Index)
{
    Assert(Index < Entity->Collision->TraversableCount);
    entity_traversable_point Result = Entity->Collision->Traversables[Index];

    // TODO: This wants to be rotated eventuially
    Result.P += Entity->P;

    return(Result);
}

internal entity_hash *
GetHashFromID(sim_region *SimRegion, entity_id StorageIndex)
{
    Assert(StorageIndex.Value);

    entity_hash *Result = 0;

    uint32_t HashValue = StorageIndex.Value;
    for(uint32_t Offset = 0; Offset < ArrayCount(SimRegion->Hash); ++Offset)
    {
        uint32_t HashMask  = (ArrayCount(SimRegion->Hash) - 1);
        uint32_t HashIndex = ((HashValue + Offset) & HashMask);
        entity_hash *Entry = SimRegion->Hash + HashIndex;
        if((Entry->Index.Value == 0) || (Entry->Index.Value == StorageIndex.Value))
        {
            Result = Entry;
            break;
        }
    }

    return(Result);
}

inline entity *
GetEntityByID(sim_region *SimRegion, entity_id ID)
{
    entity_hash *Entry = GetHashFromID(SimRegion, ID);
    entity *Result = Entry->Ptr;
    return(Result);
}

inline void
LoadEntityReference(sim_region *SimRegion, entity_reference *Ref)
{
    if(Ref->Index.Value)
    {
        entity_hash *Entry = GetHashFromID(SimRegion, Ref->Index);
        Ref->Ptr = Entry ? Entry->Ptr : 0;
    }
}

inline void
StoreEntityReference(entity_reference *Ref)
{
    if(Ref->Ptr != 0)
    {
        Ref->Index = Ref->Ptr->ID;
    }
}

internal entity *
AddEntityRaw(game_mode_world *WorldMode, sim_region *SimRegion, entity_id ID, entity *Source)
{
    TIMED_FUNCTION();

    Assert(ID.Value);
    entity *Entity = 0;

    return(Entity);
}

inline bool32
EntityOverlapsRectangle(v3 P, entity_collision_volume Volume, rectangle3 Rect)
{
    rectangle3 Grown = AddRadiusTo(Rect, 0.5f*Volume.Dim);
    bool32 Result = IsInRectangle(Grown, P + Volume.OffsetP);
    return(Result);
}

internal void
AddEntity(game_mode_world *WorldMode, sim_region *SimRegion, entity *Source, v3 ChunkDelta)
{
    entity_id ID = Source->ID;

    entity_hash *Entry = GetHashFromID(SimRegion, ID);
    Assert(Entry->Ptr == 0);

    if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
    {
        entity *Dest = SimRegion->Entities + SimRegion->EntityCount++;

        Entry->Index = ID;
        Entry->Ptr = Dest;

        if(Source)
        {
            // TODO: This should really be a decompression step, not a copy.
            *Dest = *Source;
        }

        Dest->ID = ID;
        Dest->P += ChunkDelta;
        Dest->MovementFrom += ChunkDelta;
        Dest->MovementTo += ChunkDelta;

        Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume, SimRegion->UpdatableBounds);
    }
    else
    {
        InvalidCodePath;
    }
}

internal void
ConnectEntityPointers(sim_region *SimRegion)
{
    for(u32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
    {
        entity *Entity = SimRegion->Entities + EntityIndex;
        LoadEntityReference(SimRegion, &Entity->Head);
    }
}

internal sim_region *
BeginSim(memory_arena *SimArena, game_mode_world *WorldMode, world *World, world_position Origin, rectangle3 Bounds, real32 dt)
{
    TIMED_FUNCTION();

    sim_region *SimRegion = PushStruct(SimArena, sim_region);

    // TODO Try to make these get enforced more rigorously
    SimRegion->MaxEntityRadius = 5.0f;
    SimRegion->MaxEntityVelocity = 30.0f;
    real32 UpdateSafetyMargin = SimRegion->MaxEntityRadius + dt*SimRegion->MaxEntityVelocity;
    real32 UpdateSafetyMarginZ = 1.0f;

    SimRegion->World = World;
    SimRegion->Origin = Origin;
    SimRegion->UpdatableBounds = AddRadiusTo(Bounds, V3(SimRegion->MaxEntityRadius,
                                                        SimRegion->MaxEntityRadius,
                                                        0.0f));
    SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds,
                                    V3(UpdateSafetyMargin, UpdateSafetyMargin, UpdateSafetyMarginZ));

    // TODO: Need to be more specific about entity counts.
    SimRegion->MaxEntityCount = 4096;
    SimRegion->EntityCount = 0;
    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, entity);

    world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));

    for(int32_t ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
    {
        for(int32_t ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
        {
            for(int32_t ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
            {
                world_chunk *Chunk = RemoveWorldChunk(World, ChunkX, ChunkY, ChunkZ);
                if(Chunk)
                {
                    world_position ChunkPosition = {ChunkX, ChunkY, ChunkZ};
                    v3 ChunkDelta = Subtract(SimRegion->World, &ChunkPosition, &SimRegion->Origin);
                    world_entity_block *Block = Chunk->FirstBlock;
                    while(Block)
                    {
                        for(uint32_t EntityIndex = 0;
                            EntityIndex < Block->EntityCount;
                            ++EntityIndex)
                        {
                            entity *Low = (entity *)Block->EntityData + EntityIndex;
                            v3 SimSpaceP = Low->P + ChunkDelta;
                            if(EntityOverlapsRectangle(SimSpaceP, Low->Collision->TotalVolume, SimRegion->Bounds))
                            {
                                AddEntity(WorldMode, SimRegion, Low, ChunkDelta);
                            }
                        }

                        world_entity_block *NextBlock = Block->Next;
                        AddBlockToFreeList(World, Block);
                        Block = NextBlock;
                    }

                    AddChunkToFreeList(World, Chunk);
                }
            }
        }
    }

    ConnectEntityPointers(SimRegion);

    return(SimRegion);
}

inline void
DeleteEntity(sim_region *Region, entity *Entity)
{
    Entity->Flags |= EntityFlag_Deleted;
}

internal void
EndSim(sim_region *Region, game_mode_world *WorldMode)
{
    TIMED_FUNCTION();

    world *World = WorldMode->World;

    entity *Entity = Region->Entities;
    for(uint32_t EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
    {
        if(!(Entity->Flags & EntityFlag_Deleted))
        {
            world_position EntityP = MapIntoChunkSpace(World, Region->Origin, Entity->P);
            world_position ChunkP = EntityP;
            ChunkP.Offset_ = V3(0, 0, 0);

            v3 ChunkDelta = -Subtract(Region->World, &ChunkP, &Region->Origin);

            // TODO: Save state back to the stored entity, once high Entities
            // do state decompression, etc

            if(Entity->ID.Value == WorldMode->CameraFollowingEntityIndex.Value)
            {
                world_position NewCameraP = WorldMode->CameraP;

                NewCameraP.ChunkZ = EntityP.ChunkZ;

                if(Global_Renderer_Camera_RoomBased)
                {
                    if(Entity->P.x > (9.0f))
                    {
                        NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 0.0f, 0.0f));
                    }
                    if(Entity->P.x < -(9.0f))
                    {
                        NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(-18.0f, 0.0f, 0.0f));
                    }
                    if(Entity->P.y > (5.0f))
                    {
                        NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 10.0f, 0.0f));
                    }
                    if(Entity->P.y < -(5.0f))
                    {
                        NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(0.0f, -10.0f, 0.0f));
                    }
                }
                else
                {
    //            real32 CamZOffset = NewCameraP.Offset_.z;
                    NewCameraP = EntityP;
    //            NewCameraP.Offset_.z = CamZOffset;
                }

                WorldMode->CameraP = NewCameraP;
            }

            Entity->P += ChunkDelta;
            Entity->MovementFrom += ChunkDelta;
            Entity->MovementTo += ChunkDelta;
            StoreEntityReference(&Entity->Head);
            PackEntityIntoWorld(World, Entity, EntityP);
        }
    }
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
CanCollide(game_mode_world *WorldMode, entity *A, entity *B)
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

            // TODO Implement a better hash function.
            uint32_t HashBucket = A->ID.Value & (ArrayCount(WorldMode->CollisionRuleHash) - 1);
            for(pairwise_collision_rule *Rule = WorldMode->CollisionRuleHash[HashBucket];
                Rule;
                Rule = Rule->NextInHash)
            {
                if((Rule->IDA == A->ID.Value) &&
                   (Rule->IDB == B->ID.Value))
                {
                    Result = Rule->CanCollide;
                    break;
                }
            }
        }
    }

    return(Result);
}

internal bool32
HandleCollision(game_mode_world *WorldMode, entity *A, entity *B)
{
    bool32 StopsOnCollision = true;

    if(A->Type > B->Type)
    {
        entity *Temp = A;
        A = B;
        B = Temp;
    }

    return(StopsOnCollision);
}

internal bool32
CanOverlap(game_mode_world *WorldMode, entity *Mover, entity *Region)
{
    bool32 Result = false;

    if(Mover != Region)
    {
        if(Region->Type == EntityType_Stairwell)
        {
            Result = true;
        }
    }

    return(Result);
}

internal bool32
SpeculativeCollide(entity *Mover, entity *Region, v3 TestP)
{
    TIMED_FUNCTION();

    bool32 Result = true;

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

    return(Result);
}

internal bool32
EntitiesOverlap(entity *Entity, entity *TestEntity, v3 Epsilon = V3(0, 0, 0))
{
    TIMED_FUNCTION();

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

internal void
MoveEntity(game_mode_world *WorldMode, sim_region *SimRegion, entity *Entity, real32 dt, move_spec *MoveSpec, v3 ddP)
{
    TIMED_FUNCTION();

    world *World = SimRegion->World;

    if(Entity->Type == EntityType_HeroHead)
    {
        int BreakHere = 5;
    }

    if(MoveSpec->UnitMaxAccelVector)
    {
        real32 ddPLength = LengthSq(ddP);
        if(ddPLength > 1.0f)
        {
            ddP *= (1.0f / SquareRoot(ddPLength));
        }
    }

    ddP *= MoveSpec->Speed;

    v3 Drag = -MoveSpec->Drag*Entity->dP;
    Drag.z = 0.0f;
    ddP += Drag;

    v3 PlayerDelta = (0.5f*ddP*Square(dt) + Entity->dP*dt);
    Entity->dP = ddP*dt + Entity->dP;
    // TODO Upgrade physical motion routinges to handle capping
    // the maximum velocity?
    Assert(LengthSq(Entity->dP) <= Square(SimRegion->MaxEntityVelocity));

    real32 DistanceRemaining = Entity->DistanceLimit;
    if(DistanceRemaining == 0.0f)
    {
        // TODO: Maybe formalise this number.
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

                if(CanCollide(WorldMode, Entity, TestEntity))
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

                bool32 StopsOnCollision = HandleCollision(WorldMode, Entity, HitEntity);
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
