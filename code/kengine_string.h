#ifndef KENGINE_STRING_H

inline void
AppendCString(char *StartAt, char *Text)
{
    while(*Text)
    {
        *StartAt++ = *Text++;
    }
}

inline u32
StringToHashValue(string Text)
{
    u32 Result = 0;
    
    u8 *At = Text.Data;
    u32 Index = 0;
    for(;
        Index < Text.Size;
        ++At, ++Index)
    {
        Result = 65599*Result + *At;
    }
    
    return Result;
}

internal u32
U32FromCString_(char **Text)
{
    u32 Result = 0;
    
    char *At = *Text;
    while((*At >= '0') &&
          (*At <= '9'))
    {
        Result *= 10;
        Result += (*At - '0');
        ++At;
    }
    
    *Text = At;
    
    return Result;
}

inline u32
U32FromCString(char *Text)
{
    char *Ignored = Text;
    u32 Result = U32FromCString_(&Ignored);
    return Result;
}

internal u32
U32FromString_(string *Text)
{
    u32 Result = 0;
    
    while((Text->Size > 0) &&
          (Text->Data[0] >= '0') &&
          (Text->Data[0] <= '9'))
    {
        Result *= 10;
        Result += (Text->Data[0] - '0');
        ++Text->Data;
        --Text->Size;
    }
    
    return Result;
}

inline u32
U32FromString(string *Text)
{
    string Ignored;
    Ignored.Size = Text->Size;
    Ignored.Data = Text->Data;
    
    u32 Result = U32FromString_(&Ignored);
    
    return Result;
}

internal u64
U64FromString_(string *Text)
{
    u64 Result = 0;
    
    while((Text->Size > 0) &&
          (Text->Data[0] >= '0') &&
          (Text->Data[0] <= '9'))
    {
        Result *= 10;
        Result += (Text->Data[0] - '0');
        ++Text->Data;
        --Text->Size;
    }
    
    return Result;
}

inline u64
U64FromString(string *Text)
{
    string Ignored;
    Ignored.Size = Text->Size;
    Ignored.Data = Text->Data;
    
    u64 Result = U64FromString_(&Ignored);
    
    return Result;
}

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
    
    return Result;
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
    else if((Char >= 'a') && (Char <= 'f'))
    {
        Result = 0xA + (Char - 'a');
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
CToLowercase(char Char)
{
    char Result = Char;
    
    if((Result >= 'A') && (Result <= 'Z'))
    {
        Result += 'a' - 'A';
    }
    
    return(Result);
}

inline char
CToUppercase(char Char)
{
    char Result = Char;
    
    if((Result >= 'a') && (Result <= 'z'))
    {
        Result += 'A' - 'a';
    }
    
    return(Result);
}

inline void
StringDeleteCharactersAt(string *Text, umm At, umm Count)
{
    for(u32 Current = 0;
        Current < Count;
        ++Current)
    {
        for(umm Index = At;
            Index < Text->Size;
            ++Index)
        {
            Text->Data[Index] = Text->Data[Index + 1];
        }
        --Text->Size;
    }
}

inline void
SnakeToLowerCase(string *Text)
{
    u32 ToSkip = 0;
    Text->Data[0] = CToLowercase(Text->Data[0]);
    for(u32 Index = 0;
        Index < Text->Size;
        ++Index)
    {
        if(Text->Data[Index + ToSkip] == '_')
        {
            Text->Data[Index + ToSkip + 1] = CToLowercase(Text->Data[Index + ToSkip + 1]);
            ++ToSkip;
        }
        
        if(ToSkip)
        {
            Text->Data[Index] = Text->Data[Index + ToSkip];
        }
    }
    Text->Size -= ToSkip;
}

inline void
ToUpperCamelCase(string *Text)
{
    u32 ToSkip = 0;
    Text->Data[0] = CToUppercase(Text->Data[0]);
    for(u32 Index = 0;
        Index < Text->Size;
        ++Index)
    {
        if(Text->Data[Index + ToSkip] == '_')
        {
            Text->Data[Index + ToSkip + 1] = CToUppercase(Text->Data[Index + ToSkip + 1]);
            ++ToSkip;
        }
        
        if(ToSkip)
        {
            Text->Data[Index] = Text->Data[Index + ToSkip];
        }
    }
    Text->Size -= ToSkip;
}

inline void
StringToUppercase(string Text)
{
    for(u32 Index = 0;
        Index < Text.Size;
        ++Index)
    {
        Text.Data[Index] = CToUppercase(Text.Data[Index]);
    }
}

inline void
StringToLowercase(string Text)
{
    for(u32 Index = 0;
        Index < Text.Size;
        ++Index)
    {
        Text.Data[Index] = CToLowercase(Text.Data[Index]);
    }
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

inline string
CStringToString(char *NullTerminatedString)
{
    string Result = String_(GetNullTerminiatedStringLength(NullTerminatedString),
                            (u8 *)NullTerminatedString);
    return Result;
}

inline b32
StringBeginsWith(string Needle, string HayStack)
{
    b32 Result = StringsAreEqual(String_(Needle.Size, HayStack.Data), Needle);
    
    return Result;
}

inline b32
StringContains(string Needle, string HayStack)
{
    b32 Result = false;
    
    while(HayStack.Size >= Needle.Size)
    {
        if(StringBeginsWith(Needle, HayStack))
        {
            Result = true;
            break;
        }
        --HayStack.Size;
        ++HayStack.Data;
    }
    
    return Result;
}

inline s32
StringComparison(string A, string B)
{
    u32 IndexA = 0;
    u32 IndexB = 0;
    while(A.Data[IndexA] == B.Data[IndexB])
    {
        if((IndexA == A.Size - 1) ||
           (IndexB == B.Size - 1))
        {
            break;
        }
        ++IndexA;
        ++IndexB;
    }
    
    s32 Result = A.Data[IndexA] - B.Data[IndexB];
    
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

inline editable_string
PushEditableString(memory_arena *Arena, umm Size, string Text)
{
    editable_string Result;
    
    Result.Size = Size;
    Result.Length = Text.Size;
    Result.Data = PushSize(Arena, Size);
    Copy(Text.Size, Text.Data, Result.Data);
    Result.SelectionStart = 3;
    Result.SelectionEnd = 70;
    
    return Result;
}

inline editable_string
ToEditableString(string Text)
{
    editable_string Result;
    
    Result.Size = Text.Size;
    Result.Length = Text.Size;
    Result.Data = Text.Data;
    Result.SelectionStart = U32Max;
    Result.SelectionEnd = U32Max;
    
    return Result;
}

typedef enum
{
    FormatStringToken_Unknown,
    
    FormatStringToken_Specifier,
    FormatStringToken_SignedDecimalInteger,
    FormatStringToken_UnsignedDecimalInteger,
    FormatStringToken_DecimalFloatingPoint,
    FormatStringToken_LongStringOfCharacters,
    FormatStringToken_StringOfCharacters,
    FormatStringToken_StringType,
    
    FormatStringToken_PrecisionSpecifier,
    FormatStringToken_PrecisionArgSpecifier,
    FormatStringToken_LongSpecifier,
    FormatStringToken_WidthSpecifier,
    FormatStringToken_PadWithZeros,
    
    FormatStringToken_EndOfStream,
} format_string_token_type;

typedef struct
{
    format_string_token_type Type;
    
    string Str;
} format_string_token;


typedef struct
{
    u8 Buffer[65536];
    umm BufferSize;
    char *Tail;
    char *At;
} format_string_state;

inline format_string_token
GetNextFormatStringToken(format_string_state *State)
{
    format_string_token Result;
    ZeroStruct(Result);
    
    Result.Str = String_(1, (u8 *)State->At);
    char C = State->At[0];
    ++State->At;
    switch(C)
    {
        case '\0': { Result.Type = FormatStringToken_EndOfStream; } break;
        case '%':  { Result.Type = FormatStringToken_Specifier; } break;
        case 'd':
        case 'i':  { Result.Type = FormatStringToken_SignedDecimalInteger; } break;
        case 'u':  { Result.Type = FormatStringToken_UnsignedDecimalInteger; } break;
        case 'f':  { Result.Type = FormatStringToken_DecimalFloatingPoint; } break;
        case 's':  { Result.Type = FormatStringToken_StringOfCharacters; } break;
        case 'S':  { Result.Type = FormatStringToken_StringType; } break; 
        
        case '.':  { Result.Type = FormatStringToken_PrecisionSpecifier; } break;
        case '*':  { Result.Type = FormatStringToken_PrecisionArgSpecifier; } break;
        case 'l':  
        { 
            if(State->At[0] == 's')
            {
                Result.Type = FormatStringToken_LongStringOfCharacters; 
                ++State->At;
            }
            else
            {
                Result.Type = FormatStringToken_LongSpecifier; 
            }
        } break;
        
        
        default:
        {
            if(IsNumber(C))
            {
                if(C == '0')
                {
                    Result.Type = FormatStringToken_PadWithZeros;
                }
                else
                {
                    Result.Type = FormatStringToken_WidthSpecifier;
                }
                
                
                while(State->At[0] &&
                      IsNumber(State->At[0]))
                {
                    ++State->At;
                }
                
                Result.Str.Size = State->At - (char *)Result.Str.Data;
                
            }
            else
            {
                while(State->At[0] &&
                      State->At[0] != '%' &&
                      !IsWhitespace(State->At[0]))
                {
                    ++State->At;
                }
                
                Result.Str.Size = State->At - (char *)Result.Str.Data;
                
                Result.Type = FormatStringToken_Unknown;
            }
        } break;
    }
    
    return Result;
}

#define ReadVarArgUnsignedInteger(Length, ArgList) ((Length) == 8) ? va_arg(ArgList, u64) : (u64)va_arg(ArgList, u32)
#define ReadVarArgSignedInteger(Length, ArgList) ((Length) == 8) ? va_arg(ArgList, s64) : (s64)va_arg(ArgList, s32)
#define ReadVarArgFloat(Length, ArgList) va_arg(ArgList, f64)

global char Digits[] = "0123456789";
inline void
FormatStringParseU64(format_string_state *State, u64 Value, u32 Width, b32 PadWithZeros)
{
    b32 PrintValue = (Width == 0 || Value > 0);
    
    u32 Base = 10;
    
    u64 ValueLeft = Value;
    while((ValueLeft > 0) && 
          (Width > 0))
    {
        ValueLeft /= Base;
        --Width;
    }
    while(Width > 0)
    {
        *State->Tail++ = PadWithZeros ? '0' : ' ';
        --Width;
    }
    
    if(PrintValue)
    {
        char *Start = State->Tail;
        do
        {
            u64 DigitIndex = (Value % Base);
            char Digit = Digits[DigitIndex];
            *State->Tail++ = Digit;
            
            Value /= Base;
        } while(Value != 0);
        char *End = State->Tail;
        
        while(Start < End)
        {
            --End;
            char Temp = *End;
            *End = *Start;
            *Start = Temp;
            ++Start;
        }
    }
}

inline void
FormatStringParseF64(format_string_state *State, f64 Value, u32 Width, u32 Precision, b32 PadWithZeros)
{
    f64 ValueLeft = Value;
    while((ValueLeft > 1) && 
          (Width > 0))
    {
        ValueLeft /= 10;
        --Width;
    }
    while(Width > 0)
    {
        *State->Tail++ = ' ';
        --Width;
    }
    
    if(Value < 0)
    {
        *State->Tail++ = '-';
        Value = -Value;;
    }
    
    u64 IntegerPart = (u64)Value;
    Value -= (f64)IntegerPart;
    FormatStringParseU64(State, IntegerPart, 0, PadWithZeros);
    
    *State->Tail++ = '.';
    
    // TODO(kstandbridge): Note that this is NOT an accurate way to do this!
    for(u32 PrecisionIndex = 0;
        PrecisionIndex < Precision;
        ++PrecisionIndex)
    {
        Value *= 10.0f;
        u32 Integer = (u32)Value;
        Value -= (f32)Integer;
        char Digit = Digits[Integer];
        *State->Tail++ = Digit;
    }
}

internal void
AppendFormatString_(format_string_state *State, char *Format, va_list ArgList)
{
    if(State->Tail == 0)
    {
        State->Tail = (char *)State->Buffer;
    }
    
    State->At = Format;
    b32 Parsing = true;
    while(Parsing)
    {
        format_string_token Token = GetNextFormatStringToken(State);
        switch(Token.Type)
        {
            case FormatStringToken_EndOfStream:
            {
                Parsing = false;
            }  break;
            
            case FormatStringToken_Specifier:
            {
                if(State->At[0] == '%')
                {
                    *State->Tail++ = '%';
                    ++State->At;
                }
                else
                {                
                    b32 ParsingParam = true;
                    u32 IntegerLength = 4;
                    u32 FloatLength = 8;
                    s32 Precision = 6;
                    b32 PadWithZeros = false;
                    s32 Width = 0;
                    b32 PrecisionSpecified = false;
                    while(ParsingParam)
                    {
                        format_string_token ParamToken = GetNextFormatStringToken(State);
                        switch(ParamToken.Type)
                        {
                            case FormatStringToken_PadWithZeros:
                            {
                                PadWithZeros = true;
                                Width = U32FromString(&ParamToken.Str);
                            } break;
                            case FormatStringToken_WidthSpecifier:
                            {
                                Width = U32FromString(&ParamToken.Str);
                            } break;
                            
                            case FormatStringToken_PrecisionArgSpecifier:
                            {
                                Precision = va_arg(ArgList, s32);
                            } break;
                            
                            case FormatStringToken_PrecisionSpecifier:
                            {
                                PrecisionSpecified = true;
                                format_string_token PrecisionToken = GetNextFormatStringToken(State);
                                
                                Precision = 0;
                                
                                while(IsNumber(*PrecisionToken.Str.Data))
                                {
                                    Precision *= 10;
                                    Precision += (*PrecisionToken.Str.Data - '0');
                                    ++PrecisionToken.Str.Data;
                                }
                                if(PrecisionToken.Str.Data[0] == '*')
                                {
                                    --State->At;
                                }
                                
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
                                    *State->Tail++ = '-';
                                }
                                FormatStringParseU64(State, Value, Width, PadWithZeros);
                            } break;
                            
                            case FormatStringToken_UnsignedDecimalInteger:
                            {
                                ParsingParam = false;
                                
                                u64 Value = ReadVarArgUnsignedInteger(IntegerLength, ArgList);
                                FormatStringParseU64(State, Value, Width, PadWithZeros);
                            } break;
                            
                            case FormatStringToken_DecimalFloatingPoint:
                            {
                                ParsingParam = false;
                                FloatLength; // NOTE(kstandbridge): local variable is initialized but not referenced.
                                f64 Value = ReadVarArgFloat(FloatLength, ArgList);
                                FormatStringParseF64(State, Value, Width, Precision, PadWithZeros);
                            } break;
                            
                            case FormatStringToken_StringOfCharacters:
                            {
                                ParsingParam = false;
                                
                                char *At = va_arg(ArgList, char *);
                                if(PrecisionSpecified)
                                {
                                    while(At[0] != '\0' && Precision--)
                                    {
                                        *State->Tail++ = *At++;
                                    }
                                }
                                else
                                {
                                    while(At[0] != '\0')
                                    {
                                        *State->Tail++ = *At++;
                                    }
                                }
                            } break;
                            
                            case FormatStringToken_LongStringOfCharacters:
                            {
                                ParsingParam = false;
                                
                                wchar_t *At = va_arg(ArgList, wchar_t *);
                                if(PrecisionSpecified)
                                {
                                    while(At[0] != '\0' && Precision--)
                                    {
                                        *State->Tail++ = *At++;
                                    }
                                }
                                else
                                {
                                    while(At[0] != '\0')
                                    {
                                        // TODO(kstandbridge): We should probably convert the wchar_t to char for this copy
                                        *State->Tail++ = *At++;
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
                                    *State->Tail++ = Str.Data[Index];
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
                    *State->Tail++ = *Token.Str.Data++;
                    --Token.Str.Size;
                }
            } break;
        }
    }
    va_end(ArgList);
    
    State->BufferSize = State->Tail - (char *)State->Buffer;
}

internal void
AppendFormatString(format_string_state *State, char *Format, ...)
{
    va_list ArgList;
    
    va_start(ArgList, Format);
    AppendFormatString_(State, Format, ArgList);
    va_end(ArgList);
    
}

internal format_string_state
BeginFormatString()
{
    format_string_state Result;
    ZeroStruct(Result);
    
    return Result;
}

internal string
EndFormatString(format_string_state *State, memory_arena *Arena)
{
    Assert(State->BufferSize > 0);
    
    string Result = PushString_(Arena, State->BufferSize, State->Buffer);
    
    return Result;
}

internal string
EndFormatStringToBuffer(format_string_state *State, u8 *Buffer, umm BufferSize)
{
    Assert(State->BufferSize <= BufferSize);
    
    string Result;
    Result.Size = State->BufferSize;
    Result.Data = Buffer;
    Copy(Result.Size, State->Buffer, Result.Data);
    
    return Result;
}

internal string
FormatString(memory_arena *Arena, char *Format, ...)
{
    format_string_state StringState = BeginFormatString();
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Result = EndFormatString(&StringState, Arena);
    
    return Result;
}


internal string
FormatStringToBuffer(u8 *Buffer, umm BufferSize, char *Format, ...)
{
    format_string_state StringState = BeginFormatString();
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Result = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    return Result;
}


inline void
StringToCString(string Text, u32 BufferSize, char *Buffer)
{
    Assert(BufferSize >= Text.Size + 1);
    Copy(Text.Size, Text.Data, Buffer);
    Buffer[Text.Size] = '\0';
}

internal void
ParseFromString_(string Text, char *Format, va_list ArgList)
{
    string Source;
    Source.Size = Text.Size;
    Source.Data = Text.Data;
    
    char *At = Format;
    umm Index = 0;
    b32 LongSpecified = false;
    while(At[0] && Source.Size)
    {
        if(*At == '%')
        {
            ++At;
            switch(*At)
            {
                case 'l':
                {
                    LongSpecified = true;
                    ++At;
                } // NOTE(kstandbridge): Intentionall fall through
                case 'u':
                case 'd':
                {
                    if(LongSpecified)
                    {
                        u64 *Value = va_arg(ArgList, u64 *);
                        u8 *PosBefore = Source.Data;
                        *Value = U64FromString_(&Source);
                        u8 *PosAfter = Source.Data;
                        Index += PosAfter - PosBefore;
                    }
                    else
                    {
                        u32 *Value = va_arg(ArgList, u32 *);
                        u8 *PosBefore = Source.Data;
                        *Value = U32FromString_(&Source);
                        u8 *PosAfter = Source.Data;
                        Index += PosAfter - PosBefore;
                    }
                    
                    
                    LongSpecified = false;
                } break;
                
                case 'S':
                {
                    string *SubStr = va_arg(ArgList, string *);
                    SubStr->Data = (u8 *)Source.Data;
                    ++At;
                    char End = *At;
                    // TODO(kstandbridge): This check only works when the str is at end, make a more robust solution
                    while((End != Source.Data[0]) &&
                          (Index < Text.Size))
                    {
                        ++Source.Data;
                        if(Source.Size == 0)
                        {
                            break;
                        }
                        else
                        {
                            --Source.Size;
                        }
                        ++Index;
                    }
                    SubStr->Size = Source.Data - SubStr->Data;
                    --At;
                } break;
                
                // TODO(kstandbridge): Support types?
                InvalidDefaultCase;
            }
        }
        else
        {
            ++Source.Data;
            if(Source.Size == 0)
            {
                break;
            }
            else
            {
                --Source.Size;
            }
            ++Index;
        }
        ++At;
    }
}

internal void
ParseFromString(string Text, char *Format, ...)
{
    va_list ArgList;
    
    va_start(ArgList, Format);
    ParseFromString_(Text, Format, ArgList);
    va_end(ArgList);
}

inline s16
ParseMonth(string Text)
{
    s16 Result = 0;
    
    if(StringsAreEqual(String("Jan"), Text))
    {
        Result = 1;
    }
    else if(StringsAreEqual(String("Feb"), Text))
    {
        Result = 2;
    }
    else if(StringsAreEqual(String("Mar"), Text))
    {
        Result = 3;
    }
    else if(StringsAreEqual(String("Apr"), Text))
    {
        Result = 4;
    }
    else if(StringsAreEqual(String("May"), Text))
    {
        Result = 5;
    }
    else if(StringsAreEqual(String("Jun"), Text))
    {
        Result = 6;
    }
    else if(StringsAreEqual(String("Jul"), Text))
    {
        Result = 7;
    }
    else if(StringsAreEqual(String("Aug"), Text))
    {
        Result = 8;
    }
    else if(StringsAreEqual(String("Sep"), Text))
    {
        Result = 9;
    }
    else if(StringsAreEqual(String("Oct"), Text))
    {
        Result = 10;
    }
    else if(StringsAreEqual(String("Nov"), Text))
    {
        Result = 11;
    }
    else if(StringsAreEqual(String("Dec"), Text))
    {
        Result = 12;
    }
    else
    {
        InvalidCodePath;
    }
    
    return Result;
}

#define KENGINE_STRING_H
#endif //KENGINE_STRING_H
