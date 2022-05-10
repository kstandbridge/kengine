#ifndef WIN32_KENGINE_SHARED_H

#include <Windows.h>

internal b32
ConsoleOutInternal(u8 *Data, umm Length)
{
    DWORD Result = 0;
    
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    WriteFile(OutputHandle, Data, (DWORD)Length, &Result, 0);
    
    Assert(Result == Length);
    return Result;
}

internal b32
ConsoleOut(memory_arena *Arena, char *Format, ...)
{
    va_list ArgList;
    
    va_start(ArgList, Format);
    string String = FormatStringInternal(Arena, Format, ArgList);
    va_end(ArgList);
    
    b32 Result = ConsoleOutInternal(String.Data, String.Length);
    return Result;
}

#define WIN32_KENGINE_SHARED_H
#endif //WIN32_KENGINE_SHARED_H