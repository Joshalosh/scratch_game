#if !defined(GAME_INTRINSICS_H)

#include "math.h"

int32_t
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

#define GAME_INSTRINSICS_H
#endif
