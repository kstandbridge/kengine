#ifndef KENGINE_STRING_H


inline b32
IsEndOfLine(char C)
{
    b32 Result = ((C == '\n') ||
                  (C == '\r'));
    
    return Result;
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
IsHex(char Char)
{
    b32 Result = (((Char >= '0') && (Char <= '9')) ||
                  ((Char >= 'A') && (Char <= 'F')));
    
    return Result;
}

inline u32
GetHex(char Char)
{
    u32 Result = 0;
    
    if((Char >= '0') && (Char <= '9'))
    {
        Result = Char - '0';
    }
    else if((Char >= 'A') && (Char <= 'F'))
    {
        Result = 0xA + (Char - 'A');
    }
    
    return Result;
}

inline b32
IsNumber(char C)
{
    b32 Result = ((C >= '0') && (C <= '9'));
    
    return Result;
}

inline b32
IsAlpha(char C)
{
    b32 Result = (((C >= 'a') && (C <= 'z')) ||
                  ((C >= 'A') && (C <= 'Z')));
    
    return Result;
}

inline u32
GetNullTerminiatedStringLength(char *Str)
{
    u32 Count = 0;
    
    if(Str)
    {
        while(*Str++)
        {
            ++Count;
        }
    }
    
    return Count;
}

inline char
ToLowercase(char Char)
{
    char Result = Char;
    
    if((Result >= 'A') && (Result <= 'Z'))
    {
        Result += 'a' - 'A';
    }
    
    return(Result);
}

inline char
ToUppercase(char Char)
{
    char Result = Char;
    
    if((Result >= 'a') && (Result <= 'z'))
    {
        Result += 'A' - 'a';
    }
    
    return(Result);
}

inline void
ToUpperCamelCase(string *Text)
{
    u32 ToSkip = 0;
    Text->Data[0] = ToUppercase(Text->Data[0]);
    for(u32 Index = 0;
        Index < Text->Size;
        ++Index)
    {
        if(Text->Data[Index + ToSkip] == '_')
        {
            Text->Data[Index + ToSkip + 1] = ToUppercase(Text->Data[Index + ToSkip + 1]);
            ++ToSkip;
        }
        
        if(ToSkip)
        {
            Text->Data[Index] = Text->Data[Index + ToSkip];
        }
    }
    Text->Size -= ToSkip;
}

inline b32
StringsAreEqual(string A, string B)
{
    b32 Result = (A.Size == B.Size);
    
    if(Result)
    {
        Result = true;
        for(u32 Index = 0;
            Index < A.Size;
            ++Index)
        {
            if(A.Data[Index] != B.Data[Index])
            {
                Result = false;
                break;
            }
        }
    }
    
    return Result;
}


#define String(Str) String_(sizeof(Str) - 1, (u8 *)Str)
inline string
String_(umm Length, u8 *Data)
{
    string Result;
    
    Result.Size = Length;
    Result.Data = Data;
    
    return Result;
}

#define PushString(Arena, Str) PushString_(Arena, sizeof(Str) - 1, (u8 *)Str)
inline string
PushString_(memory_arena *Arena, umm Length, u8 *Data)
{
    string Result;
    
    Result.Size = Length;
    Result.Data = PushCopy(Arena, Result.Size, Data);
    
    return Result;
}

typedef enum format_string_token_type
{
    FormatStringToken_Unknown,
    
    FormatStringToken_Specifier,
    FormatStringToken_SignedDecimalInteger,
    FormatStringToken_UnsignedDecimalInteger,
    FormatStringToken_DecimalFloatingPoint,
    FormatStringToken_StringOfCharacters,
    FormatStringToken_StringType,
    
    FormatStringToken_PrecisionSpecifier,
    FormatStringToken_PrecisionArgSpecifier,
    FormatStringToken_LongSpecifier,
    
    FormatStringToken_EndOfStream,
} format_string_token_type;

typedef struct format_string_token
{
    format_string_token_type Type;
    
    string Str;
} format_string_token;

typedef struct format_string_tokenizer
{
    char *At;
    string *Output;
    char *Tail;
} format_string_tokenizer;

inline format_string_token
GetNextFormatStringToken(format_string_tokenizer *Tokenizer)
{
    format_string_token Result;
    ZeroStruct(Result);
    
    Result.Str = String_(1, (u8 *)Tokenizer->At);
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    switch(C)
    {
        case '\0': { Result.Type = FormatStringToken_EndOfStream; } break;
        case '%': { Result.Type = FormatStringToken_Specifier; } break;
        case 'd':
        case 'i': { Result.Type = FormatStringToken_SignedDecimalInteger; } break;
        case 'u': { Result.Type = FormatStringToken_UnsignedDecimalInteger; } break;
        case 'f': { Result.Type = FormatStringToken_DecimalFloatingPoint; } break;
        case 's': { Result.Type = FormatStringToken_StringOfCharacters; } break;
        case 'S': { Result.Type = FormatStringToken_StringType; } break;
        
        case '.': { Result.Type = FormatStringToken_PrecisionSpecifier; } break;
        case '*': { Result.Type = FormatStringToken_PrecisionArgSpecifier; } break;
        case 'l': { Result.Type = FormatStringToken_LongSpecifier; } break;
        
        default:
        {
            while(Tokenizer->At[0] &&
                  Tokenizer->At[0] != '%' &&
                  !IsWhitespace(Tokenizer->At[0]))
            {
                ++Tokenizer->At;
            }
            
            Result.Str.Size = Tokenizer->At - (char *)Result.Str.Data;
            
            Result.Type = FormatStringToken_Unknown;
        } break;
    }
    
    return Result;
}

#define ReadVarArgUnsignedInteger(Length, ArgList) ((Length) == 8) ? va_arg(ArgList, u64) : (u64)va_arg(ArgList, u32)
#define ReadVarArgSignedInteger(Length, ArgList) ((Length) == 8) ? va_arg(ArgList, s64) : (s64)va_arg(ArgList, s32)
#define ReadVarArgFloat(Length, ArgList) va_arg(ArgList, f64)

global char Digits[] = "0123456789";
inline void
FormatStringParseU64(format_string_tokenizer *Tokenizer, u64 Value)
{
    u32 Base = 10;
    
    char *Start = Tokenizer->Tail;
    do
    {
        u64 DigitIndex = (Value % Base);
        char Digit = Digits[DigitIndex];
        *Tokenizer->Tail++ = Digit;
        
        Value /= Base;
    } while(Value != 0);
    char *End = Tokenizer->Tail;
    
    while(Start < End)
    {
        --End;
        char Temp = *End;
        *End = *Start;
        *Start = Temp;
        ++Start;
    }
}

inline void
FormatStringParseF64(format_string_tokenizer *Tokenizer, f64 Value, u32 Precision)
{
    if(Value < 0)
    {
        *Tokenizer->Tail++ = '-';
        Value = -Value;;
    }
    
    u64 IntegerPart = (u64)Value;
    Value -= (f64)IntegerPart;
    FormatStringParseU64(Tokenizer, IntegerPart);
    
    *Tokenizer->Tail++ = '.';
    
    // TODO(kstandbridge): Note that this is NOT an accurate way to do this!
    for(u32 PrecisionIndex = 0;
        PrecisionIndex < Precision;
        ++PrecisionIndex)
    {
        Value *= 10.0f;
        u32 Integer = (u32)Value;
        Value -= (f32)Integer;
        char Digit = Digits[Integer];
        *Tokenizer->Tail++ = Digit;
    }
}

inline string
FormatString_(memory_arena *Arena, char *Format, va_list ArgList)
{
    string Result;
    ZeroStruct(Result);
    Result.Data = BeginPushSize(Arena);
    
    format_string_tokenizer Tokenizer;
    ZeroStruct(Tokenizer);
    Tokenizer.At = Format;
    Tokenizer.Output = &Result;
    Tokenizer.Tail = (char *)Result.Data;
    
    b32 Parsing = true;
    while(Parsing)
    {
        format_string_token Token = GetNextFormatStringToken(&Tokenizer);
        switch(Token.Type)
        {
            case FormatStringToken_EndOfStream:
            {
                Parsing = false;
            }  break;
            
            case FormatStringToken_Specifier:
            {
                if(Tokenizer.At[0] == '%')
                {
                    *Tokenizer.Tail++ = '%';
                    ++Tokenizer.At;
                }
                else
                {                
                    b32 ParsingParam = true;
                    u32 IntegerLength = 4;
                    u32 FloatLength = 8;
                    s32 Precision = 6;
                    b32 PrecisionSpecified = false;
                    while(ParsingParam)
                    {
                        
                        format_string_token ParamToken = GetNextFormatStringToken(&Tokenizer);
                        switch(ParamToken.Type)
                        {
                            
                            case FormatStringToken_PrecisionArgSpecifier:
                            {
                                Precision = va_arg(ArgList, s32);
                            } break;
                            
                            case FormatStringToken_PrecisionSpecifier:
                            {
                                PrecisionSpecified = true;
                                format_string_token PrecisionToken = GetNextFormatStringToken(&Tokenizer);
                                
                                Precision = 0;
                                
                                while(IsNumber(*PrecisionToken.Str.Data))
                                {
                                    Precision *= 10;
                                    Precision += (*PrecisionToken.Str.Data - '0');
                                    ++PrecisionToken.Str.Data;
                                }
                                --Tokenizer.At;
                                
                                break;
                            }
                            
                            case FormatStringToken_LongSpecifier:
                            {
                                IntegerLength = 8;
                                break;
                            }
                            
                            case FormatStringToken_SignedDecimalInteger:
                            {
                                ParsingParam = false;
                                
                                s64 Value = ReadVarArgSignedInteger(IntegerLength, ArgList);
                                b32 IsNegative = (Value < 0);
                                if(IsNegative)
                                {
                                    Value = -Value;
                                    *Tokenizer.Tail++ = '-';
                                }
                                FormatStringParseU64(&Tokenizer, Value);
                            } break;
                            
                            case FormatStringToken_UnsignedDecimalInteger:
                            {
                                ParsingParam = false;
                                
                                u64 Value = ReadVarArgUnsignedInteger(IntegerLength, ArgList);
                                FormatStringParseU64(&Tokenizer, Value);
                            } break;
                            
                            case FormatStringToken_DecimalFloatingPoint:
                            {
                                ParsingParam = false;
                                FloatLength; // NOTE(kstandbridge): local variable is initialized but not referenced.
                                f64 Value = ReadVarArgFloat(FloatLength, ArgList);
                                FormatStringParseF64(&Tokenizer, Value, Precision);
                            } break;
                            
                            case FormatStringToken_StringOfCharacters:
                            {
                                ParsingParam = false;
                                
                                char *At = va_arg(ArgList, char *);
                                if(PrecisionSpecified)
                                {
                                    while(At[0] != '\0' && Precision--)
                                    {
                                        *Tokenizer.Tail++ = *At++;
                                    }
                                }
                                else
                                {
                                    while(At[0] != '\0')
                                    {
                                        *Tokenizer.Tail++ = *At++;
                                    }
                                }
                            } break;
                            
                            case FormatStringToken_StringType:
                            {
                                ParsingParam = false;
                                
                                string Str = va_arg(ArgList, string);
                                
                                for(u32 Index = 0;
                                    Index < Str.Size;
                                    ++Index)
                                {
                                    if(PrecisionSpecified)
                                    {
                                        if(Precision-- == 0)
                                        {
                                            break;
                                        }
                                    }
                                    *Tokenizer.Tail++ = Str.Data[Index];
                                }
                                
                            } break;
                            
                            case FormatStringToken_Unknown:
                            default:
                            {
                                __debugbreak();
                            } break;
                            
                        }
                    }
                    
                    
                }
                
            } break;
            
            case FormatStringToken_Unknown:
            default:
            {
                while(Token.Str.Size)
                {
                    *Tokenizer.Tail++ = *Token.Str.Data++;
                    --Token.Str.Size;
                }
            } break;
        }
    }
    va_end(ArgList);
    
    Result.Size = Tokenizer.Tail - (char *)Result.Data;
    EndPushSize(Arena, Result.Size);
    return Result;
}

internal string
FormatString(memory_arena *Arena, char *Format, ...)
{
    va_list ArgList;
    
    va_start(ArgList, Format);
    string Result = FormatString_(Arena, Format, ArgList);
    va_end(ArgList);
    
    return Result;
}

#define KENGINE_STRING_H
#endif //KENGINE_STRING_H
