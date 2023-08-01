#include <stdio.h>
#include <stdlib.h>

static char *
ReadEntireFileIntoMemoryAndNullTerminate(char *Filename)
{
    char *Result = 0;

    FILE *File = fopen(Filename, "r");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result = (char *)malloc(FileSize + 1);
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
    Token_Semicolon,
    Token_Asterisk,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,

    Token_String,
    Token_Identifier,

    Token_EndOfStream,
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

    return(Result);
}

inline bool
IsWhitespace(char C)
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   (C == '\v') ||
                   (C == '\f') ||
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

inline bool
TokenEquals(token Token, char *Match)
{
    char *At = Match;
    for(int Index = 0; Index < Token.TextLength; ++Index, ++At)
    {
        if((*At == 0) ||
           (Token.Text[Index] != *At))
        {
            return(false);
        }
    }

    bool Result = (*At == 0);
    return(Result);
}

static void
EatAllWhitespace(tokeniser *Tokeniser)
{
    for(;;)
    {
        if(IsWhitespace(Tokeniser->At[0]))
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
    EatAllWhitespace(Tokeniser);

    token Token = {};
    Token.TextLength = 1;
    Token.Text = Tokeniser->At;
    char C = Tokeniser->At[0];
    ++Tokeniser->At;
    switch(C)
    {
        case '\0': {Token.Type = Token_EndOfStream;} break;

        case '(': {Token.Type = Token_OpenParen;} break;
        case ')': {Token.Type = Token_CloseParen;} break;
        case ':': {Token.Type = Token_Colon;} break;
        case ';': {Token.Type = Token_Semicolon;} break;
        case '*': {Token.Type = Token_Asterisk;} break;
        case '[': {Token.Type = Token_OpenBracket;} break;
        case ']': {Token.Type = Token_CloseBracket;} break;
        case '{': {Token.Type = Token_OpenBrace;} break;
        case '}': {Token.Type = Token_CloseBrace;} break;

        case '"':
        {
            Token.Type = Token_String;

            Token.Text = Tokeniser->At;

            while(Tokeniser->At[0] &&
                  Tokeniser->At[0] != '"')
            {
                if((Tokeniser->At[0] == '\\') &&
                   Tokeniser->At[1])
                {
                    ++Tokeniser->At;
                }
                ++Tokeniser->At;
            }

            Token.TextLength = Tokeniser->At - Token.Text;
            if(Tokeniser->At[0] == '"')
            {
                ++Tokeniser->At;
            }
        } break;

        default:
        {
            if(IsAlpha(C))
            {
                Token.Type = Token_Identifier;

                while(IsAlpha(Tokeniser->At[0]) ||
                      IsNumber(Tokeniser->At[0]) ||
                      (Tokeniser->At[0] == '_'))
                {
                    ++Tokeniser->At;
                }

                Token.TextLength = Tokeniser->At - Token.Text;
            }
#if 0
            else if(IsNumeric(C))
            {
                ParseNumber();
            }
#endif
            else 
            {
                Token.Type = Token_Unknown;
            }
        } break;
    }

    return(Token);
}

static bool
RequireToken(tokeniser *Tokeniser, token_type DesiredType)
{
    token Token = GetToken(Tokeniser);
    bool Result = (Token.Type == DesiredType);
    return(Result);
}

static void
ParseIntrospectionParams(tokeniser *Tokeniser)
{
    for(;;)
    {
        token Token = GetToken(Tokeniser);
        if((Token.Type == Token_CloseParen) ||
           (Token.Type == Token_EndOfStream))
        {
            break;
        }
    }
}

static void
ParseMember(tokeniser *Tokeniser, token MemberTypeToken)
{
#if 1
    bool Parsing = true;
    bool IsPointer = false;
    while(Parsing)
    {
        token Token = GetToken(Tokeniser);
        switch(Token.Type)
        {
            case Token_Asterisk:
            {
                IsPointer = true;
            } break;

            case Token_Identifier:
            {
                printf("{Type_%.*s, \"%.*s\"},\n",
                       (int)MemberTypeToken.TextLength, MemberTypeToken.Text,
                       (int)Token.TextLength, Token.Text);
            } break;

            case Token_Semicolon:
            case Token_EndOfStream:
            {
                Parsing = false;
            } break;
        }
    }
#else
    token Token = GetToken(Tokeniser);
    switch(Token.Type)
    {
        case Token_Asterisk:
        {
            ParseMember(Tokeniser, Token);
        } break;

        case Token_Identifier:
        {
            printf("DEBUG_VALUE(%.*s);\n", (int)Token.TextLength, Token.Text);
        } break;
    }
#endif
}

static void
ParseStruct(tokeniser *Tokeniser)
{
    token NameToken = GetToken(Tokeniser);
    if(RequireToken(Tokeniser, Token_OpenBrace))
    {

        printf("member_definition MembersOf_%.*s[] = \n", (int)NameToken.TextLength, NameToken.Text);
        printf("{\n");
        for(;;)
        {
            token MemberToken = GetToken(Tokeniser);
            if(MemberToken.Type == Token_CloseBrace)
            {
                break;
            }
            else
            {
                ParseMember(Tokeniser, MemberToken);
            }
        }
        printf("}\n");
    }
}

static void
ParseIntrospectable(tokeniser *Tokeniser)
{
    if(RequireToken(Tokeniser, Token_OpenParen))
    {
        ParseIntrospectionParams(Tokeniser);

        token TypeToken = GetToken(Tokeniser);
        if(TokenEquals(TypeToken, "struct"))
        {
            ParseStruct(Tokeniser);
        }
        else
        {
            fprintf(stderr, "ERROR: Introspection is only supported for structs right now :(\n");
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Missing parentheses.\n");
    }
}

int
main(int ArgCount, char **Args)
{
    char *FileContents = ReadEntireFileIntoMemoryAndNullTerminate("game_sim_region.h");

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

            case Token_Identifier:
            {
                if(TokenEquals(Token, "introspect"))
                {
                    ParseIntrospectable(&Tokeniser);
                }
            } break;

            default:
            {
                //printf("%d: %.*s\n", Token.Type, (int)Token.TextLength, Token.Text);
            } break;
        }
    }
}
