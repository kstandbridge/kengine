
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

u32
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

u64
U64FromString(string *Text)
{
    string Ignored;
    Ignored.Size = Text->Size;
    Ignored.Data = Text->Data;
    
    u64 Result = U64FromString_(&Ignored);
    
    return Result;
}

b32
IsNumber(char C)
{
    b32 Result = ((C >= '0') && (C <= '9'));
    
    return Result;
}

b32
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

b32
StringBeginsWith(string Needle, string HayStack)
{
    string Prefix = String_(Needle.Size, HayStack.Data);
    b32 Result = StringsAreEqual(Prefix, Needle);
    
    return Result;
}


b32
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
b32
IsSpacing(char C)
{
    b32 Result = ((C == ' ') ||
                  (C == '\t') ||
                  (C == '\v') ||
                  (C == '\f'));
    
    return Result;
}

b32
IsWhitespace(char C)
{
    b32 Result = (IsSpacing(C) || 
                  IsEndOfLine(C));
    
    return Result;
}

b32
IsEndOfLine(char C)
{
    b32 Result = ((C == '\n') ||
                  (C == '\r'));
    
    return Result;
}

string
PushString_(memory_arena *Arena, umm Length, u8 *Data)
{
    string Result;
    
    Result.Size = Length;
    Result.Data = (u8 *)PushCopy(Arena, Result.Size, Data);
    
    return Result;
}

typedef enum format_string_token_type
{
    FormatStringToken_Unknown,
    
    FormatStringToken_Specifier,
    FormatStringToken_SignedDecimalInteger,
    FormatStringToken_UnsignedDecimalInteger,
    FormatStringToken_DecimalFloatingPoint,
    FormatStringToken_LongStringOfCharacters,
    FormatStringToken_StringOfCharacters,
    FormatStringToken_StringType,
    FormatStringToken_Binary,
    FormatStringToken_HexUppercase,
    FormatStringToken_HexLowercase,
    
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

internal format_string_token
GetNextFormatStringToken(format_string_state *State)
{
    format_string_token Result = {0};
    
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
        case 'b':  { Result.Type = FormatStringToken_Binary; } break; 
        case 'X':  { Result.Type = FormatStringToken_HexUppercase; } break; 
        case 'x':  { Result.Type = FormatStringToken_HexLowercase; } break; 
        
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
#define ReadVarArgByte(ArgList) va_arg(ArgList, u8)

internal void
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
    
    char Digits[] = "0123456789";
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

internal void
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
    
    char Digits[] = "0123456789";
    
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
FormatStringParseHex(format_string_state *State, u64 Value, u32 Width, b32 PadWithZeros, char *Digits, u32 DigitsLength)
{
    if(PadWithZeros)
    {
        *State->Tail++ = '0';
        *State->Tail++ = 'x';
    }
    
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
    
    if(!PadWithZeros)
    {
        *State->Tail++ = '0';
        *State->Tail++ = 'x';
    }
    
    b32 FirstDigit = true;
    for(s32 Index = 63;
        Index > 0;
        Index -= 4)
    {
        u8 Part = (Value >> (Index + 1 - 4) & ~(~0 << 4));
        if((!FirstDigit) ||
           (Part > 0))
        {
            if(FirstDigit)
            {
                FirstDigit = false;
            }
            if(Part < DigitsLength)
            {
                *State->Tail++ = Digits[Part];
            }
        }
    }
}

void
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
                                
                                // TODO(kstandbridge): We should probably convert the wchar_t to char for this copy
#if 0                                
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
                                        *State->Tail++ = *At++;
                                    }
                                }
#endif
                                
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
                            
                            case FormatStringToken_Binary:
                            {
                                ParsingParam = false;
                                
                                if(Width == 0)
                                {
                                    Width = 8;
                                }
                                u32 Value = (u32)(ReadVarArgUnsignedInteger(IntegerLength, ArgList));
                                for(s32 Index = Width - 1;
                                    Index >= 0;
                                    --Index)
                                {
                                    if((Index + 1) % 8 == 0)
                                    {
                                        *State->Tail++ = ' ';
                                        *State->Tail++ = '0';
                                        *State->Tail++ = 'b';
                                    }
                                    char Digit = (Value & (1 << Index)) ? '1' : '0';
                                    *State->Tail++ = Digit;
                                }
                                
                            } break;
                            
                            case FormatStringToken_HexUppercase:
                            case FormatStringToken_HexLowercase:
                            {
                                ParsingParam = false;
                                
                                char UpperDigits[] = "0123456789ABCDEF";
                                char LowerDigits[] = "0123456789abcdef";
                                Assert(ArrayCount(UpperDigits) == ArrayCount(LowerDigits));
                                char *Digits = (ParamToken.Type == FormatStringToken_HexUppercase) ? UpperDigits : LowerDigits;
                                
                                u64 Value = ReadVarArgUnsignedInteger(IntegerLength, ArgList);
                                FormatStringParseHex(State, Value, Width, PadWithZeros, Digits, ArrayCount(UpperDigits));
                            } break;
                            
                            case FormatStringToken_Unknown:
                            default:
                            {
#if KENGINE_INTERNAL
                                __debugbreak();
#endif
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

string
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

string
EndFormatStringToBuffer(format_string_state *State, u8 *Buffer, umm BufferSize)
{
    string Result;
    Result.Size = State->BufferSize;
    Result.Data = Buffer;
    if(State->BufferSize < BufferSize)
    {
        Copy(Result.Size, State->Buffer, Result.Data);
    }
    else
    {
        InvalidCodePath;
    }
    
    return Result;
}

string
EndFormatString(format_string_state *State, memory_arena *Arena)
{
    Assert(State->BufferSize > 0);
    
    string Result = PushString_(Arena, State->BufferSize, State->Buffer);
    
    return Result;
}

u32
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

s32
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

void
ParseFromString(string Text, char *Format, ...)
{
    va_list ArgList;
    
    va_start(ArgList, Format);
    ParseFromString_(Text, Format, ArgList);
    va_end(ArgList);
}
