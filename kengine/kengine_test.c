#ifndef KENGINE_TEST_H

#define AssertTrue(Expression) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(!(Expression)) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): expression assert fail.\n", __FILE__, __LINE__); \
    }

#define AssertFalse(Expression) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if((Expression)) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): expression assert fail.\n", __FILE__, __LINE__); \
    }

#define AssertEqualString(Expected, Actual) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(!StringsAreEqual(Expected, Actual)) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%S'\n\t\t\tActual:      '%S'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertNotEqualString(Expected, Actual) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(StringsAreEqual(Expected, Actual)) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%S'\n\t\t\tActual:      '%S'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualU32(Expected, Actual) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(Expected != Actual) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%u'\n\t\t\tActual:      '%u'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualBits(Expected, Actual) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(Expected != Actual) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%b'\n\t\t\tActual:      '%b'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualBits16(Expected, Actual) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(Expected != Actual) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%16b'\n\t\t\tActual:      '%16b'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualHex(Expected, Actual) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(Expected != Actual) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%X'\n\t\t\tActual:      '%X'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualS32(Expected, Actual) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(Expected != Actual) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%d'\n\t\t\tActual:      '%d'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualF32(Expected, Actual)\
    { \
        AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
        f32 Difference = Expected - Actual; \
        if(!((Difference < 0.00001f) && (-Difference < 0.00001f))) \
        { \
            AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
            PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%f'\n\t\t\tActual:      '%f'\n", \
                                __FILE__, __LINE__, Expected, Actual);        \
        } \
    }

#define AssertEqualU64(Expected, Actual) \
    AtomicIncrementU32(&GlobalAppMemory->TotalTests); \
    if(Expected != Actual) \
    { \
        AtomicIncrementU32(&GlobalAppMemory->FailedTests); \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%lu'\n\t\t\tActual:      '%lu'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

typedef struct app_memory
{
    memory_arena Arena;
    
    u32 volatile TotalTests;
    u32 volatile FailedTests;

    u32 ArgCount;
    char **Args;

} app_memory;
global app_memory GlobalAppMemory_;
global app_memory *GlobalAppMemory = &GlobalAppMemory_;
typedef void platform_work_queue_callback(memory_arena *TransientArena, void *Data);


typedef struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Context;
} platform_work_queue_entry;

typedef struct platform_work_queue 
{
    platform_work_queue_entry Entries[256];
} platform_work_queue;

typedef struct win32_work_queue
{
    platform_work_queue PlatformQueue;
    u32 volatile CompletionGoal;
    u32 volatile CompletionCount;
    
    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;
    
    HANDLE SemaphoreHandle;
} win32_work_queue;

void
Win32AddWorkEntry(platform_work_queue *PlatformQueue, platform_work_queue_callback *Callback, void *Context)
{
    win32_work_queue *Win32Queue = (win32_work_queue *)PlatformQueue;
    u32 NewNextEntryToWrite = (Win32Queue->NextEntryToWrite + 1) % ArrayCount(Win32Queue->PlatformQueue.Entries);
    Assert(NewNextEntryToWrite != Win32Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Win32Queue->PlatformQueue.Entries + Win32Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Context = Context;
    ++Win32Queue->CompletionGoal;
    _WriteBarrier();
    Win32Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleaseSemaphore(Win32Queue->SemaphoreHandle, 1, 0);
}

internal b32
Win32DoNextWorkQueueEntry(platform_work_queue *PlatformQueue, memory_arena *TransientArena)
{
    win32_work_queue *Win32Queue = (win32_work_queue *)PlatformQueue;
    
    b32 WeShouldSleep = false;
    
    u32 OriginalNextEntryToRead = Win32Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Win32Queue->PlatformQueue.Entries);
    if(OriginalNextEntryToRead != Win32Queue->NextEntryToWrite)
    {
        u32 Index = AtomicCompareExchangeU32(&Win32Queue->NextEntryToRead,
                                             NewNextEntryToRead,
                                             OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {        
            platform_work_queue_entry Entry = Win32Queue->PlatformQueue.Entries[Index];
            Entry.Callback(TransientArena, Entry.Context);
            AtomicIncrementU32(&Win32Queue->CompletionCount);
        }
    }
    else
    {
        WeShouldSleep = true;
    }
    
    return WeShouldSleep;
}

void
Win32CompleteAllWork(platform_work_queue *PlatformQueue)
{
    win32_work_queue *Win32Queue = (win32_work_queue *)PlatformQueue;
    
    local_persist memory_arena TransientArena = {0};
    // NOTE(kstandbridge): Hack to initialize memory
    if(TransientArena.CurrentBlock == 0)
    {
        PushSize(&TransientArena, 1);
    }
    
    while(Win32Queue->CompletionGoal != Win32Queue->CompletionCount)
    {
        temporary_memory MemoryFlush = BeginTemporaryMemory(&TransientArena);
        Win32DoNextWorkQueueEntry(PlatformQueue, MemoryFlush.Arena);
        EndTemporaryMemory(MemoryFlush);
        CheckArena(&TransientArena);
    }
    
    Win32Queue->CompletionGoal = 0;
    Win32Queue->CompletionCount = 0;
}

DWORD WINAPI
Win32WorkQueueThread(void *lpParameter)
{
    win32_work_queue *Win32Queue = (win32_work_queue *)lpParameter;
    
    u32 TestThreadId = GetThreadID();
    if(TestThreadId != GetCurrentThreadId())
    {
        InvalidCodePath;
    }
    
    memory_arena TransientArena = {0};
    // NOTE(kstandbridge): Hack to initialize memory
    PushSize(&TransientArena, 1);
    
    for(;;)
    {
        temporary_memory MemoryFlush = BeginTemporaryMemory(&TransientArena);
        if(Win32DoNextWorkQueueEntry((platform_work_queue *)Win32Queue, MemoryFlush.Arena))
        {
            WaitForSingleObjectEx(Win32Queue->SemaphoreHandle, INFINITE, false);
        }
        EndTemporaryMemory(MemoryFlush);
        CheckArena(&TransientArena);
    }
}

platform_work_queue *
Win32MakeWorkQueue(memory_arena *Arena, u32 ThreadCount)
{
    win32_work_queue *Win32Queue = PushStruct(Arena, win32_work_queue);
    
    Win32Queue->CompletionGoal = 0;
    Win32Queue->CompletionCount = 0;
    
    Win32Queue->NextEntryToWrite = 0;
    Win32Queue->NextEntryToRead = 0;
    
    u32 InitialCount = 0;
    Win32Queue->SemaphoreHandle = CreateSemaphoreExA(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
    {
        u32 ThreadId;
        HANDLE ThreadHandle = CreateThread(0, 0, Win32WorkQueueThread, Win32Queue, 0, (LPDWORD)&ThreadId);
        CloseHandle(ThreadHandle);
    }
    
    platform_work_queue *Result = (platform_work_queue *)Win32Queue;
    return Result;
}

void
RunAllTests(platform_work_queue *WorkQueue);

s32
main(int ArgCount, char *Args[])
{
    s32 Result = 0;

    GlobalAppMemory->ArgCount = ArgCount - 1;
    GlobalAppMemory->Args = Args + 1;

    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;

    u16 ThreadCount = 4;

    HANDLE ProcessHandle = GetCurrentProcess();
    DWORD_PTR ProcessAffintyMask;
    DWORD_PTR SystemAffinityMask;
    if(GetProcessAffinityMask(ProcessHandle, &ProcessAffintyMask, &SystemAffinityMask))
    {
        for(ThreadCount = 0;
            ProcessAffintyMask;
            ProcessAffintyMask >>= 1)
        {
            if(ProcessAffintyMask & 0x1)
            {
                ++ThreadCount;
            }
        }
        
    }
    else
    {
        PlatformConsoleOut("Could not get process affinty mask.\n");
    }

    PlatformConsoleOut("Creating %d threads.\n", ThreadCount);
    platform_work_queue *WorkQueue = Win32MakeWorkQueue(&GlobalAppMemory->Arena, ThreadCount);
    
    // for(;;)
    {
        GlobalAppMemory->FailedTests = 0;
        GlobalAppMemory->TotalTests = 0;
        BeginProfile();

        BEGIN_TIMED_BLOCK(RunAllTests);
        RunAllTests(WorkQueue);
        Win32CompleteAllWork(WorkQueue);
        END_TIMED_BLOCK(RunAllTests);
        
        Result = (GlobalAppMemory->FailedTests != 0);
        
        PlatformConsoleOut("Unit Tests %s: %d/%d passed.\n", 
                        Result ? "Failed" : "Successful", GlobalAppMemory->TotalTests - GlobalAppMemory->FailedTests,                        GlobalAppMemory->TotalTests);

        EndAndPrintProfile();
    }
    
    return Result;
}

#define KENGINE_TEST_H
#endif // KENGINE_TEST_H