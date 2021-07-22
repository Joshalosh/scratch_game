internal sim_region *
BeginSim()
{
    world_position MinChunkP = MapIntoChunkSpace(World, NewCameraP, GetMinCorner(CameraBounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, NewCameraP, GetMaxCorner(CameraBounds));
    for(int32_t ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
    {
        for(int32_t ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
        {
            world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, NewCameraP.ChunkZ);
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
                        if(Low->HighEntityIndex == 0)
                        {
                            v2 CameraSpaceP = GetCameraSpaceP(GameState, Low);
                            if(IsInRectangle(CameraBounds, CameraSpaceP))
                            {
                                MakeEntityHighFrequency(GameState, Low, LowEntityIndex, CameraSpaceP);
                            }
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
