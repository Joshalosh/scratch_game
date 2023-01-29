#if !defined(GAME_DEBUG_H)

#define TIMED_BLOCK__(Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define TIMED_BLOCK_(Number, ...) TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ## __VA_ARGS__) 

struct debug_record
{
    char *Filename;
    char *FunctionName;

    int LineNumber;
    u32 Reserved;

    u64 HitCount_CycleCount;
};

debug_record DebugRecordArray[];

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
    }

    ~timed_block()
    {
        u32 Delta = (__rdtsc() - StartCycles) | ((u64)HitCount << 32);
        AtomicAddU64(&Record->HitCount_CycleCount, Delta);
    }
};

#define GAME_DEBUG_H
#endif
