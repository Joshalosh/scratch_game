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
    Token_Unknown,

    Token_OpenParen,
    Token_CloseParen,
    Token_Colon,
    Token_SemiColon,
    Token_Asterisk,
    Token_OpenBacket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,

    Token_String,
    Token_Identifier,

    Token_EnOfStream,

};
struct token 
{
    token_type Type;

    size_t TextLength;
    char *Text;
};

struct tokeniser 
{
    char *At;
};

inline bool
IsEndOfLine(char C) 
{
    bool Result = ((C == '\n') ||
                   (C == '\r'));
    return Result;
}

inline bool
IsWhiteSpace(char C) 
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   IsEndOfLine(C));

    return(Result);
}

inline bool
IsAlpha(char C) 
{
    bool Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')));

    return(Result);
}

inline bool 
IsNumber(char C) 
{
    bool Result = ((C >= '0') && (C <= '9'));

    return(Result);
}

static void
EatAllWhiteSpace(tokeniser *Tokeniser)
{
    for(;;)
    {
        while (IsWhiteSpace(Tokeniser->At[0]))
        {
            ++Tokeniser->At;
        }
        else if((Tokeniser->At[0] == '/') &&
                (Tokeniser->At[1] == '/'))
        {
            Tokeniser->At += 2;
            while(Tokeniser->At[0] && !IsEndOfLine(Tokeniser->At[0]))
            {
                ++Tokeniser->At;
            }
        }
        else if((Tokeniser->At[0] == '/') &&
                (Tokeniser->At[1] == '*'))
        {
            Tokeniser->At += 2;
            while(Tokeniser->At[0] &&
                  !((Tokeniser->At[0] == '*') &&
                    (Tokeniser->At[1] == '/')))
            {
                ++Tokeniser->At;
            }

            if(Tokeniser->At[0] == '*')
            {
                Tokeniser->At += 2;
            }
        }
        else 
        {
            break;
        }
    }
}

static token
GetToken(tokeniser *Tokeniser)
{
    EatAllWhiteSpace(Tokeniser);

    token Token = {};
    Token.TextLength = 1;
    Token.Text = Tokeniser->At;
    switch(Tokeniser->At[0])
    {
        case '\0': {Token.Type = Token_EndOfStream;} break;

        case '(': {Token.Type = Token_OpenParen;} break;
        case ')': {Token.Type = Token_CloseParen;} break;
        case ':': {Token.Type = Token_Colon;} break;
        case ';': {Token.Type = Token_SemiColon;} break;
        case '*': {Token.Type = Token_Asterisk;} break;
        case '[': {Token.Type = Token_OpenBracket;} break;
        case ']': {Token.Type = Token_CloseBracket;} break;
        case '{': {Token.Type = Token_OpenBrace;} break;
        case '}': {Token.Type = Token_CloseBrace;} break;

        case '"':
        {
            ++Tokeniser->At;
            Token.Text = Tokeniser->At;

            while(Tokeniser->At[0] &&
                  Tokeniser->At[0] != '"')
            {
                if((Tokenizer->At[0] == '\\') &&
                    Tokeniser->At[1])
                {
                    ++Tokeniser->At;
                }
                ++Tokeniser->At;
            }

            Token.Type = Token_String;
            Token.TextLength = Tokensizer->At - Token.Text;
            if(Tokeniser->At[0] == '"')
            {
                ++Tokeniser->At;
            }
        } break;

        default:
        {
            if(IsAlpha(Tokeniser->At[0]))
            {
                while(IsAlpha(Tokeniser->At[0]) ||
                      IsNumber(Tokeniser->At[0]) ||
                      (Tokeniser->At[0] == '_'))
                {
                    ++Tokeniser->At;
                }

                Token.TextLength = Tokeniser->At - Token.Text;
            }
#if 0
            else if(IsNumeric(Tokeniser->At[0]))
            {
                ParseNumber();
            }
#endif
            else 
            {
                Token.Type == Token_Unknown;
            }
        } break;
    }

    return(Token);
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
            case Token_EndOfStream:
            {
                Parsing = false;
            } break;

            case Token_Unknown:
            {
            } break;

            default:
            {
                printf("%d: %.*s\n", Token.Type, Token.TextLength, Token.Text);
            } break;
        }
    }
}
