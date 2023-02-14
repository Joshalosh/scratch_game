#if !defined(GAME_DEBUG_H)

#define TIMED_BLOCK__(Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define TIMED_BLOCK_(Number, ...) TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ## __VA_ARGS__)

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
    u8 DebugRecordArrayIndex;;
    u8 Type;
};

debug_record DebugRecordArray[];

#define MAX_DEBUG_EVENT_COUNT 65536
extern u32 DebugEventIndex;
extern debug_event DebugEventArray[];

#define RecordDebugEvent(Type)
        u32 EventIndex = AtomicAddU32(&DebugEventIndex, 1);
        Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);
        debug_event *Event = DebugEventArray + EventIndex; 
        Event->Clock = __rdtsc();
        Event->ThreadIndex = 0;
        Event->CoreIndex = 0;
        Event->DebugRecordIndex = (u16)Counter;
        Event->DebugRecordArrayIndex = DebugRecordArrayIndex;
        Event->Type = Type;

struct timed_block
{
    debug_record *Record;
    u64 StartCycles;
    u32 HitCount;

    timed_block(int Counter, char *Filename, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
    {
        HitCount = HitCountInit;
        Record = DebugRecordArray + Counter;
        Record->Filename = Filename;
        Record->LineNumber = LineNumber;
        Record->FunctionName = FunctionName;

        StartCycles = __rdtsc();

        //

        RecordDebugEvent(DebugEvent_BeginBlock);
    }

    ~timed_block()
    {
        u64 Delta = (__rdtsc() - StartCycles) | ((u64)HitCount << 32);
        AtomicAddU64(&Record->HitCount_CycleCount, Delta);
        
        RecordDebugEvent(DebugEvent_BeginBlock);
    }
};

struct debug_counter_snapshot
{
    u32 HitCount;
    u32 CycleCount;
};

#define DEBUG_SNAPSHOT_COUNT 120
struct debug_counter_state
{
    char *Filename;
    char *FunctionName;

    u32 LineNumber;

    debug_counter_snapshot Snapshots[DEBUG_SNAPSHOT_COUNT];
};

struct debug_state 
{
    u32 SnapshotIndex;
    u32 CounterCount;
    debug_counter_state CounterStates[512];
    debug_frame_end_info FrameEndInfos[DEBUG_SNAPSHOT_COUNT];
};

// TODO: Fix this for looped live code editing.
struct render_group;
struct game_assets;
global_variable render_group *DEBUGRenderGroup;

internal void DEBUGReset(game_assets *Assets, u32 Width, u32 Height);
internal void DEBUGOverlay(game_memory *Memory);

#define GAME_DEBUG_H
#endif
