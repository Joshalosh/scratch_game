
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

