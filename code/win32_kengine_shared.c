
internal void
Win32LoadLibraries()
{
    User32 = Win32LoadLibraryA("User32.dll");
    Assert(User32);
    Gdi32 = Win32LoadLibraryA("Gdi32.dll");
    Assert(Gdi32);
    Winmm = Win32LoadLibraryA("Winmm.dll");
    Assert(Winmm);
    Opengl32 = Win32LoadLibraryA("Opengl32.dll");
    Assert(Opengl32);
}

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

internal FILETIME
Win32GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime;
    ZeroStruct(LastWriteTime);
    
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(Win32GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }
    else
    {
        // TODO(kstandbridge): Error failed to GetFileAttributesExA
        InvalidCodePath;
    }
    
    return LastWriteTime;
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
Win32AddWorkEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
{
    u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Queue->CompletionGoal;
    _WriteBarrier();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    Win32ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal b32
Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
{
    b32 WeShouldSleep = false;
    
    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        u32 Index = AtomicCompareExchangeU32(&Queue->NextEntryToRead,
                                             NewNextEntryToRead,
                                             OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {        
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Entry.Data);
            AtomicIncrementU32(&Queue->CompletionCount);
        }
    }
    else
    {
        WeShouldSleep = true;
    }
    
    return WeShouldSleep;
}

internal void
Win32CompleteAllWork(platform_work_queue *Queue)
{
    while(Queue->CompletionGoal != Queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(Queue);
    }
    
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

DWORD WINAPI
Win32WorkQueueThread(void *lpParameter)
{
    platform_work_queue *Queue = (platform_work_queue *)lpParameter;
    
    u32 TestThreadId = GetThreadID();
    Assert(TestThreadId == Win32GetCurrentThreadId());
    
    for(;;)
    {
        if(Win32DoNextWorkQueueEntry(Queue))
        {
            Win32WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, false);
        }
    }
}

internal void
Win32MakeQueue(platform_work_queue *Queue, u32 ThreadCount)
{
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
    
    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;
    
    u32 InitialCount = 0;
    Queue->SemaphoreHandle = Win32CreateSemaphoreExA(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
    {
        u32 ThreadId;
        HANDLE ThreadHandle = Win32CreateThread(0, 0, Win32WorkQueueThread, Queue, 0, (LPDWORD)&ThreadId);
        Win32CloseHandle(ThreadHandle);
    }
}

inline f32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End, s64 PerfCountFrequency)
{
    f32 Result = ((f32)(End.QuadPart - Start.QuadPart) /
                  (f32)PerfCountFrequency);
    return Result;
}


inline LARGE_INTEGER
Win32GetWallClock()
{    
    LARGE_INTEGER Result;
    Win32QueryPerformanceCounter(&Result);
    return Result;
}
