#if !defined(GAME_DEBUG_H)

#define TIMED_BLOCK timed_block TimedBlock_##__LINE__(__COUNTER__, __FILE__, __LINE__, __FUNCTION__)

struct debug_record
{
    u64 Clocks;

    char *Filename;
    char *FunctionName;

    int LineNumber;
    u32 HitCount;
};

debug_record DebugRecordArray[];

struct timed_block 
{
    debug_record *Record;

    timed_block(int Counter, char *Filename, int LineNumber, char *FunctionName)
    {
        Record = DebugRecordArray + Counter;
        Record->Filename = Filename;
        Record->LineNumber = LineNumber;
        Record->FunctionName = FunctionName;
        Record->Clocks -= __rdtsc();
        ++Record->HitCount;
    }

    ~timed_block()
    {
        Record->Clocks += __rdtsc();
    }
};

#define GAME_DEBUG_H
#endif
