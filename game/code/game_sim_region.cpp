inline void
LoadEntityReference(sim_region *SimRegion, entity_reference *Ref)
{
    if(Ref->Index != 0)
    {
        sim_entity *Ptr = GetSimEntityFromIndex(Ref->Index);
        Assert(Ptr);

        Ref->Ptr = Ptr;
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
AddEntity(sim_region *SimRegion, uint32_t StorageIndex, low_entity *Source)
{
    sim_entity *Entity = 0;

    if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
    {
        Entity = SimRegion->Entities + SimRegion->EntityCount++;

        if(Source)
        {
            *Entity = Source->Sim;
            Entity->StorageIndex = StorageIndex;
        }
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
AddEntity(sim_region *SimRegion, uint32_t LowEntityIndex, low_entity *Source, v2 *SimP)
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
}

internal sim_region *
BeginSim(memory_arena *SimArena, game_state *GameState, world *World, world_position Origin, rectangle2 Bounds)
{
    sim_region *SimRegion = PushStruct(SimArena, sim_region);

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
                           AddEntity(SimRegion, LowEntityIndex, Low, &SimSpaceP);
                        }
                    }
                }
            }
        }
    }
}

internal void
EndSim(sim_region *Region, game_state *GameState)
{
    sim_entity *Entities = Region->Entities;
    for(uint32_t EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
    {
        low_entity *Stored = GameState->LowEntities + Entity->StorageIndex;

        Stored->Sim = *Entity;
        StoredEntityReference(&Stored->Sim->Sword);

        world_position NewP = MapIntoChunkSpace(GameState->World, Region->Origin, Entity->P);
        ChangeEntityLocation(&GameState->WorldArena, GameState->World. Entity->StorageIndex,
                             Stored, &Stored-P, &NewP);

        entity CameraFollowingEntity = ForceEntityIntoHigh(GameState, GameState->CameraFollowingEntityIndex);
        if(CameraFollowingEntity.High)
        {
            world_position NewCameraP = GameState->CameraP;

            NewCameraP.ChunkZ = CameraFollowingEntity.Low->P.ChunkZ;

#if 0
            if(CameraFollowingEntity.High->P.X > (9.0f*World->TileSideInMeters))
            {
                NewCameraP.AbsTileX += 17;
            }
            if(CameraFollowingEntity.High->P.X < -(9.0f*World->TileSideInMeters))
            {
                NewCameraP.AbsTileX -= 17;
            }
            if(CameraFollowingEntity.High->P.Y > (5.0f*World->TileSideInMeters))
            {
                NewCameraP.AbsTileY += 9;
            }
            if(CameraFollowingEntity.High->P.Y < -(5.0f*World->TileSideInMeters))
            {
                NewCameraP.AbsTileY -= 9;
            }
#else
            NewCameraP = CameraFollowingEntity.Low->P;
#endif
            SetCamera(GameState, NewCameraP);
        }
    }
}
