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
    // TODO: Fix floating point math so this can be exact?
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
        if(!World->FirstFreeChunk)
        {
            World->FirstFreeChunk = PushStruct(Arena, world_chunk, NoClear());
            World->FirstFreeChunk->NextInHash = 0;
        }

        Result = World->FirstFreeChunk;
        World->FirstFreeChunk = Result->NextInHash;

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
    TIMED_FUNCTION();

    BeginTicketMutex(&World->ChangeTicket);

    world_chunk **ChunkPtr = GetWorldChunkInternal(World, ChunkX, ChunkY, ChunkZ);
    world_chunk *Result = *ChunkPtr;
    if(Result)
    {
        *ChunkPtr = Result->NextInHash;
    }

    EndTicketMutex(&World->ChangeTicket);

    return(Result);
}

internal world *
CreateWorld(v3 ChunkDimInMeters, memory_arena *ParentArena)
{
    world *World = PushStruct(ParentArena, world);

    World->ChunkDimInMeters = ChunkDimInMeters;
    World->FirstFree = 0;
    World->Arena = ParentArena;
    World->GameEntropy = RandomSeed(1234);

    return(World);
}

inline void
RecanonicaliseCoord(real32 ChunkDim, int32_t *Tile, real32 *TileRel)
{
    int32_t Offset = RoundReal32ToInt32(*TileRel / ChunkDim);
    *Tile += Offset;
    *TileRel -= (r32)Offset*ChunkDim;

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

inline b32
HasRoomFor(world_entity_block *Block, u32 Size)
{
    b32 Result = ((Block->EntityDataSize + Size) <= sizeof(Block->EntityData));
    return(Result);
}

internal void *
UseChunkSpace(world *World, u32 Size, world_chunk *Chunk)
{
    if(!Chunk->FirstBlock || !HasRoomFor(Chunk->FirstBlock, Size))
    {
        if(!World->FirstFreeBlock)
        {
            World->FirstFreeBlock = PushStruct(World->Arena, world_entity_block);
            World->FirstFreeBlock->Next = 0;
        }

        world_entity_block *NewBlock = World->FirstFreeBlock;
        World->FirstFreeBlock = NewBlock->Next;

        ClearWorldEntityBlock(NewBlock);

        NewBlock->Next = Chunk->FirstBlock;
        Chunk->FirstBlock = NewBlock;
    }

    world_entity_block *Block = Chunk->FirstBlock;

    Assert(HasRoomFor(Block, Size));
    u8 *Dest = (Block->EntityData + Block->EntityDataSize);
    Block->EntityDataSize += Size;
    ++Block->EntityCount;

    return(Dest);
}

internal void *
UseChunkSpace(world *World, u32 Size, world_position At)
{
    TIMED_FUNCTION();

    BeginTicketMutex(&World->ChangeTicket);

    world_chunk *Chunk = GetWorldChunk(World, At.ChunkX, At.ChunkY, At.ChunkZ, World->Arena);
    Assert(Chunk);
    void *Result = UseChunkSpace(World, Size, Chunk);

    EndTicketMutex(&World->ChangeTicket);

    return(Result);
}

inline void
AddToFreeList(world *World, world_chunk *Old, world_entity_block *FirstBlock, world_entity_block *LastBlock)
{
    TIMED_FUNCTION();

    BeginTicketMutex(&World->ChangeTicket);

    Old->NextInHash = World->FirstFreeChunk;
    World->FirstFreeChunk = Old;

    if(FirstBlock)
    {
        Assert(LastBlock);
        LastBlock->Next = World->FirstFreeBlock;
        World->FirstFreeBlock = FirstBlock;
    }

    EndTicketMutex(&World->ChangeTicket);
}

inline rectangle3
GetWorldChunkBounds(world *World, s32 ChunkX, s32 ChunkY, s32 ChunkZ)
{
    v3 ChunkCenter = Hadamard(World->ChunkDimInMeters, V3((f32)ChunkX, (f32)ChunkY, (f32)ChunkZ));
    rectangle3 Result = RectCenterDim(ChunkCenter, World->ChunkDimInMeters);

    return(Result);
}
