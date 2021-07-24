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

internal sim_region *
BeginSim(world_position RegionCentre, rectangle2 RegionBounds)
{
    world_position MinChunkP = MapIntoChunkSpace(World, RegionCentre, GetMinCorner(RegionBounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, RegionCentre, GetMaxCorner(RegionBounds));
    for(int32_t ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
    {
        for(int32_t ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
        {
            world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, RegionCentre.ChunkZ);
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
                        v2 CameraSpaceP = GetCameraSpaceP(GameState, Low);
                        if(IsInRectangle(RegionBounds, CameraSpaceP))
                        {
                           AddEntity(SimRegion, Low, CameraSpaceP);
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
