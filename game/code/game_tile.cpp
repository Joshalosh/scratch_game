#define TILE_CHUNK_SAFE_MARGIN 256

inline tile_chunk *
GetTileChunk(tile_map *TileMap, uint32_t TileChunkX, uint32_t TileChunkY, uint32_t TileChunkZ,
             memory_arena *Arena = 0)
{
    Assert(TileChunkX > -TILE_CHUNK_SAFE_MARGIN);
    Assert(TileChunkY > -TILE_CHUNK_SAFE_MARGIN);
    Assert(TileChunkZ > -TILE_CHUNK_SAFE_MARGIN);
    Assert(TileChunkX < TILE_CHUNK_SAFE_MARGIN);
    Assert(TileChunkY < TILE_CHUNK_SAFE_MARGIN);
    Assert(TileChunkZ < TILE_CHUNK_SAFE_MARGIN);

    uint32_t HashValue = 19*TileChunkX + 7*TileChunkY + 3*TileChunkZ;
    uint32_t HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
    Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));

    tile_chunk *Chunk = TileMap->TileChunkHash + HashSlot;
    do
    {
        if((TileChunkX == Chunk->TileChunkX) &&
           (TileChunkY == Chunk->TileChunkY) &&
           (TileChunkZ == Chunk->TileChunkZ))
        {
            break;
        }

        if(Arena && (Chunk->TileChunkX != TILE_CHUNK_UNINITIALISED) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, tile_chunk);
            Chunk = Chunk->NextInHash;
            Chunk->TileChunkX = TILE_CHUNK_UNINITIALISED;
        }

        if(Arena && (Chunk->TileChunkX == TILE_CHUNK_UNINITIALISED))
        {
            uint32_t TileCount = TileMap->ChunkDim*TileMap->ChunkDim;
            
            Chunk->TileChunkX = TileChunkX;
            Chunk->TileChunkY = TileChunkY;
            Chunk->TileChunkZ = TileChunkZ;

            Chunk->Tiles = PushArray(Arena, TileCount, uint32_t);
            for(uint32_t TileIndex = 0; TileIndex < TileCount; ++TileIndex)
            {
                Chunk->Tiles[TileIndex] = 1;
            }

            Chunk->NextInHash = 0;

            break;
        }

        Chunk = Chunk->NextInHash;
    } while(Chunk);

    return(Chunk);
}

inline uint32_t
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TileX, uint32_t TileY)
{
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);

    uint32_t TileChunkValue = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];
    return(TileChunkValue);
}

inline void
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TileX, uint32_t TileY,
                      uint32_t TileValue)
{
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);

    TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;
}

inline void
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk,
             uint32_t TestTileX, uint32_t TestTileY, uint32_t TileValue)
{
    if(TileChunk && TileChunk->Tiles)
    {
        SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
    }
}

inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.TileChunkZ = AbsTileZ;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY = AbsTileY & TileMap->ChunkMask;

    return(Result);
}

inline uint32_t
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TestTileX, uint32_t TestTileY)
{
    uint32_t TileChunkValue = 0;

    if(TileChunk && TileChunk->Tiles)
    {
        TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    }

    return(TileChunkValue);
}

inline uint32_t
GetTileValue(tile_map *TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
    uint32_t TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

    return(TileChunkValue);
}

inline uint32_t
GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
    uint32_t TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);

    return(TileChunkValue);
}

internal bool32
IsTileValueEmpty(uint32_t TileValue)
{
    bool32 Empty = ((TileValue == 1) ||
                    (TileValue == 3) ||
                    (TileValue == 4));

    return(Empty);
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
    uint32_t TileChunkValue = GetTileValue(TileMap, Pos);
    bool32 Empty = IsTileValueEmpty(TileChunkValue);

    return(Empty);
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap,
             uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ,
             uint32_t TileValue)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY,
                                         ChunkPos.TileChunkZ, Arena);

    SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}

internal void
InitialiseTileMap(tile_map *TileMap, real32 TileSideInMeters)
{
    TileMap->ChunkShift = 4;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = (1 << TileMap->ChunkShift);
    TileMap->TileSideInMeters = TileSideInMeters;

    for(uint32_t TileChunkIndex = 0;
        TileChunkIndex < ArrayCount(TileMap->TileChunkHash);
        ++TileChunkIndex)

    {
        TileMap->TileChunkHash[TileChunkIndex].TileChunkX = TILE_CHUNK_UNINITIALISED;
    }
}

//
// TODO: Maybe put these in a more positioning or geometry file.
//

inline void
RecanonicaliseCoord(tile_map *TileMap, uint32_t *Tile, real32 *TileRel)
{
    // NOTE: TileMap is assumed to be toroidal topology, stepping off
    // one end brings you back to the other.
    int32_t Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*TileMap->TileSideInMeters;

    Assert(*TileRel > -0.5f*TileMap->TileSideInMeters);
    Assert(*TileRel < 0.5f*TileMap->TileSideInMeters);
}

inline tile_map_position
MapIntoTileSpace(tile_map *TileMap, tile_map_position BasePos, v2 Offset)
{
    tile_map_position Result = BasePos;

    Result.Offset_ += Offset;
    RecanonicaliseCoord(TileMap, &Result.AbsTileX, &Result.Offset_.X);
    RecanonicaliseCoord(TileMap, &Result.AbsTileY, &Result.Offset_.Y);

    return(Result);
}

inline bool32
AreOnSameTile(tile_map_position *A, tile_map_position *B)
{
    bool32 Result = ((A->AbsTileX == B->AbsTileX) &&
                     (A->AbsTileY == B->AbsTileY) &&
                     (A->AbsTileZ == B->AbsTileZ));

    return(Result);
}

inline tile_map_difference
Subtract(tile_map *TileMap, tile_map_position *A, tile_map_position *B)
{
    tile_map_difference Result;

    v2 dTileXY = {(real32)A->AbsTileX - (real32)B->AbsTileX,
                  (real32)A->AbsTileY - (real32)B->AbsTileY};
    real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;

    Result.dXY = TileMap->TileSideInMeters*dTileXY + (A->Offset_ - B->Offset_);
    Result.dZ = TileMap->TileSideInMeters*dTileZ;

    return(Result);
}

inline tile_map_position
CentredTilePoint(uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    tile_map_position Result = {};

    Result.AbsTileX = AbsTileX;
    Result.AbsTileY = AbsTileY;
    Result.AbsTileZ = AbsTileZ;

    return(Result);
}
