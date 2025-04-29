
struct memory_block_footer
{
    u8 *Base;
    umm Size;
    umm Used;
};

struct memory_arena
{
    memory_index Size;
    uint8_t *Base;
    memory_index Used;

    memory_index MinimumBlockSize;

    u64 AllocationFlags;
    u32 BlockCount;
    s32 TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    u8 *Base;
    memory_index Used;
};

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    uint8_t *Byte = (uint8_t *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

inline void
SetMinimumBlockSize(memory_arena *Arena, memory_index MinimumBlockSize)
{
    Arena->MinimumBlockSize = MinimumBlockSize;
}

inline void
InitialiseArena(memory_arena *Arena, memory_index MinimumBlockSize)
{
    Arena->Size = 0;
    Arena->Base = 0;
    Arena->Used = 0;
    Arena->TempCount = 0;
    Arena->MinimumBlockSize = MinimumBlockSize;
}

inline memory_index
GetAlignmentOffset(memory_arena *Arena, memory_index Alignment)
{
    memory_index AlignmentOffset = 0;

    memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
    memory_index AlignmentMask = Alignment - 1;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
    }

    return(AlignmentOffset);
}

enum arena_push_flag
{
    ArenaFlag_ClearToZero = 0x1,
};
struct arena_push_params
{
    u32 Flags;
    u32 Alignment;
};

struct arena_bootstrap_params
{
    u64 AllocationFlags;
    umm MinimumBlockSize;
};

inline arena_push_params
DefaultArenaParams()
{
    arena_push_params Params;
    Params.Flags = ArenaFlag_ClearToZero;
    Params.Alignment = 4;
    return(Params);
}

inline arena_bootstrap_params
DefaultBootstrapParams()
{
    arena_bootstrap_params Params = {};
    return(Params);
}

inline arena_push_params
AlignNoClear(u32 Alignment)
{
    arena_push_params Params = DefaultArenaParams();
    Params.Flags &= ~ArenaFlag_ClearToZero;
    Params.Alignment = Alignment;
    return(Params);
}

inline arena_push_params
Align(u32 Alignment, b32 Clear)
{
    arena_push_params Params = DefaultArenaParams();
    if(Clear)
    {
        Params.Flags |= ArenaFlag_ClearToZero;
    }
    else
    {
        Params.Flags &= ~ArenaFlag_ClearToZero;
    }
    Params.Alignment = Alignment;
    return(Params);
}

inline arena_push_params
NoClear()
{
    arena_push_params Params = DefaultArenaParams();
    Params.Flags &= ~ArenaFlag_ClearToZero;
    return(Params);
}

inline memory_index
GetArenaSizeRemaining(memory_arena *Arena, arena_push_params Params = DefaultArenaParams())
{
    memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Params.Alignment));

    return(Result);
}

// TODO Optional "clear" parameter.
#define PushStruct(Arena, type, ...) (type *)PushSize_(Arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(Arena, Count, type, ...) (type *)PushSize_(Arena, (Count)*sizeof(type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)
#define PushCopy(Arena, Size, Source, ...) Copy(Size, Source, PushSize_(Arena, Size, ## __VA_ARGS__))
inline memory_index
GetEffectiveSizeFor(memory_arena *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
{
    memory_index Size = SizeInit;

    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Params.Alignment);
    Size += AlignmentOffset;

    return(Size);
}

inline b32
ArenaHasRoomFor(memory_arena *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
{
    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Params);
    b32 Result = ((Arena->Used + Size) <= Arena->Size);
    return(Result);
}

inline memory_block_footer *
GetFooter(memory_arena *Arena)
{
    memory_block_footer *Result = (memory_block_footer *)(Arena->Base + Arena->Size);
    return(Result);
}

inline void *
PushSize_(memory_arena *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
{
    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Params);

    if((Arena->Used + Size) > Arena->Size)
    { 
        if(!Arena->MinimumBlockSize)
        {
            // TODO: Tune default block size eventually
            Arena->MinimumBlockSize = 1024*1024;
        }

        memory_block_footer Save;
        Save.Base = Arena->Base;
        Save.Size = Arena->Size;
        Save.Used = Arena->Used;

        Size = SizeInit; // NOTE: The base will automatically be aligned now
        memory_index BlockSize = Maximum(Size + sizeof(memory_block_footer), Arena->MinimumBlockSize);
        Arena->Size = BlockSize - sizeof(memory_block_footer);
        Arena->Base = (u8 *)Platform.AllocateMemory(BlockSize, Arena->AllocationFlags);
        Arena->Used = 0;
        ++Arena->BlockCount;

        memory_block_footer *Footer = GetFooter(Arena);
        *Footer = Save;
    }

    Assert((Arena->Used + Size) <= Arena->Size);

    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Params.Alignment);
    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;

    Assert(Size >= SizeInit);

    if(Params.Flags & ArenaFlag_ClearToZero)
    {
        ZeroSize(SizeInit, Result);
    }

    return(Result);
}

// This isn't for production use, it's probably only
// really something I'll need during testing, but you never know.
inline char *
PushString(memory_arena *Arena, char *Source)
{
    u32 Size = 1;
    for(char *At = Source; *At; ++At)
    {
        ++Size;
    }

    char *Dest = (char *)PushSize_(Arena, Size, NoClear());
    for(u32 CharIndex = 0; CharIndex < Size; ++CharIndex)
    {
        Dest[CharIndex] = Source[CharIndex];
    }

    return(Dest);
}

inline char *
PushAndNullTerminate(memory_arena *Arena, u32 Length, char *Source)
{
    char *Dest = (char *)PushSize_(Arena, Length + 1, NoClear());
    for(u32 CharIndex = 0; CharIndex < Length; ++CharIndex)
    {
        Dest[CharIndex] = Source[CharIndex];
    }
    Dest[Length] = 0;

    return(Dest);
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;

    Result.Arena = Arena;
    Result.Base = Arena->Base;
    Result.Used = Arena->Used;

    ++Arena->TempCount;

    return(Result);
}

inline void
FreeLastBlock(memory_arena *Arena)
{
    void *Free = Arena->Base;

    memory_block_footer *Footer = GetFooter(Arena);

    Arena->Base = Footer->Base;
    Arena->Size = Footer->Size;
    Arena->Used = Footer->Used;

    Platform.DeallocateMemory(Free);

    --Arena->BlockCount;
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    while(Arena->Base != TempMem.Base)
    {
        FreeLastBlock(Arena);
    }

    Assert(Arena->Used >= TempMem.Used);
    Arena->Used = TempMem.Used;
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

inline void
Clear(memory_arena *Arena)
{
    while(Arena->BlockCount > 0)
    {
        FreeLastBlock(Arena);
    }
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

inline void
SubArena(memory_arena *Result, memory_arena *Arena, memory_index Size, arena_push_params Params = DefaultArenaParams())
{
    Result->Size = Size;
    Result->Base = (uint8_t *)PushSize_(Arena, Size, Params);
    Result->Used = 0;
    Result->TempCount = 0;
}

inline void *
Copy(memory_index Size, void *SourceInit, void *DestInit)
{
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    while(Size--) {*Dest++ = *Source++;}

    return(DestInit);
}

#define BootstrapPushStruct(type, Member, ...) (type *)BootstrapPushSize_(sizeof(type), OffsetOf(type, Member), ## __VA_ARGS__)
inline void *
BootstrapPushSize_(umm StructSize, umm OffsetToArena, 
                   arena_bootstrap_params BootstrapParams = DefaultBootstrapParams(), 
                   arena_push_params Params = DefaultArenaParams())
{
    memory_arena Bootstrap = {};
    Bootstrap.AllocationFlags = BootstrapParams.AllocationFlags;
    Bootstrap.MinimumBlockSize = BootstrapParams.MinimumBlockSize;
    void *Struct = PushSize(&Bootstrap, StructSize, Params);
    *(memory_arena *)((u8 *)Struct + OffsetToArena) = Bootstrap;

    return(Struct);
}
