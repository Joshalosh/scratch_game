#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILES_PER_CHUNK 8

#define TILE_CHUNK_UNINITIALISED INT32_MAX

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

inline void
ClearWorldEntityBlock(world_entity_block *Block)
{
    Block->EntityCount = 0;
    Block->Next = 0;
    Block->EntityDataSize = 0;
}

inline world_chunk **
GetWorldChunkInternal(world *World, int32_t ChunkX, int32_t ChunkY, int32_t ChunkZ)
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

    world_chunk **Chunk = &World->ChunkHash[HashSlot];
    while(*Chunk &&
          !((ChunkX == (*Chunk)->ChunkX) &&
            (ChunkY == (*Chunk)->ChunkY) &&
            (ChunkZ == (*Chunk)->ChunkZ)))
    {
        Chunk = &(*Chunk)->NextInHash;
    }

    return(Chunk);
}

inline world_chunk *
GetWorldChunk(world *World, s32 ChunkX, s32 ChunkY, s32 ChunkZ, memory_arena *Arena = 0)
{
    world_chunk **ChunkPtr = GetWorldChunkInternal(World, ChunkX, ChunkY, ChunkZ);
    world_chunk *Result = *ChunkPtr;
    if(!Result && Arena)
    {
        Result = PushStruct(Arena, world_chunk, NoClear());
        Result->FirstBlock = 0;
        Result->ChunkX = ChunkX;
        Result->ChunkY = ChunkY;
        Result->ChunkZ = ChunkZ;

        Result->NextInHash = *ChunkPtr;
        *ChunkPtr = Result;
    }

    return(Result);
}

internal world_chunk *
RemoveWorldChunk(world *World, s32 ChunkX, s32 ChunkY, s32 ChunkZ)
{
    world_chunk **ChunkPtr = GetWorldChunkInternal(World, ChunkX, ChunkY, ChunkZ);
    world_chunk *Result = *ChunkPtr;
    if(Result)
    {
        *ChunkPtr = Result->NextInHash;
    }

    return(Result);
}

internal world *
CreateWorld(v3 ChunkDimInMeters, memory_arena *ParentArena)
{
    world *World = PushStruct(ParentArena, world);

    World->ChunkDimInMeters = ChunkDimInMeters;
    World->FirstFree = 0;
    SubArena(&World->Arena, ParentArena, GetArenaSizeRemaining(ParentArena), NoClear());

    return(World);
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

inline b32
HasRoomFor(world_entity_block *Block, u32 Size)
{
    b32 Result = ((Block->EntityDataSize + Size) <= sizeof(Block->EntityData));
    return(Result);
}

inline void
PackEntityReference(entity_reference *Ref)
{
    if(Ref->Ptr != 0)
    {
        Ref->Index = Ref->Ptr->ID;
    }
}

inline void
PackTraversableReference(traversable_reference *Ref)
{
    PackEntityReference(&Ref->Entity);
}

internal void
PackEntityIntoChunk(world *World, entity *Source, world_chunk *Chunk)
{
    u32 PackSize = sizeof(*Source);

    if(!Chunk->FirstBlock || !HasRoomFor(Chunk->FirstBlock, PackSize))
    {
        if(!World->FirstFreeBlock)
        {
            World->FirstFreeBlock = PushStruct(&World->Arena, world_entity_block);
            World->FirstFreeBlock->Next = 0;
        }
        
        Chunk->FirstBlock = World->FirstFreeBlock;
        World->FirstFreeBlock = Chunk->FirstBlock->Next;

        ClearWorldEntityBlock(Chunk->FirstBlock);
    }

    world_entity_block *Block = Chunk->FirstBlock;

    Assert(HasRoomFor(Block, PackSize));
    u8 *Dest = (Block->EntityData + Block->EntityDataSize);
    Block->EntityDataSize += PackSize;
    ++Block->EntityCount;

    entity *DestE = (entity *)Dest;;
    *DestE = *Source;
    PackTraversableReference(&DestE->StandingOn);
    PackTraversableReference(&DestE->MovingTo);
    PackEntityReference(&DestE->Head);
}

internal void
PackEntityIntoWorld(world *World, entity *Source, world_position At)
{
    world_chunk *Chunk = GetWorldChunk(World, At.ChunkX, At.ChunkY, At.ChunkZ, &World->Arena);
    Assert(Chunk);
    PackEntityIntoChunk(World, Source, Chunk);
}

inline void
AddBlockToFreeList(world *World, world_entity_block *Old)
{
    Old->Next = World->FirstFreeBlock;
    World->FirstFreeBlock = Old;
}

inline void
AddChunkToFreeList(world *World, world_chunk *Old)
{
    Old->NextInHash = World->FirstFreeChunk;
    World->FirstFreeChunk = Old;
}
