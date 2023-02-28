#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALISED INT32_MAX

#define TILES_PER_CHUNK 8

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
IsCanonical(real32 ChunkDim, real32 TileRel)
{
    real32 Epsilon = 0.01f;
    bool32 Result = ((TileRel >= -(0.5f*ChunkDim + Epsilon)) &&
                     (TileRel <= (0.5f*ChunkDim + Epsilon)));
    return(Result);
}

inline bool32
IsCanonical(world *World, v3 Offset)
{
    bool32 Result = (IsCanonical(World->ChunkDimInMeters.x, Offset.x) &&
                     IsCanonical(World->ChunkDimInMeters.y, Offset.y) &&
                     IsCanonical(World->ChunkDimInMeters.z, Offset.z));

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
    TIMED_FUNCTION();

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
InitialiseWorld(world *World, v3 ChunkDimInMeters)
{
    World->ChunkDimInMeters = ChunkDimInMeters;
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
RecanonicaliseCoord(real32 ChunkDim, int32_t *Tile, real32 *TileRel)
{
    int32_t Offset = RoundReal32ToInt32(*TileRel / ChunkDim);
    *Tile += Offset;
    *TileRel -= Offset*ChunkDim;

    Assert(IsCanonical(ChunkDim, *TileRel));
}

inline world_position
MapIntoChunkSpace(world *World, world_position BasePos, v3 Offset)
{
    world_position Result = BasePos;

    Result.Offset_ += Offset;
    RecanonicaliseCoord(World->ChunkDimInMeters.x, &Result.ChunkX, &Result.Offset_.x);
    RecanonicaliseCoord(World->ChunkDimInMeters.y, &Result.ChunkY, &Result.Offset_.y);
    RecanonicaliseCoord(World->ChunkDimInMeters.z, &Result.ChunkZ, &Result.Offset_.z);

    return(Result);
}

inline v3
Subtract(world *World, world_position *A, world_position *B)
{
    v3 dTile = {(real32)A->ChunkX - (real32)B->ChunkX,
                (real32)A->ChunkY - (real32)B->ChunkY,
                (real32)A->ChunkZ - (real32)B->ChunkZ};

    v3 Result = Hadamard(World->ChunkDimInMeters, dTile) + (A->Offset_ - B->Offset_);

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

inline world_position
CentredChunkPoint(world_chunk *Chunk)
{
    world_position Result = CentredChunkPoint(Chunk->ChunkX, Chunk->ChunkY, Chunk->ChunkZ);

    return(Result);
}

inline void
ChangeEntityLocationRaw(memory_arena *Arena, world *World, uint32_t LowEntityIndex,
                        world_position *OldP, world_position *NewP)
{
    TIMED_FUNCTION();

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
                     world_position NewPInit)
{
    TIMED_FUNCTION();

    world_position *OldP = 0;
    world_position *NewP = 0;

    if(!IsSet(&LowEntity->Sim, EntityFlag_Nonspatial) && IsValid(LowEntity->P))
    {
        OldP = &LowEntity->P;
    }

    if(IsValid(NewPInit))
    {
        NewP = &NewPInit;
    }

    ChangeEntityLocationRaw(Arena, World, LowEntityIndex, OldP, NewP);
    if(NewP)
    {
        LowEntity->P = *NewP;
        ClearFlags(&LowEntity->Sim, EntityFlag_Nonspatial);
    }
    else
    {
        LowEntity->P = NullPosition();
        AddFlags(&LowEntity->Sim, EntityFlag_Nonspatial);
    }
}
