
#include "game_intrinsics.h"
#include "game_math.h"
#include "game_memory.h"

#include <stdarg.h>

global_variable v3 DebugColorTable[] =
{
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    {1, 1, 0},
    {0, 1, 1},
    {1, 0, 1},
    {1, 0.5f, 0},
    {1, 0, 0.5f},
    {0.5f, 1, 0},
    {0, 1, 0.5f},
    {0.5f, 0, 1},
    //{0, 0.5f, 1},
};

inline b32
IsEndOfLine(char C)
{
    b32 Result = ((C == '\n') ||
                  (C == '\r'));

    return(Result);
}

inline b32
IsWhitespace(char C)
{
    b32 Result = ((C == ' ') ||
                  (C == '\t') ||
                  (C == '\v') ||
                  (C == '\f') ||
                  IsEndOfLine(C));

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
    b32 Result = false;

    if(B)
    {
        char *At = B;
        for(umm Index = 0; Index < ALength; ++Index, ++At)
        {
            if((*At == 0) || (A[Index] != *At))
            {
                return(false);
            }
        }

        Result = (*At == 0);
    }
    else
    {
        Result = (ALength == 0);
    }

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

internal s32
S32FromZInternal(char **AtInit)
{
    s32 Result = 0;

    char *At = *AtInit;
    while((*At >= '0') && (*At <= '9'))
    {
        Result *= 10;
        Result += (*At - '0');
        ++At;
    }

    *AtInit = At;

    return(Result);
}

internal s32
S32FromZ(char *At)
{
    char *Ignored = At;
    s32 Result = S32FromZInternal(&At);
    return(Result);
}

struct format_dest
{
    umm Size;
    char *At;
};

inline void
OutChar(format_dest *Dest, char Value)
{
    if(Dest->Size)
    {
        --Dest->Size;
        *Dest->At++ = Value;
    }
}

inline u64
ReadVarArgUnsignedInteger(u32 Length, va_list *ArgList)
{
    u64 Result = 0;
    switch(Length)
    {
        case 1:
        {
            Result = va_arg(*ArgList, u8);
        } break;

        case 2:
        {
            Result = va_arg(*ArgList, u16);
        } break;

        case 4:
        {
            Result = va_arg(*ArgList, u32);
        } break;

        case 8:
        {
            Result = va_arg(*ArgList, u64);
        } break;
    }

    return(Result);
}

inline s64
ReadVarArgSignedInteger(u32 Length, va_list *ArgList)
{
    u64 Temp = ReadVarArgUnsignedInteger(Length, ArgList);
    s64 Result = *(s64 *)&Temp;
    return(Result);
}

inline f64
ReadVarArgFloat(u32 Length, va_list *ArgList)
{
    f64 Result = 0;
    switch(Length)
    {
        case 4:
        {
            Result = va_arg(*ArgList, f32);
        } break;

        case 8:
        {
            Result = va_arg(*ArgList, f64);
        } break;
    }

    return(Result);
}

internal umm
FormatStringList(umm DestSize, char *DestInit, char *Format, va_list ArgList)
{
    format_dest Dest = {DestSize, DestInit};
    if(Dest.Size)
    {
        char *At = Format;
        while(At[0])
        {
            if(*At == '%')
            {
                ++At;

                b32 ForceSign = false;
                b32 PadWithZeros = false;
                b32 LeftJustify = false;
                b32 PositiveSignIsBlank = false;
                b32 AnnotateIfNotZero = false;

                b32 Parsing = true;

                //
                // NOTE: Handle the flags
                //
                Parsing = true;
                while(Parsing)
                {
                    switch(*At)
                    {
                        case '+': {ForceSign = true;} break;
                        case '0': {PadWithZeros = true;} break;
                        case '-': {LeftJustify = true;} break;
                        case ' ': {PositiveSignIsBlank = true;} break;
                        case '#': {AnnotateIfNotZero = true;} break;
                        default: {Parsing = false;} break;
                    }

                    if(Parsing)
                    {
                        ++At;
                    }
                }

                //
                // NOTE: Handle the width
                //
                b32 WidthSpecified = false;
                s32 Width = 0;
                if(*At == '*')
                {
                    Width = va_arg(ArgList, int);
                    WidthSpecified = true;
                    ++At;
                }
                else if((*At >= '0') && (*At <= '9'))
                {
                    Width = S32FromZInternal(&At);
                    WidthSpecified = true;
                }

                //
                // NOTE: Handle the precision
                //
                b32 PrecisionSpecified = false;
                s32 Precision = 0;
                if(*At == '.')
                {
                    ++At;

                    if(*At == '*')
                    {
                        Precision = va_arg(ArgList, int);
                        PrecisionSpecified = true;
                        ++At;
                    }
                    else if((*At >= '0') && (*At <= '9'))
                    {
                        Precision = S32FromZInternal(&At);
                        PrecisionSpecified = true;
                    }
                    else
                    {
                        Assert(!"Malformed precision specifier!");
                    }
                }

                //
                // NOTE: Handle the length
                //
                u32 IntegerLength = 4;
                u32 FloatLength = 8;
                // TODO: Actually set different values here
                if((At[0] == 'h') && (At[1] == 'h'))
                {
                    At += 2;
                }
                else if((At[0] == 'l') && (At[1] == 'l'))
                {
                    At += 2;
                }
                else if(*At == 'h')
                {
                    ++At;
                }
                else if(*At == 'l')
                {
                    ++At;
                }
                else if(*At == 'j')
                {
                    ++At;
                }
                else if(*At == 'z')
                {
                    ++At;
                }
                else if(*At == 't')
                {
                    ++At;
                }
                else if(*At == 'L')
                {
                    ++At;
                }

                switch(*At)
                {
                    case 'd':
                    case 'i':
                    {
                        s64 Value = ReadVarArgSignedInteger(IntegerLength, &ArgList);
                    } break;

                    case 'u':
                    {
                        u64 Value = ReadVarArgUnsignedInteger(IntegerLength, &ArgList);
                    } break;

                    case 'o':
                    {
                        u64 Value = ReadVarArgUnsignedInteger(IntegerLength, &ArgList);
                    } break;

                    case 'x':
                    {
                        u64 Value = ReadVarArgUnsignedInteger(IntegerLength, &ArgList);
                    } break;

                    case 'X':
                    {
                        u64 Value = ReadVarArgUnsignedInteger(IntegerLength, &ArgList);
                    } break;

                    case 'f':
                    {
                        f64 Value = ReadVarArgFloat(FloatLength, &ArgList);
                    } break;

                    case 'F':
                    {
                        f64 Value = ReadVarArgFloat(FloatLength, &ArgList);
                    } break;

                    case 'e':
                    {
                        f64 Value = ReadVarArgFloat(FloatLength, &ArgList);
                    } break;

                    case 'E':
                    {
                        f64 Value = ReadVarArgFloat(FloatLength, &ArgList);
                    } break;

                    case 'g':
                    {
                        f64 Value = ReadVarArgFloat(FloatLength, &ArgList);
                    } break;

                    case 'G':
                    {
                        f64 Value = ReadVarArgFloat(FloatLength, &ArgList);
                    } break;

                    case 'a':
                    {
                        f64 Value = ReadVarArgFloat(FloatLength, &ArgList);
                    } break;

                    case 'A':
                    {
                        f64 Value = ReadVarArgFloat(FloatLength, &ArgList);
                    } break;

                    case 'c':
                    {
                        // TODO: How much is suppose to be read here?
                        int Value = va_arg(ArgList, int);
                    } break;

                    case 's':
                    {
                        char *String = va_arg(ArgList, char *);

                        for(char *Source = String; *Source; ++Source)
                        {
                            OutChar(&Dest, *Source);
                        }
                    } break;

                    case 'p':
                    {
                        void *Value = va_arg(ArgList, void *);
                    } break;

                    case 'n':
                    {
                        int *TabDest = va_arg(ArgList, int *);
                    } break;

                    case '%':
                    {
                        OutChar(&Dest, '%');
                    } break;

                    default:
                    {
                        Assert(!"Unrecognised format specifier");
                    } break;
                }

                if(*At)
                {
                    ++At;
                }
            }
            else
            {
                OutChar(&Dest, *At++);
            }
        }

        if(Dest.Size)
        {
            Dest.At[0] = 0;
        }
        else
        {
            Dest.At[-1] = 0;
        }
    }

    umm Result = Dest.At - DestInit;
    return(Result);
}

internal umm
FormatString(umm DestSize, char *Dest, char *Format, ...)
{
    va_list ArgList;

    va_start(ArgList, Format);
    umm Result = FormatStringList(DestSize, Dest, Format, ArgList);
    va_end(ArgList);

    return(Result);
}
