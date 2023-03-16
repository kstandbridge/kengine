#ifndef KENGINE_STRING_H

typedef struct string
{
    umm Size;
    u8 *Data;
} string;
#define NullString() String_(0, 0)
#define IsNullString(Text) ((Text.Data == 0) || (Text.Size == 0))

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

#define KENGINE_STRING_H
#endif //KENGINE_STRING_H
