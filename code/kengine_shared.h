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

typedef enum format_string_token_type
{
    FormatStringToken_Unknown,
    
    FormatStringToken_Specifier,
    FormatStringToken_SignedDecimalInteger,
    FormatStringToken_StringOfCharacters,
    
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

internal format_string_token
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
        case 's': { Result.Type = FormatStringToken_StringOfCharacters; } break;
        
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

inline void
FormatStringParseU64Interal(format_string_tokenizer *Tokenizer, u64 Value)
{
    local_persist char Digits[] = "0123456789";
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

internal string
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
                if(Tokenizer.At[0] == '%')
                {
                    *Tokenizer.Tail++ = '%';
                    ++Tokenizer.At;
                }
            } break;
            
            case FormatStringToken_SignedDecimalInteger:
            {
                // NOTE(kstandbridge): Signed decimal integer
                
                u32 IntegerLength = 4;
                s64 Value = ReadVarArgSignedInteger(IntegerLength, ArgList);
                b32 IsNegative = (Value < 0);
                if(IsNegative)
                {
                    Value = -Value;
                    *Tokenizer.Tail++ = '-';
                }
                FormatStringParseU64Interal(&Tokenizer, Value);
            } break;
            
            case FormatStringToken_StringOfCharacters:
            {
                char *At = va_arg(ArgList, char *);
                while(At[0] != '\0')
                {
                    *Tokenizer.Tail++ = *At++;
                }
                
            } break;
            
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

#define KENGINE_SHARED_H
#endif //KENGINE_SHARED_H
