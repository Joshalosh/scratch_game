
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

typedef char unsigned u8;

enum stat_type
{
    Stat_Literal,
    Stat_Repeat,
    Stat_Copy,

    Stat_LiteralBytes,
    Stat_NonLiteralBytes,

    Stat_Count,
};
struct stat 
{
    size_t Count;
    size_t Total;
};

struct stat_group
{
    size_t UncompressedBytes;
    size_t CompressedBytes;
    stat Stats[Stat_Count];
};

struct file_contents
{
    size_t FileSize;
    u8 *Contents;
};

#define COMPRESS_HANDLER(name) size_t name(size_t InSize, u8 *InBase, size_t MaxOutSize, u8 *OutBase)
#define DECOMPRESS_HANDLER(name) void name(size_t InSize, u8 *InBase, size_t OutSize, u8 *OutBase)

typedef COMPRESS_HANDLER(compress_handler);
typedef DECOMPRESS_HANDLER(decompress_handler);

struct compressor
{
    char *Name;
    compress_handler *Compress;
    decompress_handler *Decompress;
};

static file_contents
ReadEntireFileIntoMemory(char *Filename)
{
    file_contents Result = {};
    
    FILE *File = fopen(Filename, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        Result.FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result.Contents = (u8 *)malloc(Result.FileSize);
        fread(Result.Contents, Result.FileSize, 1, File);

        fclose(File);
    }
    else 
    {
        fprintf(stderr, "Unable to read file %s\n", Filename);
    }

    return (Result);
}

static size_t
GetMaximumCompressedOutputSize(size_t InSize)
{
    // TODO: Actually figure out the equation for _our_ compressor
    size_t OutSize = 256 + 8*InSize;
    return(OutSize);
}

static void
Copy(size_t Size, u8 *Source, u8 *Dest)
{
    while(Size--)
    {
        *Dest++ = *Source++;
    }
}

COMPRESS_HANDLER(RLECompress)
{
    u8 *Out = OutBase;

#define MAX_LITERAL_COUNT 255
#define MAX_RUN_COUNT 255
    int LiteralCount = 0;
    u8 Literals[MAX_LITERAL_COUNT];

    u8 *In = InBase;
    u8 *InEnd = In + InSize;
    while(In < InEnd)
    {
        u8 StartingValue = In[0];
        size_t Run = 1;
        while((Run < (size_t)(InEnd - In)) &&
              (Run < MAX_RUN_COUNT) &&
              (In[Run] == StartingValue))
        {
            ++Run;
        }

        if((Run > 1) || (LiteralCount == MAX_LITERAL_COUNT))
        {
            // Output a literal/run pair
            u8 LiteralCount8 = (u8)LiteralCount;
            assert(LiteralCount8 == LiteralCount);
            *Out++ = LiteralCount8;

            for(int LiteralIndex = 0; LiteralIndex < LiteralCount; ++LiteralIndex)
            {
                *Out++ = Literals[LiteralIndex];
            }
            LiteralCount = 0;

            u8 Run8 = (u8)Run;
            assert(Run8 == Run);
            *Out++ = Run8;

            *Out++ = StartingValue;

            In += Run;
        }
        else 
        {
            // Buffer literals
            Literals[LiteralCount++] = StartingValue;

            ++In;
        }
    }
#undef MAX_LITERAL_COUNT
#undef MAX_RUN_COUNT

    assert(In == InEnd);

    size_t OutSize = Out - OutBase;
    assert(OutSize <= MaxOutSize);

    return(OutSize);
}

DECOMPRESS_HANDLER(RLEDecompress)
{
    u8 *Out = OutBase;
    u8 *In = InBase;
    u8 *InEnd = In + InSize;
    while(In < InEnd)
    {
        int LiteralCount = *In++;
        while(LiteralCount--)
        {
            *Out++ = *In++;
        }

        int RepCount = *In++;
        u8 RepValue = *In++;
        while(RepCount--)
        {
            *Out++ = RepValue;
        }
    }

    assert(In == InEnd);
}

COMPRESS_HANDLER(LZCompress)
{
    u8 *Out = OutBase;
    u8 *In = InBase;

#define MAX_LOOKBACK_COUNT 255
#define MAX_LITERAL_COUNT 255
#define MAX_RUN_COUNT 255
    int LiteralCount = 0;
    u8 Literals[MAX_LITERAL_COUNT];

    u8 *InEnd = In + InSize;
    while(In < InEnd)
    {
        size_t MaxLookback = In - InBase;
        if(MaxLookback > MAX_LOOKBACK_COUNT)
        {
            MaxLookback = MAX_LOOKBACK_COUNT;
        }

        size_t BestRun = 0;
        size_t BestDistance = 0;
        for(u8 *WindowStart = In - MaxLookback; WindowStart < In; ++WindowStart)
        {
            size_t WindowSize = InEnd - WindowStart;
            if(WindowSize > MAX_RUN_COUNT)
            {
                WindowSize = MAX_RUN_COUNT;
            }

            u8 *WindowEnd = WindowStart + WindowSize;
            u8 *TestIn = In;
            u8 *WindowIn = WindowStart;
            size_t TestRun = 0;
            while((WindowIn < WindowEnd) && (*TestIn++ == *WindowIn++))
            {
                ++TestRun;
            }

            if(BestRun < TestRun)
            {
                BestRun = TestRun;
                BestDistance = In - WindowStart;
            }
        }

        bool OutputRun = false;
        if(LiteralCount)
        {
            OutputRun = (BestRun > 4);
        }
        else 
        {
            OutputRun = (BestRun > 2);
        }

        if(OutputRun || (LiteralCount == MAX_LITERAL_COUNT))
        {
            // Flush
            u8 LiteralCount8 = (u8)LiteralCount;
            assert(LiteralCount8 == LiteralCount);
            if(LiteralCount8)
            {
                *Out++ = LiteralCount8;
                *Out++ = 0;

                for(int LiteralIndex = 0; LiteralIndex < LiteralCount; ++LiteralIndex)
                {
                    *Out++ = Literals[LiteralIndex];
                }
                LiteralCount = 0;
            }

            if(OutputRun)
            {
                u8 Run8 = (u8)BestRun;
                assert(Run8 == BestRun);
                *Out++ = Run8;
                *Out++ = 1;

                u8 Distance8 = (u8)BestDistance;
                assert(Distance8 == BestDistance);
                *Out++ = Distance8;

                In += BestRun;
            }
        }
        else 
        {
            // Buffer literals
            Literals[LiteralCount++] = *In++;
        }
    }
#undef MAX_LITERAL_COUNT
#undef MAX_RUN_COUNT

    assert(In == InEnd);

    size_t OutSize = Out - OutBase;
    assert(OutSize <= MaxOutSize);

    return(OutSize);
}

DECOMPRESS_HANDLER(LZDecompress)
{
    u8 *Out = OutBase;
    u8 *In = InBase;
    u8 *InEnd = In + InSize;
    while(In < InEnd)
    {
        int Count = *In++;
        u8 CopyDistance = *In++;

        u8 *Source = Out - CopyDistance;
        if(CopyDistance == 0)
        {
            Source = In;
            In += Count;
        }

        while(Count--)
        {
            *Out++ = *Source++;
        }
    }

    assert(In == InEnd);
}

compressor Compressors[] =
{
    {"rle", RLECompress, RLEDecompress},
    {"lz", LZCompress, LZDecompress},
};

int 
main(int ArgCount, char **Args)
{
    if(ArgCount == 5)
    {
        size_t FinalOutputSize = 0;
        u8 *FinalOutputBuffer = 0;

        char *CodecName = Args[1];
        char *Command = Args[2];
        char *InFilename = Args[3];
        char *OutFilename = Args[4];

        compressor *Compressor = 0;
        for(int CompressorIndex = 0; CompressorIndex < (sizeof(Compressors)/sizeof(Compressors[0])); ++CompressorIndex)
        {
            compressor *TestCompressor = Compressors + CompressorIndex;
            if(strcmp(CodecName, Compressor->Name) == 0)
            {
                Compressor = TestCompressor;
                break;
            }
        }

        if(Compressor)
        {
            file_contents InFile = ReadEntireFileIntoMemory(InFilename);
            if(strcmp(Command, "compress") == 0)
            {
                size_t OutBufferSize = GetMaximumCompressedOutputSize(InFile.FileSize);
                u8 *OutBuffer = (u8 *)malloc(OutBufferSize + 4);
                size_t CompressedSize = Compressor->Compress(InFile.FileSize, InFile.Contents, OutBufferSize, OutBuffer + 4);
                *(int unsigned *)OutBuffer = (int unsigned)InFile.FileSize;

                FinalOutputSize = CompressedSize + 4;
                FinalOutputBuffer = OutBuffer;
            }
            else if(strcmp(Command, "decompress") == 0)
            {
                if(InFile.FileSize >= 4)
                {
                    size_t OutBufferSize = *(int unsigned *)InFile.Contents;
                    u8 *OutBuffer = (u8 *)malloc(OutBufferSize);
                    Compressor->Decompress(InFile.FileSize - 4, InFile.Contents + 4, OutBufferSize, OutBuffer);

                    FinalOutputSize = OutBufferSize;
                    FinalOutputBuffer = OutBuffer;
                }
                else 
                {
                    fprintf(stderr, "Invalid input file\n");
                }
            }
            else if(strcmp(Command, "test") == 0)
            {
                size_t OutBufferSize = GetMaximumCompressedOutputSize(InFile.FileSize);
                u8 *OutBuffer = (u8 *)malloc(OutBufferSize);
                u8 *TestBuffer = (u8 *)malloc(InFile.FileSize);
                size_t CompressedSize = Compressor->Compress(InFile.FileSize, InFile.Contents, OutBufferSize, OutBuffer);
                Compressor->Decompress(CompressedSize, OutBuffer, InFile.FileSize, TestBuffer);
                if(memcmp(InFile.Contents, TestBuffer, InFile.FileSize) == 0)
                {
                    fprintf(stderr, "Success!\n");
                }
                else 
                {
                    fprintf(stderr, "Failure :(\n");
                }
            }
            else 
            {
                fprintf(stderr, "Unrecognised command %s\n", Command);
            }
        }
        else 
        {
            fprintf(stderr, "Unrecognised compressor %s\n", CodecName);
        }

        if(FinalOutputBuffer)
        {
            FILE *OutFile = fopen(OutFilename, "wb");
            if(OutFile)
            {
                fwrite(FinalOutputBuffer, 1, FinalOutputSize, OutFile);
            }
            else 
            {
                fprintf(stderr, "Unable to open output file %s\n", OutFilename);
            }
        }
    }
    else 
    {
        fprintf(stderr, "Usage: %s compress [raw filename] [compressed filename]\n", Args[0]);
        fprintf(stderr, "       %s decompress [compressed filename] [raw filename]\n", Args[0]);
    }
}
