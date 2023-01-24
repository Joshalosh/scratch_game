#if !defined(GAME_DEBUG_H)

#define TIMED_BLOCK__(Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define TIMED_BLOCK_(Number, ...) TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ## __VA_ARGS__) 

struct debug_record
{
    char *Filename;
    char *FunctionName;

    u32 CycleCount;
    int LineNumber;
    u32 HitCount;
    u32 Reserved;
};

debug_record DebugRecordArray[];

struct timed_block 
{
    debug_record *Record;
    u64 StartCycles;

    timed_block(int Counter, char *Filename, int LineNumber, char *FunctionName, int HitCount = 1)
    {
        //TODO: Thread safety.
        Record = DebugRecordArray + Counter;
        Record->Filename = Filename;
        Record->LineNumber = LineNumber;
        Record->FunctionName = FunctionName;
        AtomicAddU32(&Record->HitCount, HitCount);

        StartCycles = __rdtsc();
    }

    ~timed_block()
    {
        u32 DeltaTime = (u32)(__rdtsc() - StartCycles);
        AtomicAddU32(&Record->CycleCount, DeltaTime);
    }
};

#define GAME_DEBUG_H
#endif
