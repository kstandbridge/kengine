#ifndef WIN32_KENGINE_SHARED_H

#include <Windows.h>

internal b32
ConsoleOutInternal(string Str)
{
    DWORD Result = 0;
    
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    WriteFile(OutputHandle, Str.Data, (DWORD)Str.Size, &Result, 0);
    
    Assert(Result == Str.Size);
    return Result;
}

internal b32
ConsoleOut(memory_arena *Arena, char *Format, ...)
{
    va_list ArgList;
    
    va_start(ArgList, Format);
    string Str = FormatStringInternal(Arena, Format, ArgList);
    va_end(ArgList);
    
    b32 Result = ConsoleOutInternal(Str);
    return Result;
}

inline memory_arena
AllocateArena(memory_index TotalMemorySize)
{
    memory_arena Result;
    
#if KENGINE_INTERNAL
    LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
    LPVOID BaseAddress = 0;
#endif
    
    void *MemoryBlock = VirtualAlloc(BaseAddress, TotalMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    InitializeArena(&Result, TotalMemorySize, MemoryBlock);
    
    return Result;
}

inline string
DEBUGReadEntireFile(memory_arena *Arena, char *FilePath)
{
    string Result;
    ZeroStruct(Result);
    
    HANDLE FileHandle = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    LARGE_INTEGER FileSize;
    b32 ReadResult = GetFileSizeEx(FileHandle, &FileSize);
    Assert(ReadResult);
    
    Result.Size = (memory_index)FileSize.QuadPart;
    Result.Data = PushSize(Arena, Result.Size);
    Assert(Result.Data);
    
    DWORD BytesRead;
    ReadResult = ReadFile(FileHandle, Result.Data, (DWORD)Result.Size, &BytesRead, 0);
    Assert(ReadResult);
    Assert(BytesRead == Result.Size);
    
    CloseHandle(FileHandle);
    
    return Result;
}


#define WIN32_KENGINE_SHARED_H
#endif //WIN32_KENGINE_SHARED_H