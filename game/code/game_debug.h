#if !defined(GAME_DEBUG_H)

#define TIMED_BLOCK(ID) timed_block TimedBlock##ID(DebugCycleCounter_##ID);

struct timed_block 
{
    u64 StartCycleCount;
    u32 ID;

    timed_block(u32 IDInit)
    {
        ID = IDInit;
        BEGIN_TIMED_BLOCK_(StartCycleCount);
    }

    ~timed_block()
    {
        END_TIMED_BLOCK_(StartCycleCount, ID);
    }
};

#define GAME_DEBUG_H
#endif
