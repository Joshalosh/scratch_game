#if !defined(GAME_PLATFORM_H)

#ifdef __cplusplus
extern "C" {
#endif

//
// NOTE: Compilers
//

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
// TODO: Add more compilers
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SSE/NEON optimisations are not available for this compiler yet
#endif

//
// NOTE: Types
//
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

typedef size_t memory_index;

typedef int32_t bool32;
typedef float real32;
typedef double real64;

typedef int8_t s8;
typedef int8_t s08;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef bool32 b32;

typedef uint8_t u8;
typedef uint8_t u08;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef real32 r32;
typedef real64 r64;

#define Real32Maximum FLT_MAX

#if !defined(internal)
#define internal static
#endif
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

#if GAME_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *) 0 = 0;}
#else
#define Assert(Expression)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define AlignPow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Alignment) - 1))
#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)

inline uint32_t
SafeTruncateUInt64(uint64_t Value)
{
    // TODO: Defines maximum values.
    Assert(Value <= 0xFFFFFFFF);
    uint32_t Result = (uint32_t)Value;
    return(Result);
}

/*
  Services that the platform layer provides to the game
*/
#if GAME_INTERNAL
/* IMPORTANT

   These are not for doing anything in the shipping game - they are
   blocking and the write doesn't protect against lost data!
*/
typedef struct debug_read_file_result
{
    uint32_t ContentsSize;
    void *Contents;
} debug_read_file_result;

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *Filename, uint32_t MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

extern struct game_memory *DebugGlobalMemory;

#if 0
#if (COMPILER_MSVC || COMPILER_LLVM)
#define BEGIN_TIMED_BLOCK_(StartCycleCount) StartCycleCount = __rdtsc();
#define BEGIN_TIMED_BLOCK(ID) u64 BEGIN_TIMED_BLOCK_(StartCycleCount##ID)
#define END_TIMED_BLOCK_(StartCycleCount, ID) DebugGlobalMemory->Counters[ID].CycleCount += __rdtsc() - StartCycleCount; ++DebugGlobalMemory->Counters[ID].HitCount;
#define END_TIMED_BLOCK(ID) END_TIMED_BLOCK_(StartCycleCount##ID, DebugCycleCounter_##ID)
// TODO: Clamp END_TIMED_BLOCK_COUNTED so that if the calculation 
// is wrong it won't overflow.
#define END_TIMED_BLOCK_COUNTED(ID, Count) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID; DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount += (Count);
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK_COUNTED(ID, Count)
#endif
#endif

#endif

/*
  Services that the game provides to the platform layer
  (this may expand in the future - sound on seperate thread, etc)
*/

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

// TODO: In the future, rendering _specifically_ will become a three-tiered abstraction
#define BITMAP_BYTES_PER_PIXEL 4
typedef struct game_offscreen_buffer
{
    // Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
} game_offscreen_buffer;

typedef struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;

    // IMPORTANT: Samples need to be padded to a  multiple of 4 samples.
    int16_t *Samples;
} game_sound_output_buffer;

typedef struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
} game_button_state;

typedef struct game_controller_input
{
    bool32 IsConnected;
    bool32 IsAnalogue;
    real32 StickAverageX;
    real32 StickAverageY;

    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Back;
            game_button_state Start;

            // All Buttons must be added above this line

            game_button_state Terminator;
        };
    };
} game_controller_input;

typedef struct game_input
{
    game_button_state MouseButtons[5];
    int32_t MouseX, MouseY, MouseZ;

    bool32 ExecutableReloaded;
    real32 dtForFrame;

    game_controller_input Controllers[5];
} game_input;

typedef struct platform_file_handle
{
    b32 NoErrors;
    void *Platform;
} platform_file_handle;

typedef struct platform_file_group
{
    u32 FileCount;
    void *Platform;
} platform_file_group;

typedef enum platform_file_type
{
    PlatformFileType_AssetFile,
    PlatformFileType_SavedGameFile,

    PlatformFileType_Count,
} platform_file_type;

#define PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(name) platform_file_group name(platform_file_type Type)
typedef PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(platform_get_all_files_of_type_begin);

#define PLATFORM_GET_ALL_FILE_OF_TYPE_END(name) void name(platform_file_group *FileGroup)
typedef PLATFORM_GET_ALL_FILE_OF_TYPE_END(platform_get_all_files_of_type_end);

#define PLATFORM_OPEN_FILE(name) platform_file_handle name(platform_file_group *FileGroup)
typedef PLATFORM_OPEN_FILE(platform_open_next_file);

#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle *Source, u64 Offset, u64 Size, void *Dest)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle *Handle, char *Message)
typedef PLATFORM_FILE_ERROR(platform_file_error);

#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *Queue, void *Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

#define PLATFORM_ALLOCATE_MEMORY(name) void *name(memory_index Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void *Memory)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

typedef void platform_add_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

typedef struct platform_api
{
    platform_add_entry *AddEntry;
    platform_complete_all_work *CompleteAllWork;

    platform_get_all_files_of_type_begin *GetAllFilesOfTypeBegin;
    platform_get_all_files_of_type_end *GetAllFilesOfTypeEnd;
    platform_open_next_file *OpenNextFile;
    platform_read_data_from_file *ReadDataFromFile;
    platform_file_error *FileError;

    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;

    debug_platform_free_file_memory *DEBUGFreeFileMemory;
    debug_platform_read_entire_file *DEBUGReadEntireFile;
    debug_platform_write_entire_file *DEBUGWriteEntireFile;
} platform_api;

typedef struct game_memory
{
    uint64_t PermanentStorageSize;
    void *PermanentStorage; // Required to be cleared to zero at startup

    uint64_t TransientStorageSize;
    void *TransientStorage; // Required to be cleared to zero at startup

    uint64_t DebugStorageSize;
    void *DebugStorage; // Required to be cleared to zero at startup

    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;

    platform_api PlatformAPI;
} game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// At the moment, this has to be a very fast function, it cannot be
// more than a millisecond or so.
// TODO: Reduce the pressure on this function's performance by measuring it
// or asking about it, etc.
#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline uint32_t AtomicCompareExchangeUInt32(uint32_t volatile *Value, uint32_t New, uint32_t Expected)
{
    uint32_t Result = _InterlockedCompareExchange((long *)Value, New, Expected);

    return(Result);
}
inline u64 AtomicExchangeU64(u64 volatile *Value, u64 New)
{
    u64 Result = _InterlockedExchange64((__int64 *)Value, New);

    return(Result);
}
inline u64 AtomicAddU64(u64 volatile *Value, u64 Addend)
{
    // Returns the original value _prior_ to adding.
    u64 Result = _InterlockedExchangeAdd64((__int64 *)Value, Addend);

    return(Result);
}
inline u32 GetThreadID(void)
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);

    return(ThreadID);
}

#elif COMPILER_LLVM
// TODO: Does LLVM have real read-specific barriers yet?
#define CompletePreviousReadsBeforeFutureReads asm volatile("" ::: "memory")
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")
inline uint32_t AtomicCompareExchangeUInt32(uint32_t volatile *Value, uint32_t New, uint32_t Expected)
{
    uint32_t Result = __sync_val_compare_and_swap(Value, Expected, New);

    return(Result);
}
#else
// TODO: Other compilers / platforms.
#endif

struct debug_frame_timestamp
{
    char *Name;
    r32 Seconds;
};
struct debug_frame_end_info
{
    u32 TimestampCount;
    debug_frame_timestamp Timestamps[64];
};
#define DEBUG_GAME_FRAME_END(name) void name(game_memory *Memory, debug_frame_end_info *Info)
typedef DEBUG_GAME_FRAME_END(debug_game_frame_end);

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

struct debug_record
{
    char *Filename;
    char *FunctionName;

    u32 LineNumber;
    u32 Reserved;

    u64 HitCount_CycleCount;
};

enum debug_event_type
{
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,
};
struct debug_event
{
    u64 Clock;
    u16 ThreadIndex;
    u16 CoreIndex;
    u16 DebugRecordIndex;
    u8 TranslationUnit;
    u8 Type;
};

#define MAX_DEBUG_TRANSLATION_UNITS 3 
#define MAX_DEBUG_EVENT_COUNT (16*65536)
#define MAX_DEBUG_RECORD_COUNT (65536)
struct debug_table
{
// TODO: No attempt is being made at the moment to ensure that
// the final debug records being written to the event array 
// actually complete their output to the swap of the event array index.
    u32 CurrentEventArrayIndex;
    u64 volatile EventArrayIndex_EventIndex;
    debug_event Events[2][MAX_DEBUG_EVENT_COUNT];
    debug_record Records[MAX_DEBUG_TRANSLATION_UNITS][MAX_DEBUG_RECORD_COUNT];
};

extern debug_table GlobalDebugTable;

inline void
RecordDebugEvent(int RecordIndex, debug_event_type EventType)
{
    u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable.EventArrayIndex_EventIndex, 1);
    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;
    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);
    debug_event *Event = GlobalDebugTable.Events[ArrayIndex_EventIndex >> 32] + EventIndex;
    Event->Clock = __rdtsc(); 
    Event->ThreadIndex = (u16)GetThreadID();                                  
    Event->CoreIndex = 0;                                      
    Event->DebugRecordIndex = (u16)RecordIndex;
    Event->TranslationUnit = TRANSLATION_UNIT_INDEX;
    Event->Type = (u8)EventType;
}

#define TIMED_BLOCK__(Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define TIMED_BLOCK_(Number, ...) TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ## __VA_ARGS__)
struct timed_block
{
    int Counter;

    timed_block(int CounterInit, char *Filename, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
    {
        Counter = CounterInit;
        debug_record *Record = GlobalDebugTable.Records[TRANSLATION_UNIT_INDEX] + Counter;
        Record->Filename = Filename;
        Record->LineNumber = LineNumber;
        Record->FunctionName = FunctionName;

        RecordDebugEvent(Counter, DebugEvent_BeginBlock);
    }

    ~timed_block()
    {
        RecordDebugEvent(Counter, DebugEvent_EndBlock);
    }
};

#ifdef __cplusplus
}
#endif

#define GAME_PLATFORM_H
#endif
