
internal b32
Win32DirectoryExists(char *Path)
{
    b32 Result;
    
    u32 Attributes = Win32GetFileAttributesA(Path);
    Result = ((Attributes != INVALID_FILE_ATTRIBUTES) &&
              (Attributes & FILE_ATTRIBUTE_DIRECTORY));
    
    return Result;    
}

internal b32
Win32FileExists(char *Path)
{
    b32 Result;
    
    u32 Attributes = Win32GetFileAttributesA(Path);
    
    Result = ((Attributes != INVALID_FILE_ATTRIBUTES) && 
              !(Attributes & FILE_ATTRIBUTE_DIRECTORY));
    
    return Result;
}

internal b32
Win32ConsoleOut_(string Text)
{
    u32 Result = 0;
    
    HANDLE OutputHandle = Win32GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    Win32WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&Result, 0);
    Assert(Result == Text.Size);
    
    return Result;
}


internal b32
Win32ConsoleOut(memory_arena *Arena, char *Format, ...)
{
    format_string_state StringState = BeginFormatString(Arena);
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Text = EndFormatString(&StringState);
    
    b32 Result = Win32ConsoleOut_(Text);
    return Result;
}
