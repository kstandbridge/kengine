#ifndef KENGINE_STRING_H

b32 IsNumber(char C);
b32 IsSpacing(char C);
b32 IsWhitespace(char C);
b32 IsEndOfLine(char C);
b32 StringsAreEqual(string A, string B);

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
string
PushString_(memory_arena *Arena, umm Length, u8 *Data);

s32
StringComparison(string A, string B);

void
ParseFromString(string Text, char *Format, ...);

b32
StringContains(string Needle, string HayStack);

u32
U32FromString(string *Text);

u64
U64FromString(string *Text);

b32
StringBeginsWith(string Needle, string HayStack);

typedef struct format_string_state
{
    u8 Buffer[65536];
    umm BufferSize;
    char *Tail;
    char *At;
} format_string_state;

inline format_string_state
BeginFormatString()
{
    format_string_state Result = {0};
    
    return Result;
}

string
EndFormatString(format_string_state *State, memory_arena *Arena);

void
AppendFormatString_(format_string_state *State, char *Format, va_list ArgList);

inline void
AppendFormatString(format_string_state *State, char *Format, ...)
{
    va_list ArgList;
    
    va_start(ArgList, Format);
    AppendFormatString_(State, Format, ArgList);
    va_end(ArgList);
}

inline string
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


string
EndFormatStringToBuffer(format_string_state *State, u8 *Buffer, umm BufferSize);

string
FormatStringToBuffer(u8 *Buffer, umm BufferSize, char *Format, ...);

inline void
StringToCString(string Text, u32 BufferSize, char *Buffer)
{
    if(BufferSize >= Text.Size + 1)
    {
        Copy(Text.Size, Text.Data, Buffer);
        Buffer[Text.Size] = '\0';
    }
    else
    {
        InvalidCodePath;
    }
}

u32
GetNullTerminiatedStringLength(char *Str);

inline string
CStringToString(char *NullTerminatedString)
{
    string Result = String_(GetNullTerminiatedStringLength(NullTerminatedString),
                            (u8 *)NullTerminatedString);
    return Result;
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

inline s32
StringComparisonLowercase(string A, string B)
{
    u32 IndexA = 0;
    u32 IndexB = 0;
    
    s32 Result = 0;
    if(!A.Size && !B.Size)
    {
        Result = 0;
    } 
    else if(!A.Size)
    {
        Result = -1;
    } 
    else if(!B.Size)
    {
        Result = 1;
    }
    else
    {
        while(CToLowercase(A.Data[IndexA]) == CToLowercase(B.Data[IndexB]))
        {
            if((IndexA == A.Size - 1) ||
               (IndexB == B.Size - 1))
            {
                break;
            }
            ++IndexA;
            ++IndexB;
        }
        
        Result = A.Data[IndexA] - B.Data[IndexB];
    }
    
    return Result;
    
}

internal char *
FindLastOccurrence(char *HayStack, char *Needle)
{
    char *Result = 0;
    
    char *At = HayStack;
    char *Match = Needle;
    char *CurrentMatch = 0;
    while(*At)
    {
        if(*At == *Match)
        {
            if(CurrentMatch == 0)
            {
                CurrentMatch = At;
            }
            if(Match[+1] == '\0')
            {
                Result = CurrentMatch;
            }
            Match++;
        }
        else if(CurrentMatch != 0)
        {
            --At;
            Match = Needle;
            CurrentMatch = 0;
        }
        else
        {
            Match = Needle;
            CurrentMatch = 0;
        }
        ++At;
    }
    
    return Result;
}

// TODO(kstandbridge): String FindFirstOccurrence
internal char *
FindFirstOccurrence(char *HayStack, char *Needle)
{
    char *Result = 0;
    
    char *At = HayStack;
    char *Match = Needle;
    while(*At)
    {
        if(*At == *Match)
        {
            if(Result == 0)
            {
                Result = At;
            }
            if(Match[+1] == '\0')
            {
                break;
            }
            Match++;
        }
        else
        {
            Match = Needle;
            Result = 0;
        }
        ++At;
    }
    
    return Result;
}

// TODO(kstandbridge): StringReplace
internal void
StringReplace(char *Result, char *HayStack, char *Needle, char *Replace)
{
    size_t Index;
    size_t ReplaceLength = GetNullTerminiatedStringLength(Replace);
    size_t NeedleLength = GetNullTerminiatedStringLength(Needle);
    
    for (Index = 0; HayStack[Index] != '\0'; Index++) 
    {
        if(FindFirstOccurrence(&HayStack[Index], Needle) == &HayStack[Index]) 
        {
            Index += NeedleLength - 1;
        }
    }
    
    Index = 0;
    while(*HayStack) 
    {
        if(FindFirstOccurrence(HayStack, Needle) == HayStack) 
        {
            if(ReplaceLength != 0)
            {
                Copy(ReplaceLength + 1, Replace, &Result[Index]);
            }
            Index += ReplaceLength;
            HayStack+= NeedleLength;
        }
        else
        {
            Result[Index++] = *HayStack++;
        }
    }
    
    Result[Index] = '\0';
}

internal b32
ParsePercentFromZ(char *At, char *ResultBuffer, s32 ResultBufferSize)
{
    b32 Result = false;
    while(*At)
    {
        
        if(*At == '%')
        {
            Result = true;
            break;
        }
        ++At;
    }
    if(Result)
    {
        --At;
        while(At != ResultBuffer)
        {
            if((*At == '.') ||
               (*At >= '0' && *At <= '9'))
            {
                --At;
            }
            else
            {
                ++At;
                break;
            }
        }
        char *Dest = ResultBuffer;
        s32 Index = 0;
        while(*At != '%')
        {
            *Dest++ = *At++;
            Index++;
            if(Index > ResultBufferSize)
            {
                InvalidCodePath;
                break;
            }
        }
        *Dest = '\0';
    }
    
    return Result;
}

// TODO(kstandbridge): F32FromString
internal f32
F32FromZ(char *At)
{
    f32 Result = 0;
    b32 DecimalFound = false;
    f32 CurrentDecimal = 1;
    
    while(At[0])
    {
        if(*At >= '0' && *At <= '9')
        {
            s32 Value = *At - 48;
            Assert(Value >= 0 && Value <= 9);
            if(DecimalFound)
            {
                CurrentDecimal /= 10.0f;
                Result += Value*CurrentDecimal;
            }
            else
            {
                Result *= 10;
                Result += Value;
            }
        }
        else if(*At == '.')
        {
            if(DecimalFound)
            {
                Assert(!"Additional Decimal found");
            }
            DecimalFound = true;
        }
        else
        {
            Assert(!"None float character");
        }
        ++At;
    }
    
    return Result;
}

internal char *
FindFirstOccurrenceLowercase(char *HayStack, char *Needle)
{
    char *Result = 0;
    
    char *At = HayStack;
    char *Match = Needle;
    while(*At)
    {
        if(CToLowercase(*At) == CToLowercase(*Match))
        {
            if(GetNullTerminiatedStringLength(At) < (GetNullTerminiatedStringLength(Match)))
            {
                Result = 0;
                break;
            }
            if(Result == 0)
            {
                Result = At;
            }
            if(Match[+1] == '\0')
            {
                break;
            }
            Match++;
        }
        else if(Result != 0)
        {
            --At;
            Match = Needle;
            Result = 0;
        }
        else
        {
            Match = Needle;
            Result = 0;
        }
        ++At;
        
    }
    
    return Result;
}

inline b32
StringsAreEqualLowercase(string A, string B)
{
    b32 Result = (A.Size == B.Size);
    
    if(Result)
    {
        Result = true;
        for(u32 Index = 0;
            Index < A.Size;
            ++Index)
        {
            if(CToLowercase(A.Data[Index]) != CToLowercase(B.Data[Index]))
            {
                Result = false;
                break;
            }
        }
    }
    
    return Result;
}

internal void
PushStringToStringList(string_list **HeadRef, memory_arena *Arena, string Text)
{
    string_list *Result = PushbackStringList(HeadRef, Arena);
    Result->Entry = Text;
}

internal b32
StringListMatch(void *Context, string_list *A, string_list *B)
{
    Context;
    
    b32 Result = (A && B);
    
    if(Result)
    {
        Result = StringsAreEqual(A->Entry, B->Entry);
    }
    
    return Result;
}

internal string *
GetStringFromStringList(string_list *Head, string Text)
{
    string *Result = 0;
    
    string_list Match = { .Entry = Text };
    string_list *Found = GetStringList(Head, StringListMatch, 0, &Match);
    if(Found)
    {
        Result = &Found->Entry;
    }
    
    return Result;
}

internal string *
GetStringFromStringListByIndex(string_list *Head, s32 Index)
{
    string *Result = 0;
    
    string_list *Found = GetStringListByIndex(Head, Index);
    if(Found)
    {
        Result = &Found->Entry;
    }
    
    return Result;
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

internal u8 *
StringAdvance(string *Text, umm Count)
{
    u8 *Result = 0;
    
    if(Text->Size >= Count)
    {
        Result = Text->Data;
        Text->Data += Count;
        Text->Size -= Count;
    }
    else
    {
        Text->Data += Text->Size;
        Text->Size = 0;
    }
    
    return Result;
}

inline b32
IsAlpha(char C)
{
    b32 Result = (((C >= 'a') && (C <= 'z')) ||
                  ((C >= 'A') && (C <= 'Z')));
    
    return Result;
}

inline b32
StringEndsWith(string Needle, string HayStack)
{
    string Suffix = String_(Needle.Size, HayStack.Data + HayStack.Size - Needle.Size);
    b32 Result = StringsAreEqual(Suffix, Needle);
    
    return Result;
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
Path_GetDirectory(char *Result, size_t Size, char *Source)
{
    Copy(Size, Source, Result);
    for(size_t Index = GetNullTerminiatedStringLength(Source);
        Index > 0;
        --Index)
    {
        if(Source[Index] == '\\')
        {
            Result[Index] = '\0';
            break;
        }
    }
}

#define KENGINE_STRING_H
#endif //KENGINE_STRING_H
