#if !defined(GAME_DEBUG_INTERFACE_H)
struct debug_table;
#define DEBUG_GAME_FRAME_END(name) debug_table *name(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef DEBUG_GAME_FRAME_END(debug_game_frame_end);

struct debug_id
{
    void *Value[2];
};

#if GAME_INTERNAL
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

    DebugType_CounterThreadList,
    // DebugType_CounterFunctionLost,
};
struct debug_event
{
    u64 Clock;
    // TODO: To save space we could put these two strings back-to-back 
    // with a null terminator in the middle
    char *Filename;
    char *BlockName;
    u32 LineNumber;
    u16 ThreadID;
    u16 CoreIndex;
    u8 Type;
    union
    {
        debug_id DebugID;

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
#define MAX_DEBUG_EVENT_COUNT (16*65536)
struct debug_table
{
    // TODO: No attempt is being made at the moment to ensure that
    // the final debug records being written to the event array 
    // actually complete their output to the swap of the event array index.
    
    u32 CurrentEventArrayIndex;
    u64 volatile EventArrayIndex_EventIndex;
    u32 EventCount[MAX_DEBUG_EVENT_ARRAY_COUNT];
    debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];
};

extern debug_table *GlobalDebugTable;

#define RecordDebugEvent(EventType, Block)                                                      \
    u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;                                        \
    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);                                                 \
    debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex;    \
    Event->Clock = __rdtsc();                                                                   \
    Event->Type = (u8)EventType;                                                                \
    Event->CoreIndex = 0;                                                                       \
    Event->ThreadID = (u16)GetThreadID();                                                       \
    Event->Filename = __FILE__;                                                                 \
    Event->LineNumber = __LINE__;                                                               \
    Event->BlockName = Block;                                                                   \

#define FRAME_MARKER(SecondsElapsedInit)                               \
     {                                                                 \
     int Counter = __COUNTER__;                                        \
     RecordDebugEvent(DebugType_FrameMarker, "Frame Marker");          \
     Event->Real32 = SecondsElapsedInit;                               \
    }

#define TIMED_BLOCK__(BlockName, Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, BlockName, ## __VA_ARGS__)
#define TIMED_BLOCK_(BlockName, Number, ...) TIMED_BLOCK__(BlockName, Number, ## __VA_ARGS__)
#define TIMED_BLOCK(BlockName, ...) TIMED_BLOCK_(#BlockName, __LINE__, ## __VA_ARGS__)
#define TIMED_FUNCTION(...) TIMED_BLOCK_((char *)__FUNCTION__, __LINE__, ## __VA_ARGS__)

#define BEGIN_BLOCK_(Counter, FilenameInit, LineNumberInit, BlockNameInit) \
    {RecordDebugEvent(DebugType_BeginBlock, BlockNameInit);}
#define END_BLOCK_(Counter)                            \
{                                                      \
    RecordDebugEvent(DebugType_EndBlock, "End Block"); \
}

#define BEGIN_BLOCK(Name)                                   \
    int Counter_##Name = __COUNTER__;                       \
    BEGIN_BLOCK_(Counter_##Name, __FILE__, __LINE__, #Name);

#define END_BLOCK(Name)         \
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

#define DEBUG_BEGIN_DATA_BLOCK(Name, ID)                                                \
{                                                                                       \
    RecordDebugEvent(DebugType_OpenDataBlock, #Name);                                   \
    Event->DebugID = ID;                                                                \
}

#define DEBUG_END_DATA_BLOCK()                                                          \
{                                                                                       \
    RecordDebugEvent(DebugType_CloseDataBlock, "End Data Block");                       \
}

#define DEBUG_VALUE(Value)                                                              \
{                                                                                       \
    RecordDebugEvent(DebugType_R32, #Value);                                            \
    DEBUGValueSetEventData(Event, Value);                                               \
}

#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)

inline debug_id DEBUG_POINTER_ID(void *Pointer)
{
    debug_id ID = {Pointer};

    return(ID);
}

#define DEBUG_UI_ENABLED 1

internal void DEBUG_HIT(debug_id ID, r32 ZValue);
internal b32 DEBUG_HIGHLIGHTED(debug_id ID, v4 *Colour);
internal b32 DEBUG_REQUESTED(debug_id ID);

#else

inline debug_id DEBUG_POINTER_ID(void *Pointer) {debug_id NullID = {}; return(NullID)}

#define DEBUG_BEGIN_DATA_BLOCK(...)
#define DEBUG_END_DATA_BLOCK(...)
#define DEBUG_VALUE(...)
#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)
#define DEBUG_UI_ENABLED 0
#define DEBUG_HIT(...)
#define DEBUG_HIGHLIGHTED(...) 0
#define DEBUG_REQUESTED(...) 0

#endif

#define GAME_DEBUG_INTERFACE_H
#endif
