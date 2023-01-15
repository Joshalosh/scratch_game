#if !defined(GAME_DEBUG_H)

#define TIMED_BLOCK()

struct timed_block 
{
    timed_block(u32 ID)
    {
        BEGIN_TIMED_BLOCK(ID);
    }

    ~timed_block()
    {
        END_TIMED_BLOCK(ID);
    }
};

#define GAME_DEBUG_H
#endif
