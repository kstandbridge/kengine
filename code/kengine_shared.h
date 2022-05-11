#ifndef KENGINE_SHARED_H

#include <stdarg.h>

// NOTE(kstandbridge): Memory

inline void
InitializeArena(memory_arena *Arena, memory_index Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (u8 *)Base;
    Arena->Used = 0;
    Arena->TempCount = 0;
}


inline memory_index
GetAlignmentOffset(memory_arena *Arena, memory_index Alignment)
{
    memory_index AlignmentOffset = 0;
    
    memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
    memory_index AlignmentMask = Alignment - 1;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
    }
    
    return(AlignmentOffset);
}


inline void *
Copy(memory_index Size, void *SourceInit, void *DestInit)
{
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    while(Size--) {*Dest++ = *Source++;}
    
    return(DestInit);
}


#define PushStruct(Arena, type) (type *)PushSizeInternal(Arena, sizeof(type), 4)
#define PushArray(Arena, Count, type) (type *)PushSizeInternal(Arena, (Count)*sizeof(type), 4)
#define PushSize(Arena, Size) PushSizeInternal(Arena, Size, 4)
#define PushCopy(Arena, Size, Source) Copy(Size, Source, PushSizeInternal(Arena, Size, 4))
inline void *
PushSizeInternal(memory_arena *Arena, memory_index SizeInit, memory_index Alignment)
{
    memory_index Size = SizeInit;
    
    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Size += AlignmentOffset;
    
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;
    
    Assert(Size >= SizeInit);
    
    return(Result);
}


inline u8 *
BeginPushSize(memory_arena *Arena)
{
    u8 *Result = Arena->Base;
    ++Arena->TempCount;
    
    return Result;
}

inline void
EndPushSize(memory_arena *Arena, memory_index Size)
{
    --Arena->TempCount;
    PushSizeInternal(Arena, Size, 4);
}

// NOTE(kstandbridge): String

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
IsNumber(char C)
{
    b32 Result = ((C >= '0') && (C <= '9'));
    
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
    
    FormatStringToken_PrecisionSpecifier,
    FormatStringToken_PrecisionArgSpecifier,
    FormatStringToken_LongSpecifier,
    
    FormatStringToken_EndOfStream,
} format_string_token_type;

typedef struct format_string_token
{
    format_string_token_type Type;
    
    size_t TextLength;
    char *Text;
} format_string_token;

typedef struct format_string_tokenizer
{
    char *At;
    string *Output;
    char *Tail;
} format_string_tokenizer;

inline format_string_token
GetNextFormatStringTokenInternal(format_string_tokenizer *Tokenizer)
{
    format_string_token Result;
    ZeroStruct(Result);
    
    Result.TextLength = 1;
    Result.Text = Tokenizer->At;
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
            
            Result.TextLength = Tokenizer->At - Result.Text;
            
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
FormatStringParseU64Internal(format_string_tokenizer *Tokenizer, u64 Value)
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
FormatStringParseF64Interal(format_string_tokenizer *Tokenizer, f64 Value, u32 Precision)
{
    if(Value < 0)
    {
        *Tokenizer->Tail++ = '-';
        Value = -Value;;
    }
    
    u64 IntegerPart = (u64)Value;
    Value -= (f64)IntegerPart;
    FormatStringParseU64Internal(Tokenizer, IntegerPart);
    
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
FormatStringInternal(memory_arena *Arena, char *Format, va_list ArgList)
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
    u32 IntegerLength = 4;
    u32 FloatLength = 8;
    s32 Precision = 6;
    b32 PrecisionSpecified = false;
    while(Parsing)
    {
        format_string_token Token = GetNextFormatStringTokenInternal(&Tokenizer);
        switch(Token.Type)
        {
            case FormatStringToken_EndOfStream:
            {
                Parsing = false;
            }  break;
            
            case FormatStringToken_Specifier:
            {
                IntegerLength = 4;
                Precision = 6;
                PrecisionSpecified = false;
                if(Tokenizer.At[0] == '%')
                {
                    *Tokenizer.Tail++ = '%';
                    ++Tokenizer.At;
                }
            } break;
            
            case FormatStringToken_SignedDecimalInteger:
            {
                s64 Value = ReadVarArgSignedInteger(IntegerLength, ArgList);
                b32 IsNegative = (Value < 0);
                if(IsNegative)
                {
                    Value = -Value;
                    *Tokenizer.Tail++ = '-';
                }
                FormatStringParseU64Internal(&Tokenizer, Value);
            } break;
            
            case FormatStringToken_UnsignedDecimalInteger:
            {
                u64 Value = ReadVarArgUnsignedInteger(IntegerLength, ArgList);
                FormatStringParseU64Internal(&Tokenizer, Value);
            } break;
            
            case FormatStringToken_DecimalFloatingPoint:
            {
                f64 Value = ReadVarArgFloat(FloatLength, ArgList);
                FormatStringParseF64Interal(&Tokenizer, Value, Precision);
            } break;
            
            case FormatStringToken_StringOfCharacters:
            {
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
            
            case FormatStringToken_PrecisionArgSpecifier:
            {
                Precision = va_arg(ArgList, s32);
            } break;
            
            case FormatStringToken_PrecisionSpecifier:
            {
                // TODO(kstandbridge): * means read from the argument
                // TODO(kstandbridge): a number is the specified
                PrecisionSpecified = true;
                format_string_token PrecisionToken = GetNextFormatStringTokenInternal(&Tokenizer);
                
                Precision = 0;
                
                while(IsNumber(*PrecisionToken.Text))
                {
                    Precision *= 10;
                    Precision += (*PrecisionToken.Text - '0');
                    ++PrecisionToken.Text;
                }
                --Tokenizer.At;
                
                break;
            }
            
            case FormatStringToken_LongSpecifier:
            {
                IntegerLength = 8;
                break;
            }
            
            case FormatStringToken_Unknown:
            default:
            {
                while(Token.TextLength)
                {
                    *Tokenizer.Tail++ = *Token.Text++;
                    --Token.TextLength;
                }
            } break;
        }
    }
    va_end(ArgList);
    
    Result.Length = Tokenizer.Tail - (char *)Result.Data;
    EndPushSize(Arena, Result.Length);
    return Result;
}

internal string
FormatString(memory_arena *Arena, char *Format, ...)
{
    va_list ArgList;
    
    va_start(ArgList, Format);
    string Result = FormatStringInternal(Arena, Format, ArgList);
    va_end(ArgList);
    
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

#define String(Str) StringInternal(GetNullTerminiatedStringLength(Str), (u8 *)Str)
inline string
StringInternal(umm Length, u8 *Data)
{
    string Result;
    
    Result.Length = Length;
    Result.Data = Data;
    
    return Result;
}

inline b32
StringsAreEqual(string A, string B)
{
    b32 Result = (A.Length == B.Length);
    
    if(Result)
    {
        Result = true;
        for(u32 Index = 0;
            Index < A.Length;
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

#define KENGINE_SHARED_H
#endif //KENGINE_SHARED_H
