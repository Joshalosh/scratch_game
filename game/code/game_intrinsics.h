#if !defined(GAME_INTRINSICS_H)

#include "math.h"

#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline uint32_t AtomicCompareExchangeUInt32(uint32_t volatile *Value, uint32_t New, uint32_t Expected)
{
    uint32_t Result = _InterlockedCompareExchange((long *)Value, New, Expected);

    return(Result);
}
#elif COMPILER_LLVM
// TODO: Does LLVM have real read-specific barriers yet?
#define CompletePreviousReadsBeforeFutureReads asm volatile("" ::: "memory")
#define CompletePreviuosWritesBeforeFutureWrites asm volatile("" ::: "memory")
inline uint32_t AtomicCompareExchangeUInt32(uint32_t volatile *Value, uint32_t New, uint32_t Expected)
{
    uint32_t Result = __sync_val_compare_and_swap(Value, Expected, New);

    return(Result);
}
#else
// TODO: Other compilers / platforms.
#endif

inline int32_t
SignOf(int32_t Value)
{
    int32_t Result = (Value >= 0) ? 1 : -1;
    return(Result);
}

inline real32
SignOf(real32 Value)
{
    real32 Result = (Value >= 0) ? 1.0f : -1.0f;
    return(Result);
}

inline real32
SquareRoot(real32 Real32)
{
    real32 Result = sqrtf(Real32);
    return(Result);
}

inline real32
AbsoluteValue(real32 Real32)
{
    real32 Result = fabs(Real32);
    return(Result);
}

inline uint32_t
RotateLeft(uint32_t Value, int32_t Amount)
{
#if COMPILER_MSVC
    uint32_t Result = _rotl(Value, Amount);
#else
    Amount &= 31;
    uint32_t Result = ((Value << Amount) | (Value >> (32 - Amount)));
#endif
    return(Result);
}

inline uint32_t
RotateRight(uint32_t Value, int32_t Amount)
{
#if COMPILER_MSVC
    uint32_t Result = _rotl(Value, Amount);
#else
    Amount &= 31;
    uint32_t Result = ((Value >> Amount) | (Value << (32 - Amount)));
#endif
    return(Result);
}

inline int32_t
RoundReal32ToInt32(real32 Real32)
{
    int32_t Result = (int32_t)roundf(Real32);
    return(Result);
}

inline uint32_t
RoundReal32ToUInt32(real32 Real32)
{
    uint32_t Result = (uint32_t)roundf(Real32);
    return(Result);
}

inline int32_t
FloorReal32ToInt32(real32 Real32)
{
    int32_t Result = (int32_t)floorf(Real32);
    return(Result);
}

inline int32_t
CeilReal32ToInt32(real32 Real32)
{
    int32_t Result = (int32_t)ceilf(Real32);
    return(Result);
}

inline int32_t
TruncateReal32ToInt32(real32 Real32)
{
    int32_t Result = (int32_t)Real32;
    return(Result);
}

inline real32
Sin(real32 Angle)
{
    real32 Result = sinf(Angle);
    return(Result);
}

inline real32
Cos(real32 Angle)
{
    real32 Result = cosf(Angle);
    return(Result);
}

inline real32
ATan2(real32 Y, real32 X)
{
    real32 Result = atan2f(Y, X);
    return(Result);
}

struct bit_scan_result
{
    bool32 Found;
    uint32_t Index;
};
inline bit_scan_result
FindLeastSignificantSetBit(uint32_t Value)
{
    bit_scan_result Result = {};
    
#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(uint32_t Test = 0; Test < 32; ++Test)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif

    return(Result);
}

#define GAME_INSTRINSICS_H
#endif
