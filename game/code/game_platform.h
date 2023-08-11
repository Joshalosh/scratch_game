#if !defined(GAME_PLATFORM_H)

// TODO: Have the meta-parser ignore its own #define
#define introspect(params)
#define counted_pointer(params)

#include "game_config.h"

/*
  GAME_INTERNAL:
    0 - Build for public release.
    1 - Build for developer only.

  GAME_SLOW:
    0 - Slow code not allowed.
    1 - Slow code welcome.
*/

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
#error SEE/NEON optimisations are not available for this compiler yet
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

#pragma pack(push, 1)
struct bitmap_id 
{
    u32 Value;
};

struct sound_id
{
    u32 Value;
};

struct font_id
{
    u32 Value;
};
#pragma pack(pop)

union v2
{
    struct
    {
        real32 x, y;
    };
    struct
    {
        real32 u, v;
    };
    real32 E[2];
};

union v3
{
    struct
    {
        real32 x, y, z;
    };
    struct
    {
        real32 u, v, w;
    };
    struct
    {
        real32 r, g, b;
    };
    struct
    {
        v2 xy;
        real32 Ignored0_;
    };
    struct
    {
        real32 Ignored1_;
        v2 yz;
    };
    struct
    {
        v2 uv;
        real32 Ignored2_;
    };
    struct
    {
        real32 Ignored3_;
        v2 vw;
    };
    real32 E[3];
};

union v4
{
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                real32 x, y, z;
            };
        };

        real32 w;
    };
    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                real32 r, g, b;
            };
        };

        real32 a;
    };
    struct
    {
        v2 xy;
        real32 Ignored0_;
        real32 Ignored1_;
    };
    struct
    {
        real32 Ignored2_;
        v2 yz;
        real32 Ignored3_;
    };
    struct
    {
        real32 Ignored4_;
        real32 Ignored5_;
        v2 zw;
    };
    real32 E[4];
};

introspect(category:"math") struct rectangle2
{
    v2 Min;
    v2 Max;
};

introspect(category:"math") struct rectangle3
{
    v3 Min;
    v3 Max;
};

#define Real32Maximum FLT_MAX

#if !defined(internal)
#define internal static
#endif
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

#if GAME_SLOW
// TODO: Need to complete the assertion macro.
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
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

typedef struct debug_executing_process
{
    u64 OSHandle;
} debug_executing_process;

typedef struct debug_process_state
{
    b32 StartedSuccessfully;
    b32 IsRunning;
    s32 ReturnCode;
} debug_process_state;

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *Filename, uint32_t MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#define DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(name) debug_executing_process name(char *Path, char *Command, char *CommandLine)
typedef DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(debug_platform_execute_system_command);

// TODO: Do I want a formal release mechanism here?
#define DEBUG_PLATFORM_GET_PROCESS_STATE(name) debug_process_state name(debug_executing_process Process)
typedef DEBUG_PLATFORM_GET_PROCESS_STATE(debug_platform_get_process_state);

// TODO: I should prabably actually start using this.
extern struct game_memory *DebugGlobalMemory;

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

enum game_input_mouse_button
{
    PlatformMouseButton_Left,
    PlatformMouseButton_Middle,
    PlatformMouseButton_Right,
    PlatformMouseButton_Extended0,
    PlatformMouseButton_Extended1,

    PlatformMouseButton_Count,
};
typedef struct game_input
{
    game_button_state MouseButtons[PlatformMouseButton_Count];
    r32 MouseX, MouseY, MouseZ;

    r32 dtForFrame;

    game_controller_input Controllers[5];
} game_input;

inline b32 WasPressed(game_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));

    return(Result);
}

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

#if GAME_INTERNAL
    debug_platform_free_file_memory *DEBUGFreeFileMemory;
    debug_platform_read_entire_file *DEBUGReadEntireFile;
    debug_platform_write_entire_file *DEBUGWriteEntireFile;
    debug_platform_execute_system_command *DEBUGExecuteSystemCommand;
    debug_platform_get_process_state *DEBUGGetProcessState;
#endif    debug_table *

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

    b32 ExecutableReloaded;
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
    uint32_t Result = _InterlockedCompareExchange((long volatile *)Value, New, Expected);

    return(Result);
}
inline u64 AtomicExchangeU64(u64 volatile *Value, u64 New)
{
    u64 Result = _InterlockedExchange64((__int64 volatile *)Value, New);

    return(Result);
}
inline u64 AtomicAddU64(u64 volatile *Value, u64 Addend)
{
    // Returns the original value _prior_ to adding.
    u64 Result = _InterlockedExchangeAdd64((__int64 volatile *)Value, Addend);

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
inline u64 AtomicExchangeU64(u64 volatile *Value, u64 New)
{
    u64 Result = __sync_lock_test_and_set(Value, New);

    return(Result);
}
inline u64 AtomicAddU64(u64 volatile *Value, u64 Addend)
{
    // Returns the original value _prior_ to adding.
    u64 Result = __sync_fetch_and_add(Value, Addend);

    return(Result);
}
inline u32 GetThreadID(void)
{
    u32 ThreadID;
#if defined(__APPLE__) && defined(__x86_64__)
    asm("mov %%gs:0x00,%0" : "=r"(ThreadID));
#elif defined(__i386__)
    asm("mov %%gs:0x08,%0" : "=r"(ThreadID));
#elif defined(__x86_64__)
    asm("mov %%fs:0x10,%0" : "=r"(ThreadID));
#else
#error Unsupported architecture
#endif

    return(ThreadID);
}
#else
// TODO: Other compilers / platforms.
#endif

struct debug_table;
#define DEBUG_GAME_FRAME_END(name) debug_table *name(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef DEBUG_GAME_FRAME_END(debug_game_frame_end);

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

#if GAME_INTERNAL
struct debug_record
{
    char *Filename;
    char *BlockName;

    u32 LineNumber;
    u32 Reserved;
};

enum debug_type
{
    DebugType_FrameMarker,
    DebugType_BeginBlock,
    DebugType_EndBlock,

    DebugType_OpenDataBlock,
    DebugType_CloseDataBlock,

    DebugType_B32,
    DebugType_R32,
    DebugType_U32,
    DebugType_S32,
    DebugType_V2,
    DebugType_V3,
    DebugType_V4,
    DebugType_Rectangle2,
    DebugType_Rectangle3,
    DebugType_BitmapID,
    DebugType_SoundID,
    DebugType_FontID,
    
    //

    DebugType_FirstUIType = 256,
    DebugType_CounterThreadList,
    // DebugType_CounterFunctionLost,
    DebugType_VarGroup,
};
struct threadid_coreindex
{
    u16 ThreadID;
    u16 CoreIndex;
};
struct debug_event
{
    u64 Clock;
    threadid_coreindex TC;
    u16 DebugRecordIndex;
    u8 TranslationUnit;
    u8 Type;
    union
    {
        void *VecPtr[2];

        b32 Bool32;
        s32 Int32;
        u32 UInt32;
        r32 Real32;
        v2 Vector2;
        v3 Vector3;
        v4 Vector4;
        rectangle2 Rectangle2;
        rectangle3 Rectangle3;
        bitmap_id BitmapID;
        sound_id SoundID;
        font_id FontID;
    };
};

#define MAX_DEBUG_THREAD_COUNT 256
#define MAX_DEBUG_EVENT_ARRAY_COUNT 8
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
    u32 EventCount[MAX_DEBUG_EVENT_ARRAY_COUNT];
    debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];

    u32 RecordCount[MAX_DEBUG_TRANSLATION_UNITS];
    debug_record Records[MAX_DEBUG_TRANSLATION_UNITS][MAX_DEBUG_RECORD_COUNT];
};

extern debug_table *GlobalDebugTable;

// TODO: I think i'll probably swicth away from the translation unity indexing
// and just go to a more standard one-time hash table because the complexity
// seems to be causing problems.
#define RecordDebugEvent(RecordIndex, EventType)                                          \
    u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;                                        \
    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);                                                 \
    debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex;    \
    Event->Clock = __rdtsc();                                                                   \
    Event->DebugRecordIndex = (u16)RecordIndex;                                                 \
    Event->TranslationUnit = TRANSLATION_UNIT_INDEX;                                            \
    Event->Type = (u8)EventType;                                                                \
    Event->TC.CoreIndex = 0;                        \
    Event->TC.ThreadID = (u16)GetThreadID();        \

#define FRAME_MARKER(SecondsElapsedInit) \
     { \
     int Counter = __COUNTER__; \
     RecordDebugEvent(Counter, DebugType_FrameMarker); \
     Event->Real32 = SecondsElapsedInit;    \
     debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
     Record->Filename = __FILE__; \
     Record->LineNumber = __LINE__; \
     Record->BlockName = "Frame Marker"; \
    }

#define TIMED_BLOCK__(BlockName, Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, BlockName, ## __VA_ARGS__)
#define TIMED_BLOCK_(BlockName, Number, ...) TIMED_BLOCK__(BlockName, Number, ## __VA_ARGS__)
#define TIMED_BLOCK(BlockName, ...) TIMED_BLOCK_(#BlockName, __LINE__, ## __VA_ARGS__)
#define TIMED_FUNCTION(...) TIMED_BLOCK_((char *)__FUNCTION__, __LINE__, ## __VA_ARGS__)

#define BEGIN_BLOCK_(Counter, FilenameInit, LineNumberInit, BlockNameInit) \
    {debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
    Record->Filename = FilenameInit; \
    Record->LineNumber = LineNumberInit; \
    Record->BlockName = BlockNameInit; \
    RecordDebugEvent(Counter, DebugType_BeginBlock);}
#define END_BLOCK_(Counter) \
{ \
    RecordDebugEvent(Counter, DebugType_EndBlock); \
}

#define BEGIN_BLOCK(Name) \
    int Counter_##Name = __COUNTER__; \
    BEGIN_BLOCK_(Counter_##Name, __FILE__, __LINE__, #Name);

#define END_BLOCK(Name) \
    END_BLOCK_(Counter_##Name);

struct timed_block
{
    int Counter;

    timed_block(int CounterInit, char *Filename, int LineNumber, char *BlockName, u32 HitCountInit = 1)
    {
        // TODO: Should I record the hit count value here?
        Counter = CounterInit;
        BEGIN_BLOCK_(Counter, Filename, LineNumber, BlockName);
    }

    ~timed_block()
    {
        END_BLOCK_(Counter);
    }
};

#else

#define TIMED_BLOCK(...)
#define TIMED_FUNCTION(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)
#define FRAME_MARKER(...)

#endif

//
// SHARED UTILS
//

inline u32
StringLength(char *String)
{
    u32 Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return(Count);
}

#ifdef __cplusplus
}
#endif


#if defined(__cplusplus) && defined(GAME_INTERNAL)

inline void
DEBUGValueSetEventData(debug_event *Event, r32 Value)
{
    Event->Type = DebugType_R32;
    Event->Real32 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, u32 Value)
{
    Event->Type = DebugType_U32;
    Event->UInt32 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, s32 Value)
{
    Event->Type = DebugType_S32;
    Event->Int32 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, v2 Value)
{
    Event->Type = DebugType_V2;
    Event->Vector2 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, v3 Value)
{
    Event->Type = DebugType_V3;
    Event->Vector3 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, v4 Value)
{
    Event->Type = DebugType_V3;
    Event->Vector4 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, rectangle2 Value)
{
    Event->Type = DebugType_Rectangle2;
    Event->Rectangle2 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, rectangle3 Value)
{
    Event->Type = DebugType_Rectangle3;
    Event->Rectangle3 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, bitmap_id Value)
{
    Event->Type = DebugType_BitmapID;
    Event->BitmapID = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, sound_id Value)
{
    Event->Type = DebugType_SoundID;
    Event->SoundID = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, font_id Value)
{
    Event->Type = DebugType_FontID;
    Event->FontID = Value;
}

#define DEBUG_BEGIN_DATA_BLOCK(Name, Ptr0, Ptr1)                                        \
{                                                                                       \
    int Counter = __COUNTER__;                                                          \
    RecordDebugEvent(Counter, DebugType_OpenDataBlock);                          \
    Event->VecPtr[0] = Ptr0;                                                            \
    Event->VecPtr[1] = Ptr1;                                                            \
    debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
    Record->Filename = __FILE__;                                                        \
    Record->LineNumber = __LINE__;                                                      \
    Record->BlockName = Name;                                                           \
}

#define DEBUG_END_DATA_BLOCK()                                                          \
{                                                                                       \
    int Counter = __COUNTER__;                                                          \
    RecordDebugEvent(Counter, DebugType_CloseDataBlock);                         \
    debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
    Record->Filename = __FILE__;                                                        \
    Record->LineNumber = __LINE__;                                                      \
}

#define DEBUG_VALUE(Value)                                                              \
{                                                                                       \
    int Counter = __COUNTER__;                                                          \
    RecordDebugEvent(Counter, DebugType_R32);                                    \
    DEBUGValueSetEventData(Event, Value);                                               \
    debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
    Record->Filename = __FILE__;                                                        \
    Record->LineNumber = __LINE__;                                                      \
    Record->BlockName = "Value";                                                        \
}

#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)

#else

#define DEBUG_BEGIN_DATA_BLOCK(...)
#define DEBUG_END_DATA_BLOCK(...)
#define DEBUG_VALUE(...)
#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)

#endif

#define GAME_PLATFORM_H
#endif
