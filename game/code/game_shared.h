#if !defined(GAME_SHARED_H)

#include "game_intrinsics.h"
#include "game_math.h"

inline b32
IsEndOfLine(char c)
{
    b32 Result = ((c == '\n') ||
                  (c == '\r'));

    return(Result);
}

inline b32
IsWhitespace(char c) 
{
    b32 Result = ((c == ' ') ||
                  (c == '\t') ||
                  (c == '\v') ||
                  (c == '\f') ||
                  IsEndOfLine(c));

    return(Result);
}

inline b32
StringsAreEqual(char *A, char *B)
{
    b32 Result = (A == B);

    if(A && B)
    {
        while(*A && *B && (*A == *B))
        {
            ++A;
            ++B;
        }
        
        Result = ((*A == 0) && (*B == 0));
    }

    return(Result);
}

inline b32
StringsAreEqual(umm ALength, char *A, char *B)
{
    char *At = B;
    for(umm Index = 0; Index < ALength; ++Index, ++At)
    {
        if((*At == 0) || (A[Index] != *At))
        {
            return(false);
        }
    }
    
    b32 Result = (*At == 0);
    return(Result);
}

inline b32
StringsAreEqual(memory_index ALength, char *A, memory_index BLength, char *B)
{
    b32 Result = (ALength == BLength);

    if(Result)
    {
        Result = true;
        for(u32 Index = 0; Index < ALength; ++Index)
        {
            if(A[Index] != B[Index])
            {
                Result = false;
                break;
            }
        }
    }

    return(Result);
}

#define GAME_SHARED_H
#endif