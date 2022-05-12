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

typedef enum generate_method_op
{
    GenerateMethod_Ctor,
    GenerateMethod_Set1,
    GenerateMethod_MathAdd,
    GenerateMethod_MathSubtract,
    GenerateMethod_MathMultiply,
    GenerateMethod_MathDivide,
} generate_method_op;

inline void
GenerateMethod(c_tokenizer *Tokenizer, generate_method_op Op, string SnakeStruct, string UpperCamelStruct)
{
    char *Start = Tokenizer->At;;
    
    switch(Op)
    {
        case GenerateMethod_Ctor:
        {
            ConsoleOut(Tokenizer->Arena, "inline %S\n%S(", SnakeStruct, UpperCamelStruct);
        } break;
        case GenerateMethod_Set1:
        {
            ConsoleOut(Tokenizer->Arena, "inline %S\n%SSet1(", SnakeStruct, UpperCamelStruct);
        } break;
        case GenerateMethod_MathAdd:
        {
            ConsoleOut(Tokenizer->Arena, "inline %S\n%SAdd(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathSubtract:
        {
            ConsoleOut(Tokenizer->Arena, "inline %S\n%SSubtract(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathMultiply:
        {
            ConsoleOut(Tokenizer->Arena, "inline %S\n%SMultiply(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathDivide:
        {
            ConsoleOut(Tokenizer->Arena, "inline %S\n%SDivide(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        InvalidDefaultCase;
    }
    b32 FirstParam = true;
    for(;;)
    {
        c_token Token = GetNextCTokenInternal(Tokenizer);
        if((Token.Type == CToken_EndOfStream) ||
           (Token.Type == CToken_CloseBrace))
        {
            ConsoleOut(Tokenizer->Arena, ")\n{\n    %S Result;\n\n", SnakeStruct);
            break;
        }
        else if(Token.Type == CToken_Identifier)
        {
            string Type = Token.Str;
            Token = GetNextCTokenInternal(Tokenizer);
            
            switch(Op)
            {
                case GenerateMethod_Ctor:
                {
                    if(!FirstParam)
                    {
                        ConsoleOut(Tokenizer->Arena, ", ");
                    }
                    FirstParam = false;
                    ConsoleOut(Tokenizer->Arena, "%S %S", Type, Token.Str);
                } break;
                case GenerateMethod_Set1:
                {
                    if(FirstParam)
                    {
                        FirstParam = false;
                        ConsoleOut(Tokenizer->Arena, "%S Value", Type);
                    }
                } break;
                case GenerateMethod_MathAdd:
                case GenerateMethod_MathSubtract:
                case GenerateMethod_MathMultiply:
                case GenerateMethod_MathDivide:
                {
                } break;
                InvalidDefaultCase;
            }
        }
    }
    
    Tokenizer->At = Start;
    
    for(;;)
    {
        c_token Token = GetNextCTokenInternal(Tokenizer);
        if((Token.Type == CToken_EndOfStream) ||
           (Token.Type == CToken_CloseBrace))
        {
            ConsoleOut(Tokenizer->Arena, "\n    return Result;\n}\n\n");
            break;
        }
        else if(Token.Type == CToken_Identifier)
        {
            Token = GetNextCTokenInternal(Tokenizer);
            string Var = Token.Str;
            switch(Op)
            {
                case GenerateMethod_Ctor:
                {
                    ConsoleOut(Tokenizer->Arena, "    Result.%S = %S;\n", Var, Var);
                } break;
                case GenerateMethod_Set1:
                {
                    ConsoleOut(Tokenizer->Arena, "    Result.%S = Value;\n", Var);
                } break;
                case GenerateMethod_MathAdd:
                {
                    ConsoleOut(Tokenizer->Arena, "    Result.%S = A.%S + B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathSubtract:
                {
                    ConsoleOut(Tokenizer->Arena, "    Result.%S = A.%S - B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathMultiply:
                {
                    ConsoleOut(Tokenizer->Arena, "    Result.%S = A.%S * B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathDivide:
                {
                    ConsoleOut(Tokenizer->Arena, "    Result.%S = A.%S / B.%S;\n", Var, Var, Var);
                } break;
                InvalidDefaultCase;
            }
        }
    }
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
        
        if(RequireCToken(Tokenizer, CToken_Semicolon))
        {
            char *Start = 0;
            string StructName;
            Token = GetNextCTokenInternal(Tokenizer);
            Assert(StringsAreEqual(String("typedef"), Token.Str));
            Token = GetNextCTokenInternal(Tokenizer);
            Assert(StringsAreEqual(String("struct"), Token.Str));
            Token = GetNextCTokenInternal(Tokenizer);
            StructName = Token.Str;
            // TODO(kstandbridge): UpperCamelCase function?
            string UpperCamelStruct = PushStringInternal(Tokenizer->Arena, Token.Str.Length, Token.Str.Data);
            UpperCamelStruct.Data[0] = ToUppercase(UpperCamelStruct.Data[0]);
            if(RequireCToken(Tokenizer, CToken_OpenBrace))
            {
                Start = Tokenizer->At;
                
                if(IsCtor)
                {
                    GenerateMethod(Tokenizer, GenerateMethod_Ctor, StructName, UpperCamelStruct);
                }
                
                if(IsSet1)
                {
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_Set1, StructName, UpperCamelStruct);
                }
                
                if(IsMath)
                {
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_MathAdd, StructName, UpperCamelStruct);
                    
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_MathSubtract, StructName, UpperCamelStruct);
                    
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_MathMultiply, StructName, UpperCamelStruct);
                    
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_MathDivide, StructName, UpperCamelStruct);
                }
                
            }
            else
            {
                // TODO(kstandbridge): Error missing open brace on introspect
            }
        }
        else
        {
            // TODO(kstandbridge): Error missing semicolon on introspect
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