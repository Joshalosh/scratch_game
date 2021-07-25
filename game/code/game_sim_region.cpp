internal sim_entity *
AddEntity(sim_region *SimRegion)
{
    sim_entity *Entity = 0;

    if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
    {
        Entity = SimRegion->Entities[SimRegion->EntityCount++];
        Entity = {};
    }
    else
    {
        InvalidCodePath;
    }

    return(Entity);
}

inline v2
GetSimSpaceP(sim_region *SimRegion, stored_entity *Stored)
{
    world_difference Diff = Subtract(SimRegion->World, &Stored->P, &SimRegion->Origin);
    v2 Result = Diff.dXY;

    return(Result);
}

internal sim_entity *
AddEntity(sim_region *SimRegion, stored_entity *Source, v2 *SimP)
{
    sim_entity *Dest = AddEntity(SimRegion);
    if(Dest)
    {
        if(SimP)
        {
            Dest->P = SimP;
        }
        else
        {
            Dest->P = GetSimSpaceP(SimRegion, Low);
        }
    }
}

internal sim_region *
BeginSim(world *World, world_position Origin, rectangle2 Bounds)
{
    SimRegion->World  = World;
    SimRegion->Origin = Origin;
    SimRegion->Bounds = Bounds;

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
                           AddEntity(SimRegion, Low, &SimSpaceP);
                        }
                    }
                }
            }
        }
    }
}

internal void
EndSim(sim_region *Region)
{
    entity *Entities = Region->Entities;
    for(uint32_t EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
    {
    }
}
