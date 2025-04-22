
// TODO: Have the meta-parser ignore its own #define
#define introspect(IGNORED)
#define counted_pointer(IGNORED)

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
typedef real32 f32;
typedef real64 f64;

typedef uintptr_t umm;
typedef intptr_t smm;

#define U32FromPointer(Pointer) ((u32)(memory_index)(Pointer))
#define PointerFromU32(type, Value) (type *)((memory_index)Value)

#define OffsetOf(type, Member) (umm)&(((type *)0)->Member)

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

#define U16Maximum 65535
#define U32Maximum ((u32)-1)
#define Real32Maximum FLT_MAX
#define Real32Minimum -FLT_MAX
#define R32Maximum FLT_MAX
#define R32Minimum -FLT_MAX

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

inline u16
SafeTruncateToU16(u32 Value)
{
    // TODO: Defines for maximum values
    Assert(Value <= 0xFFFF);
    u16 Result = (u16)Value;
    return(Result);
}

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

struct ticket_mutex
{
    u64 volatile Ticket;
    u64 volatile Serving;
};

inline void
BeginTicketMutex(ticket_mutex *Mutex)
{
    u64 Ticket = AtomicAddU64(&Mutex->Ticket, 1);
    while(Ticket != Mutex->Serving) {_mm_pause();}
}

inline void
EndTicketMutex(ticket_mutex *Mutex)
{
    AtomicAddU64(&Mutex->Serving, 1);
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

struct manual_sort_key
{
    u16 AlwaysInFrontOf;
    u16 AlwaysBehind;
};

#define BITMAP_BYTES_PER_PIXEL 4
typedef struct game_offscreen_buffer
{
    // Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
} game_offscreen_buffer;

typedef struct game_render_commands
{
    u32 Width;
    u32 Height;

    u32 MaxPushBufferSize;
    u32 SortEntryCount;
    u8 *PushBufferBase;
    u8 *PushBufferDataAt;

    v4 ClearColor;

    u32 LastUsedManualSortKey;
    u32 ClipRectCount;
    u32 MaxRenderTargetIndex;

    struct render_entry_cliprect *FirstRect;
    struct render_entry_cliprect *LastRect;
} game_render_commands;
#define RenderCommandStruct(MaxPushBufferSize, PushBuffer, Width, Height) \
    {Width, Height, MaxPushBufferSize, 0, (u8 *)PushBuffer, ((u8 *)PushBuffer) + MaxPushBufferSize};

inline struct sort_sprite_bound *GetSortEntries(game_render_commands *Commands)
{
    sort_sprite_bound *Result = (sort_sprite_bound *)Commands->PushBufferBase;
    return(Result);
}

typedef struct game_render_prep
{
    struct render_entry_cliprect *ClipRects;
    u32 SortedIndexCount;
    u32 *SortedIndices;
} game_render_prep;

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

#define MAX_CONTROLLER_COUNT 5
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
    r32 dtForFrame;

    game_controller_input Controllers[MAX_CONTROLLER_COUNT];

    // Signals back to the platform layer
    b32 QuitRequested;

    // For debugging only
    game_button_state MouseButtons[PlatformMouseButton_Count];
    r32 MouseX, MouseY, MouseZ;
    b32 ShiftDown, AltDown, ControlDown;
} game_input;

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

inline b32 WasPressed(game_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));

    return(Result);
}

inline b32 IsDown(game_button_state State)
{
    b32 Result = (State.EndedDown);

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

struct platform_texture_op_queue
{
    ticket_mutex Mutex;

    struct texture_op *First;
    texture_op *Last;
    texture_op *FirstFree;
};

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
#endif

} platform_api;
extern platform_api Platform;

typedef struct game_memory
{
    struct game_state *GameState;
    struct transient_state *TransientState;

#if GAME_INTERNAL
    struct debug_table *DebugTable;
    struct debug_state *DebugState;
#endif

    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;
    platform_texture_op_queue TextureOpQueue;

    b32 ExecutableReloaded;
    platform_api PlatformAPI;
} game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_render_commands *RenderCommands)
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

#include "game_debug_interface.h"
