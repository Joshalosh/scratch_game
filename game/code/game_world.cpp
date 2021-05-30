#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALISED INT32_MAX

inline world_chunk *
GetWorldChunk(world *World, int32_t ChunkX, int32_t ChunkY, int32_t ChunkZ,
             memory_arena *Arena = 0)
{
    Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkZ < TILE_CHUNK_SAFE_MARGIN);

    uint32_t HashValue = 19*ChunkX + 7*ChunkY + 3*ChunkZ;
    uint32_t HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
    Assert(HashSlot < ArrayCount(World->ChunkHash));

    world_chunk *Chunk = World->ChunkHash + HashSlot;
    do
    {
        if((ChunkX == Chunk->ChunkX) &&
           (ChunkY == Chunk->ChunkY) &&
           (ChunkZ == Chunk->ChunkZ))
        {
            break;
        }

        if(Arena && (Chunk->ChunkX != TILE_CHUNK_UNINITIALISED) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, world_chunk);
            Chunk = Chunk->NextInHash;
            Chunk->ChunkX = TILE_CHUNK_UNINITIALISED;
        }

        if(Arena && (Chunk->ChunkX == TILE_CHUNK_UNINITIALISED))
        {
            uint32_t TileCount = World->ChunkDim*World->ChunkDim;
            
            Chunk->ChunkX = ChunkX;
            Chunk->ChunkY = ChunkY;
            Chunk->ChunkZ = ChunkZ;

            Chunk->NextInHash = 0;

            break;
        }

        Chunk = Chunk->NextInHash;
    } while(Chunk);

    return(Chunk);
}

#if 0
inline world_chunk_position
GetChunkPositionFor(world *World, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    tile_chunk_position Result;

    Result.ChunkX = AbsTileX >> World->ChunkShift;
    Result.ChunkY = AbsTileY >> World->ChunkShift;
    Result.ChunkZ = AbsTileZ;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY = AbsTileY & World->ChunkMask;

    return(Result);
}
#endif


internal void
InitialiseWorld(world *World, real32 TileSideInMeters)
{
    World->ChunkShift = 4;
    World->ChunkMask = (1 << World->ChunkShift) - 1;
    World->ChunkDim = (1 << World->ChunkShift);
    World->TileSideInMeters = TileSideInMeters;

    for(uint32_t ChunkIndex = 0;
        ChunkIndex < ArrayCount(World->ChunkHash);
        ++ChunkIndex)

    {
        World->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALISED;
    }
}

//
// TODO: Maybe put these in a more positioning or geometry file.
//

inline void
RecanonicaliseCoord(world *World, int32_t *Tile, real32 *TileRel)
{
    // NOTE: World is assumed to be toroidal topology, stepping off
    // one end brings you back to the other.
    int32_t Offset = RoundReal32ToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*World->TileSideInMeters;

    Assert(*TileRel > -0.5f*World->TileSideInMeters);
    Assert(*TileRel < 0.5f*World->TileSideInMeters);
}

inline world_position
MapIntoTileSpace(world *World, world_position BasePos, v2 Offset)
{
    world_position Result = BasePos;

    Result.Offset_ += Offset;
    RecanonicaliseCoord(World, &Result.AbsTileX, &Result.Offset_.X);
    RecanonicaliseCoord(World, &Result.AbsTileY, &Result.Offset_.Y);

    return(Result);
}

inline bool32
AreOnSameTile(world_position *A, world_position *B)
{
    bool32 Result = ((A->AbsTileX == B->AbsTileX) &&
                     (A->AbsTileY == B->AbsTileY) &&
                     (A->AbsTileZ == B->AbsTileZ));

    return(Result);
}

inline world_difference
Subtract(world *World, world_position *A, world_position *B)
{
    world_difference Result;

    v2 dTileXY = {(real32)A->AbsTileX - (real32)B->AbsTileX,
                  (real32)A->AbsTileY - (real32)B->AbsTileY};
    real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;

    Result.dXY = World->TileSideInMeters*dTileXY + (A->Offset_ - B->Offset_);
    Result.dZ = World->TileSideInMeters*dTileZ;

    return(Result);
}

inline world_position
CentredTilePoint(uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position Result = {};

    Result.AbsTileX = AbsTileX;
    Result.AbsTileY = AbsTileY;
    Result.AbsTileZ = AbsTileZ;

    return(Result);
}

