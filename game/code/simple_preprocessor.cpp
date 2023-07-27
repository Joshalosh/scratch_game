#include <stdio.h>
#include <stdlib.h>

static char *
ReadEntireFileIntoMemoryAndNullTerminate(char *Filename)
{
    char *Result = 0;

    FILE *File = fopen(Filename, "r");
    if(file)
    {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result = (char *)malloc(Filesize + 1);
        fread(Result, FileSize, 1, File);
        Result[FileSize] = 0;

        fclose(File);
    }

    return(Result);
}

enum token_type
{
    Token_Identifier,
    Token_OpenParen,
    Token_Colon,
    Token_String,
    Token_CloseParen,
    Token_SemiColon,
    Token_Asterisk,
    Token_OpenBacket,
    Token_CloseBracket,
    Token_OpenBraces,
    Token_CloseBraces,

    Token_EnOfStream,

};
struct token 
{
    token_type Type;

    int TextLength;
    char *Text;
};

struct tokeniser 
{
    char *At;
};

inline bool
IsWhiteSpace(char C) 
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   (C == '\n') ||
                   (C == '\r'));
    return(Result);
            
}

static void
EatAllWhiteSpace(tokeniser *Tokeniser)
{
    while (IsWhiteSpace(Tokeniser.At[0]))
    {
        ++Tokeniser.At;
    }
}

static token
GetToken(tokeniser *Tokeniser)
{
    EatAllWhiteSpace(Tokeniser);
}

int 
main(int ArgCount, char **Args)
{
    char *FileContents = ReadEntireFileIntoMemoryAndNullTerminate("handmade_sim_region.h");

    tokeniser Tokeniser = {};
    Tokeniser.At = FileContents;

    bool Parsing = true;
    while(Parsing)
    {
        token Token = GetToken(&Tokeniser);
        switch(Token.Type)
        {
            default:
            {
                printf("%d: %.*s\n", Token.Type, Token.TextLength, Token.Text);
            } break;

            case Token_EndOfStream:
            {
                Parsing = false;
            } break;
        
        }
    }
}
