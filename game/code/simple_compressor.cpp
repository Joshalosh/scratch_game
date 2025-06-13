
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

typedef char unsigned u8;

struct file_contents
{
    size_t FileSize;
    u8 *Contents;
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
    size_t OutSize = 256 + 2 *InSize;
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

static size_t 
Compress(size_t InSize, u8 *In, size_t OutSize, u8 *Out)
{
    Copy(InSize, In, Out);
    return(InSize);
}

static void
Decompress(size_t InSize, u8 *In, size_t OutSize, u8 *Out)
{
    Copy(InSize, In, Out);
}

int 
main(int ArgCount, char **Args)
{
    if(ArgCount == 4)
    {
        size_t FinalOutputSize = 0;
        u8 *FinalOutputBuffer = 0;

        char *Command = Args[1];
        char *InFilename = Args[2];
        char *OutFilename = Args[3];

        file_contents InFile = ReadEntireFileIntoMemory(InFilename);
        if(strcmp(Command, "compress") == 0)
        {
            size_t OutBufferSize = GetMaximumCompressedOutputSize(InFile.FileSize);
            u8 *OutBuffer = (u8 *)malloc(OutBufferSize + 4);
            size_t CompressedSize = Compress(InFile.FileSize, InFile.Contents, OutBufferSize, OutBuffer + 4);
            *(int unsigned *)OutBuffer = (int unsigned)CompressedSize;

            FinalOutputSize = CompressedSize + 4;
            FinalOutputBuffer = OutBuffer;
        }
        else if(strcmp(Command, "decompress") == 0)
        {
            if(InFile.FileSize >= 4)
            {
                size_t OutBufferSize = *(int unsigned *)InFile.Contents;
                u8 *OutBuffer = (u8 *)malloc(OutBufferSize);
                Decompress(InFile.FileSize - 4, InFile.Contents + 4, OutBufferSize, OutBuffer);

                FinalOutputSize = OutBufferSize;
                FinalOutputBuffer = OutBuffer;
            }
            else 
            {
                fprintf(stderr, "Invalid input file\n");
            }
        }
        else 
        {
            fprintf(stderr, "Unrecognised command %s\n", Command);
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
