#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALISED INT32_MAX
#define TILES_PER_CHUNK 16

inline world_position
NullPosition(void)
{
    world_position Result = {};

    Result.ChunkX = TILE_CHUNK_UNINITIALISED;

    return(Result);
}

inline bool32
IsValid(world_position P)
{
    bool32 Result = (P.ChunkX != TILE_CHUNK_UNINITIALISED);
    return(Result);
}

inline bool32
IsCanonical(world *World, real32 TileRel)
{
    bool32 Result = ((TileRel >= -0.5f*World->ChunkSideInMeters) &&
                     (TileRel <= 0.5f*World->ChunkSideInMeters));
    return(Result);
}

inline bool32
IsCanonical(world *World, v2 Offset)
{
    bool32 Result = (IsCanonical(World, Offset.X) && IsCanonical(World, Offset.Y));

    return(Result);
}

inline bool32
AreInSameChunk(world *World, world_position *A, world_position *B)
{
    Assert(IsCanonical(World, A->Offset_));
    Assert(IsCanonical(World, B->Offset_));

    bool32 Result = ((A->ChunkX == B->ChunkX) &&
                     (A->ChunkY == B->ChunkY) &&
                     (A->ChunkZ == B->ChunkZ));

    return(Result);
}

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

internal void
InitialiseWorld(world *World, real32 TileSideInMeters)
{
    World->TileSideInMeters = TileSideInMeters;
    World->ChunkSideInMeters = (real32)TILES_PER_CHUNK*TileSideInMeters;
    World->FirstFree = 0;

    for(uint32_t ChunkIndex = 0;
        ChunkIndex < ArrayCount(World->ChunkHash);
        ++ChunkIndex)

    {
        World->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALISED;
        World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
    }
}

inline void
RecanonicaliseCoord(world *World, int32_t *Tile, real32 *TileRel)
{
    // NOTE: World is assumed to be toroidal topology, stepping off
    // one end brings you back to the other.
    int32_t Offset = RoundReal32ToInt32(*TileRel / World->ChunkSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*World->ChunkSideInMeters;

    Assert(IsCanonical(World, *TileRel));
}

inline world_position
MapIntoChunkSpace(world *World, world_position BasePos, v2 Offset)
{
    world_position Result = BasePos;

    Result.Offset_ += Offset;
    RecanonicaliseCoord(World, &Result.ChunkX, &Result.Offset_.X);
    RecanonicaliseCoord(World, &Result.ChunkY, &Result.Offset_.Y);

    return(Result);
}

inline world_position
ChunkPositionFromTilePosition(world *World, int32_t AbsTileX, int32_t AbsTileY, int32_t AbsTileZ)
{
    world_position Result = {};

    Result.ChunkX = AbsTileX / TILES_PER_CHUNK;
    Result.ChunkY = AbsTileY / TILES_PER_CHUNK;
    Result.ChunkZ = AbsTileZ / TILES_PER_CHUNK;

    if(AbsTileX < 0)
    {
        --Result.ChunkX;
    }
    if(AbsTileY < 0)
    {
        --Result.ChunkY;
    }
    if(AbsTileZ < 0)
    {
        --Result.ChunkZ;
    }

    Result.Offset_.X = (real32)((AbsTileX - TILES_PER_CHUNK/2) - (Result.ChunkX*TILES_PER_CHUNK)) * World->TileSideInMeters;
    Result.Offset_.Y = (real32)((AbsTileY - TILES_PER_CHUNK/2) - (Result.ChunkY*TILES_PER_CHUNK)) * World->TileSideInMeters;

    Assert(IsCanonical(World, Result.Offset_));

    return(Result);
}

inline world_difference
Subtract(world *World, world_position *A, world_position *B)
{
    world_difference Result;

    v2 dTileXY = {(real32)A->ChunkX - (real32)B->ChunkX,
                  (real32)A->ChunkY - (real32)B->ChunkY};
    real32 dTileZ = (real32)A->ChunkZ - (real32)B->ChunkZ;

    Result.dXY = World->ChunkSideInMeters*dTileXY + (A->Offset_ - B->Offset_);
    Result.dZ = World->ChunkSideInMeters*dTileZ;

    return(Result);
}

inline world_position
CentredChunkPoint(uint32_t ChunkX, uint32_t ChunkY, uint32_t ChunkZ)
{
    world_position Result = {};

    Result.ChunkX = ChunkX;
    Result.ChunkY = ChunkY;
    Result.ChunkZ = ChunkZ;

    return(Result);
}

inline void
ChangeEntityLocationRaw(memory_arena *Arena, world *World, uint32_t LowEntityIndex,
                        world_position *OldP, world_position *NewP)
{
    Assert(!OldP || IsValid(*OldP));
    Assert(!NewP || IsValid(*NewP));

    if(OldP && NewP && AreInSameChunk(World, OldP, NewP))
    {
    }
    else
    {
        if(OldP)
        {
            world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
            Assert(Chunk);
            if(Chunk)
            {
                bool32 NotFound = true;
                world_entity_block *FirstBlock = &Chunk->FirstBlock;
                for(world_entity_block *Block = FirstBlock; Block && NotFound; Block = Block->Next)
                {
                    for(uint32_t Index = 0; (Index < Block->EntityCount) && NotFound; ++Index)
                    {
                        if(Block->LowEntityIndex[Index] == LowEntityIndex)
                        {
                            Assert(FirstBlock->EntityCount > 0);
                            Block->LowEntityIndex[Index] = FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];
                            if(FirstBlock->EntityCount == 0)
                            {
                                if(FirstBlock->Next)
                                {
                                    world_entity_block *NextBlock = FirstBlock->Next;
                                    *FirstBlock = *NextBlock;

                                    NextBlock->Next = World->FirstFree;
                                    World->FirstFree = NextBlock;
                                }
                            }

                            NotFound = false;
                        }
                    }
                }
            }
        }

        if(NewP)
        {
            world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
            Assert(Chunk);
        
            world_entity_block *Block = &Chunk->FirstBlock;
            if(Block->EntityCount == ArrayCount(Block->LowEntityIndex))
            {
                world_entity_block *OldBlock = World->FirstFree;
                if(OldBlock)
                {
                    World->FirstFree = OldBlock->Next;
                }
                else
                {
                    OldBlock = PushStruct(Arena, world_entity_block);
                }

                *OldBlock = *Block;
                Block->Next = OldBlock;
                Block->EntityCount = 0;
            }

            Assert(Block->EntityCount < ArrayCount(Block->LowEntityIndex));
            Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
        }
    }
}

internal void
ChangeEntityLocation(memory_arena *Arena, world *World,
                     uint32_t LowEntityIndex, low_entity *LowEntity,
                     world_position *OldP, world_position *NewP)
{
    ChangeEntityLocationRaw(Arena, World, LowEntityIndex, OldP, NewP);
    if(NewP)
    {
        LowEntity->P = *NewP;
    }
    else
    {
        LowEntity->P = NullPosition();
    }
}
