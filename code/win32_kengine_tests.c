#include "kengine_platform.h"
#include "kengine_shared.h"
#include "win32_kengine_shared.h"

global s32 TotalTests;
global s32 FailedTests;

#define ASSERT(Expression)                 \
++TotalTests;                          \
if(!(Expression))                      \
{                                      \
++FailedTests;                     \
ConsoleOut(Arena, "%s(%d): failed assert!\n", \
__FILE__, __LINE__);        \
}

internal s32
RunAllTests(memory_arena *Arena)
{
    s32 Test = 42;
    ASSERT(Test == 41);
    
    
    
    s32 Result = (FailedTests == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    return Result;
}

s32 __stdcall
mainCRTStartup()
{
#if KENGINE_INTERNAL
    LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
    LPVOID BaseAddress = 0;
#endif
    
    memory_index TotalMemorySize = Megabytes(8);
    void *MemoryBlock = VirtualAlloc(BaseAddress, TotalMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    memory_arena ArenaInternal;
    memory_arena *Arena = &ArenaInternal;;
    InitializeArena(Arena, TotalMemorySize, MemoryBlock);
    
    //ConsoleOut(&Arena, "Before %d mid %d after", 24, 42);
    //ConsoleOut(Arena, "Before %s foo %d bar %i end", "insert me", 123, 456);
    
    s32 Result = RunAllTests(Arena);
    
    if(MemoryBlock)
    {
        VirtualFree(MemoryBlock, 0, MEM_RELEASE);
    }
    
    ExitProcess(Result);
}
