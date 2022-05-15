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

typedef struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
} platform_work_queue_entry;


typedef struct platform_work_queue
{
    u32 volatile CompletionGoal;
    u32 volatile CompletionCount;
    
    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;
    HANDLE SemaphoreHandle;
    
    platform_work_queue_entry Entries[256];
} platform_work_queue;

internal void
AddWorkEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
{
    // TODO(casey): Switch to InterlockedCompareExchange eventually
    // so that any thread can add?
    u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Queue->CompletionGoal;
    _WriteBarrier();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal b32
DoNextWorkQueueEntry(platform_work_queue *Queue)
{
    b32 WeShouldSleep = false;
    
    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        u32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                               NewNextEntryToRead,
                                               OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {        
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Entry.Data);
            InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
        }
    }
    else
    {
        WeShouldSleep = true;
    }
    
    return WeShouldSleep;
}

internal void
CompleteAllWork(platform_work_queue *Queue)
{
    while(Queue->CompletionGoal != Queue->CompletionCount)
    {
        DoNextWorkQueueEntry(Queue);
    }
    
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
    platform_work_queue *Queue = (platform_work_queue *)lpParameter;
    
    u32 TestThreadID = GetThreadID();
    Assert(TestThreadID == GetCurrentThreadId());
    
    for(;;)
    {
        if(DoNextWorkQueueEntry(Queue))
        {
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }
    
    //    return(0);
}

internal void
MakeQueue(platform_work_queue *Queue, u32 ThreadCount)
{
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
    
    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;
    
    u32 InitialCount = 0;
    Queue->SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount,
                                               0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
    {
        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Queue, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
}



#define WIN32_KENGINE_SHARED_H
#endif //WIN32_KENGINE_SHARED_H