#include "kengine_platform.h"
#include "kengine_shared.h"
#include "win32_kengine_shared.h"

typedef enum c_token_type
{
    CToken_Unknown,
    
    CToken_OpenParen,    
    CToken_CloseParen,    
    CToken_Colon,    
    CToken_Semicolon,    
    CToken_Asterisk,    
    CToken_OpenBracket,    
    CToken_CloseBracket,    
    CToken_OpenBrace,    
    CToken_CloseBrace,
    
    CToken_String,
    CToken_Identifier,
    
    CToken_EndOfStream
} c_token_type;

typedef struct c_token
{
    c_token_type Type;
    
    string Str;
    
} c_token;

typedef struct c_tokenizer
{
    memory_arena *Arena;
    char *At;
} c_tokenizer;

inline void
EatWhitespaceAndComments(c_tokenizer *Tokenizer)
{
    for(;;)
    {
        if(IsWhitespace(Tokenizer->At[0]))
        {
            ++Tokenizer->At;
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '/'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
            {
                ++Tokenizer->At;
            }
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '*'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] &&
                  !((Tokenizer->At[0] == '*') &&
                    (Tokenizer->At[1] == '/')))
            {
                ++Tokenizer->At;
            }
            
            if(Tokenizer->At[0] == '*')
            {
                Tokenizer->At += 2;
            }
        }
        else
        {
            break;
        }
    }
}

inline c_token
GetNextCTokenInternal(c_tokenizer *Tokenizer)
{
    EatWhitespaceAndComments(Tokenizer);
    
    c_token Result;
    ZeroStruct(Result);
    
    Result.Str = StringInternal(1, (u8 *)Tokenizer->At);
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    switch(C)
    {
        case '\0': { Result.Type = CToken_EndOfStream; } break;
        
        case '(': {Result.Type = CToken_OpenParen;} break;
        case ')': {Result.Type = CToken_CloseParen;} break;
        case ':': {Result.Type = CToken_Colon;} break;
        case ';': {Result.Type = CToken_Semicolon;} break;
        case '*': {Result.Type = CToken_Asterisk;} break;
        case '[': {Result.Type = CToken_OpenBracket;} break;
        case ']': {Result.Type = CToken_CloseBracket;} break;
        case '{': {Result.Type = CToken_OpenBrace;} break;
        case '}': {Result.Type = CToken_CloseBrace;} break;
        
        case '"':
        {
            Result.Type = CToken_String;
            
            Result.Str.Data = (u8 *)Tokenizer->At;
            
            while(Tokenizer->At[0] &&
                  Tokenizer->At[0] != '"')
            {
                if((Tokenizer->At[0] == '\\') &&
                   Tokenizer->At[1])
                {
                    ++Tokenizer->At;
                }                
                ++Tokenizer->At;
            }
            
            Result.Str.Length = Tokenizer->At - (char *)Result.Str.Data;
            if(Tokenizer->At[0] == '"')
            {
                ++Tokenizer->At;
            }
        } break;
        
        default:
        {
            if(IsAlpha(C))
            {
                Result.Type = CToken_Identifier;
                
                while(IsAlpha(Tokenizer->At[0]) ||
                      IsNumber(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '_'))
                {
                    ++Tokenizer->At;
                }
                
                Result.Str.Length = Tokenizer->At - (char *)Result.Str.Data;
            }
#if 0
            else if(IsNumber(C))
            {
                ParseNumber();
            }
#endif
            else
            {
                Result.Type = CToken_Unknown;
            }
        } break;        
        
    }
    return Result;
}

inline b32
RequireCToken(c_tokenizer *Tokenizer, c_token_type DesiredType)
{
    c_token Token = GetNextCTokenInternal(Tokenizer);
    b32 Result = (Token.Type == DesiredType);
    return Result;
}

inline void
ParseIntrospectable(c_tokenizer *Tokenizer)
{
    if(RequireCToken(Tokenizer, CToken_OpenParen))
    {
        b32 IsCtor = false;
        b32 IsSet1 = false;
        b32 IsMath = false;
        
        // NOTE(kstandbridge): Parse params
        c_token Token;
        ZeroStruct(Token);
        while((Token.Type != CToken_EndOfStream) &&
              (Token.Type != CToken_CloseParen))
        {
            Token = GetNextCTokenInternal(Tokenizer);
            if(Token.Type == CToken_Identifier)
            {
                if(StringsAreEqual(String("ctor"), Token.Str))
                {
                    IsCtor = true;
                }
                else if(StringsAreEqual(String("set1"), Token.Str))
                {
                    IsSet1 = true;
                }
                else if(StringsAreEqual(String("math"), Token.Str))
                {
                    IsMath = true;
                }
            }
        }
        
        char *Start = 0;
        string StructName;
        Token = GetNextCTokenInternal(Tokenizer);
        Assert(StringsAreEqual(String("typedef"), Token.Str));
        Token = GetNextCTokenInternal(Tokenizer);
        Assert(StringsAreEqual(String("struct"), Token.Str));
        Token = GetNextCTokenInternal(Tokenizer);
        StructName = Token.Str;
        // TODO(kstandbridge): UpperCamelCase function?
        string StructUpperCamelCase = PushStringInternal(Tokenizer->Arena, Token.Str.Length, Token.Str.Data);
        StructUpperCamelCase.Data[0] = ToUppercase(StructUpperCamelCase.Data[0]);
        if(RequireCToken(Tokenizer, CToken_OpenBrace))
        {
            Start = Tokenizer->At;
            
            if(IsCtor)
            {
                ConsoleOut(Tokenizer->Arena, "inline %S\n%S(", StructName, StructUpperCamelCase);
                b32 FirstParam = true;
                for(;;)
                {
                    Token = GetNextCTokenInternal(Tokenizer);
                    if((Token.Type == CToken_EndOfStream) ||
                       (Token.Type == CToken_CloseBrace))
                    {
                        ConsoleOut(Tokenizer->Arena, ")\n{\n    %S Result;\n\n", StructName);
                        break;
                    }
                    else if(Token.Type == CToken_Identifier)
                    {
                        if(!FirstParam)
                        {
                            ConsoleOut(Tokenizer->Arena, ", ");
                            FirstParam = false;
                        }
                        string Type = Token.Str;
                        Token = GetNextCTokenInternal(Tokenizer);
                        ConsoleOut(Tokenizer->Arena, "%S %S", Type, Token.Str);
                    }
                }
                
                Tokenizer->At = Start;
                
                for(;;)
                {
                    Token = GetNextCTokenInternal(Tokenizer);
                    if((Token.Type == CToken_EndOfStream) ||
                       (Token.Type == CToken_CloseBrace))
                    {
                        ConsoleOut(Tokenizer->Arena, "\n    return Result;\n}\n\n");
                        break;
                    }
                    else if(Token.Type == CToken_Identifier)
                    {
                        Token = GetNextCTokenInternal(Tokenizer);
                        ConsoleOut(Tokenizer->Arena, "    Result.%S = %S;\n", Token.Str, Token.Str);
                    }
                }
                
            }
            
            if(IsSet1)
            {
                Tokenizer->At = Start;
                
                ConsoleOut(Tokenizer->Arena, "inline %S\n%SSet1(", StructName, StructUpperCamelCase);
                b32 FirstParam = true;
                for(;;)
                {
                    Token = GetNextCTokenInternal(Tokenizer);
                    if((Token.Type == CToken_EndOfStream) ||
                       (Token.Type == CToken_CloseBrace))
                    {
                        ConsoleOut(Tokenizer->Arena, ")\n{\n    %S Result;\n\n", StructName);
                        break;
                    }
                    else if(Token.Type == CToken_Identifier)
                    {
                        if(FirstParam)
                        {
                            FirstParam = false;
                            string Type = Token.Str;
                            Token = GetNextCTokenInternal(Tokenizer);
                            ConsoleOut(Tokenizer->Arena, "%S Value", Type);
                        }
                    }
                }
                
                Tokenizer->At = Start;
                
                for(;;)
                {
                    Token = GetNextCTokenInternal(Tokenizer);
                    if((Token.Type == CToken_EndOfStream) ||
                       (Token.Type == CToken_CloseBrace))
                    {
                        ConsoleOut(Tokenizer->Arena, "\n    return Result;\n}\n\n");
                        break;
                    }
                    else if(Token.Type == CToken_Identifier)
                    {
                        Token = GetNextCTokenInternal(Tokenizer);
                        ConsoleOut(Tokenizer->Arena, "    Result.%S = Value;\n", Token.Str);
                    }
                }
                
            }
            
        }
        else
        {
            // TODO(kstandbridge): Error missing open brace on introspect
        }
        
        
    }
    else
    {
        // TODO(kstandbridge): Error missing paren on introspect
    }
}

s32 __stdcall
mainCRTStartup()
{
    memory_arena ArenaInternal = AllocateArena(Megabytes(8));
    memory_arena *Arena = &ArenaInternal;
    
    ConsoleOut(Arena, "// NOTE(kstandbridge): This file is automatically generated by win32_kengine_prerocessor\n");
    ConsoleOut(Arena, "// Do not make any changes to this file, as it will be replaced upon build!\n\n");
    
    string File = ReadEntireFile(Arena, "kengine_math.h");
    
    c_tokenizer Tokenizer;
    ZeroStruct(Tokenizer);
    Tokenizer.Arena = Arena;
    Tokenizer.At = (char *)File.Data;
    
    b32 Parsing = true;
    while(Parsing)
    {
        c_token Token = GetNextCTokenInternal(&Tokenizer);
        switch(Token.Type)
        {
            case CToken_EndOfStream:
            {
                Parsing = false;
            } break;
            
            case CToken_Unknown:
            {
            } break;
            
            case CToken_Identifier:
            {
                if(StringsAreEqual(String("introspect"), Token.Str))
                {
                    ParseIntrospectable(&Tokenizer);
                }
            } break;
        }
    }
    
    
    
    ExitProcess(EXIT_SUCCESS);
}