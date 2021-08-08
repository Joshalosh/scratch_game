internal sim_entity_hash *
GetHashFromStorageIndex(sim_region *SimRegion, uint32_t StorageIndex)
{
    Assert(StorageIndex);

    sim_entity_hash *Result = 0;

    uint32_t HashValue = StorageIndex;
    for(uint32_t Offset = 0; Offset < ArrayCount(SimRegion->Hash); ++Offset)
    {
        uint32_t HashMask  = (ArrayCount(SimRegion->Hash) - 1);
        uint32_t HashIndex = ((HashValue + Offset) & HashMask);
        sim_entity_hash *Entry = SimRegion->Hash + HashIndex;
        if((Entry->Index == 0) || (Entry->Index == StorageIndex))
        {
            Result = Entry;
            break;
        }
    }

    return(Result);
}

internal void
MapStorageIndexToEntity(sim_region *SimRegion, uint32_t StorageIndex, sim_entity *Entity)
{
    sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
    Assert((Entry->Index == 0) || (Entry->Index == StorageIndex));
    Entry->Index = StorageIndex;
    Entry->Ptr = Entity;
}

inline sim_entity *
GetEntityByStorageIndex(sim_region *SimRegion, uint32_t StorageIndex)
{
    sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
    sim_entity *Result = Entry->Ptr;
    return(Result);
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32_t StorageIndex, low_entity *Source);
inline void
LoadEntityReference(game_state *GameState, sim_region *SimRegion, entity_reference *Ref)
{
    if(Ref->Index)
    {
        sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, Ref->Index);
        if(Entry->Ptr == 0)
        {
            Entry->Index = Ref->Index;
            Entry->Ptr = AddEntity(GameState, SimRegion, Ref->Index, GetLowEntity(GameState, Ref->Index));
        }

        Ref->Ptr = Entry->Ptr;
    }
}

inline void
StoreEntityReference(entity_reference *Ref)
{
    if(Ref->Ptr != 0)
    {
        Ref->Index = Ref->Ptr->StorageIndex;
    }
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32_t StorageIndex, low_entity *Source)
{
    Assert(StorageIndex);
    sim_entity *Entity = 0;

    if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
    {
        Entity = SimRegion->Entities + SimRegion->EntityCount++;
        MapStorageIndexToEntity(SimRegion, StorageIndex, Entity);

        if(Source)
        {
            *Entity = Source->Sim;
            LoadEntityReference(GameState, SimRegion, &Entity->Sword);
        }

        Entity->StorageIndex = StorageIndex;
    }
    else
    {
        InvalidCodePath;
    }

    return(Entity);
}

inline v2
GetSimSpaceP(sim_region *SimRegion, low_entity *Stored)
{
    world_difference Diff = Subtract(SimRegion->World, &Stored->P, &SimRegion->Origin);
    v2 Result = Diff.dXY;

    return(Result);
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32_t LowEntityIndex, low_entity *Source, v2 *SimP)
{
    sim_entity *Dest = AddEntity(SimRegion);
    if(Dest)
    {
        if(SimP)
        {
            Dest->P = *SimP;
        }
        else
        {
            Dest->P = GetSimSpaceP(SimRegion, Source);
        }
    }

    return(Dest);
}

internal sim_region *
BeginSim(memory_arena *SimArena, game_state *GameState, world *World, world_position Origin, rectangle2 Bounds)
{
    sim_region *SimRegion = PushStruct(SimArena, sim_region);
    ZeroStruct(SimRegion->Hash);

    SimRegion->World  = World;
    SimRegion->Origin = Origin;
    SimRegion->Bounds = Bounds;
    SimRegion->MaxEntityCount = 4096;
    SimRegion->EntityCount = 0;
    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);

    world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
    for(int32_t ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
    {
        for(int32_t ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
        {
            world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, SimRegion->Origin.ChunkZ);
            if(Chunk)
            {
                for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next)
                {
                    for(uint32_t EntityIndexIndex = 0;
                        EntityIndexIndex < Block->EntityCount;
                        ++EntityIndexIndex)
                    {
                        uint32_t LowEntityIndex = Block->LowEntityIndex[EntityIndexIndex];
                        low_entity *Low = GameState->LowEntities + LowEntityIndex;
                        v2 SimSpaceP = GetSimSpaceP(SimRegion, Low);
                        if(IsInRectangle(SimRegion->Bounds, SimSpaceP))
                        {
                           AddEntity(GameState, SimRegion, LowEntityIndex, Low, &SimSpaceP);
                        }
                    }
                }
            }
        }
    }

    return(SimRegion);
}

internal void
EndSim(sim_region *SimRegion, game_state *GameState)
{
    sim_entity *Entities = SimRegion->Entities;
    for(uint32_t EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex, ++Entity)
    {
        low_entity *Stored = GameState->LowEntities + Entity->StorageIndex;

        Stored->Sim = *Entity;
        StoredEntityReference(&Stored->Sim.Sword);

        world_position NewP = MapIntoChunkSpace(GameState->World, SimRegion->Origin, Entity->P);
        ChangeEntityLocation(&GameState->WorldArena, GameState->World. Entity->StorageIndex,
                             Stored, &Stored-P, &NewP);

        if(Entity->StorageIndex == GameState->CameraFollowingEntityIndex)
        {
            world_position NewCameraP = GameState->CameraP;

            NewCameraP.ChunkZ = Stored->P.ChunkZ;

#if 0
            if(CameraFollowingEntity->P.X > (9.0f*World->TileSideInMeters))
            {
                NewCameraP.AbsTileX += 17;
            }
            if(CameraFollowingEntity->P.X < -(9.0f*World->TileSideInMeters))
            {
                NewCameraP.AbsTileX -= 17;
            }
            if(CameraFollowingEntity->P.Y > (5.0f*World->TileSideInMeters))
            {
                NewCameraP.AbsTileY += 9;
            }
            if(CameraFollowingEntity->P.Y < -(5.0f*World->TileSideInMeters))
            {
                NewCameraP.AbsTileY -= 9;
            }
#else
            NewCameraP = Stored->P;
#endif
        }
    }
}

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

internal void
MoveEntity(sim_region *SimRegion, sim_entity *Entity, real32 dt, move_spec *MoveSpec, v2 ddP)
{
    world *World = SimRegion->World;

    if(MoveSpec->UnitMaxAccelVector)
    {
        real32 ddPLength = LengthSq(ddP);
        if(ddPLength > 1.0f)
        {
            ddP *= (1.0f / SquareRoot(ddPLength));
        }
    }

    ddP *= MoveSpec->Speed;
    
    //TODO: Diagonal will be faster! Fix with vectors
    ddP += -MoveSpec->Drag*Entity->dP;

    v2 OldPlayerP  = Entity->P;
    v2 PlayerDelta = (0.5f*ddP*Square(dt) +
                      Entity->dP*dt);
    Entity->dP = ddP*dt + Entity->dP;
    v2 NewPlayerP = OldPlayerP + PlayerDelta;

    for(uint32_t Iteration = 0; Iteration < 4; ++Iteration)
    {
        real32 tMin = 1.0f;
        v2 WallNormal = {};
        sim_entity *HitEntity = 0;

        v2 DesiredPosition = Entity->P + PlayerDelta;

        if(Entity.Collides)
        {
            for(uint32_t TestHighEntityIndex = 0; TestHighEntityIndex < SimRegion->EntityCount; ++TestHighEntityIndex)
            {
                sim_entity *TestEntity = SimRegion->Entities + TestHighEntityIndex;
                if(Entity != TestEntity)
                {
                    if(TestEntity.Collides)
                    {
                        real32 DiameterW = TestEntity.Width + Entity.Width;
                        real32 DiameterH = TestEntity.Height + Entity.Height;

                        v2 MinCorner = -0.5f*V2(DiameterW, DiameterH);
                        v2 MaxCorner = 0.5f*V2(DiameterW, DiameterH);

                        v2 Rel = Entity->P - TestEntity->P;

                        if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                                    &tMin, MinCorner.Y, MaxCorner.Y))
                        {
                            WallNormal = V2(-1, 0);
                            HitEntity = TestEntity;
                        }

                        if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                                    &tMin, MinCorner.Y, MaxCorner.Y))
                        {
                            WallNormal = V2(1, 0);
                            HitEntity = TestEntity;
                        }

                        if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                                    &tMin, MinCorner.X, MaxCorner.X))
                        {
                            WallNormal = V2(0, -1);
                            HitEntity = TestEntity;
                        }

                        if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                                    &tMin, MinCorner.X, MaxCorner.X))
                        {
                            WallNormal = V2(0, 1);
                            HitEntity = TestEntity;
                        }
                    }
                }
            }
        }

        Entity->P += tMin*PlayerDelta;
        if(HitEntity)
        {
            Entity->dP = Entity->dP - 1*Inner(Entity->dP, WallNormal)*WallNormal;
            PlayerDelta = DesiredPosition - Entity->P;
            PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;

            //Entity->AbsTileZ += HitLow->dAbsTileZ;
        }
        else
        {
            break;
        }
    }

    if((Entity->dP.X == 0.0f) && (Entity->dP.Y == 0.0f))
    {
        // NOTE: Leave facing direction as whatever it was.
    }
    else if(AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y))
    {
        if(Entity->dP.X > 0)
        {
            Entity->FacingDirection = 0;
        }
        else
        {
            Entity->FacingDirection = 2;
        }
    }
    else
    {
        if(Entity->dP.Y > 0)
        {
            Entity->FacingDirection = 1;
        }
        else
        {
            Entity->FacingDirection = 3;
        }
    }
}

