#include "win32_kengine.h"
#include "win32_kengine_shared.c"

global platform_api Platform;

#include "kengine_generated.c"
#include "win32_kengine_generated.c"
#include "kengine_sort.c"
#include "kengine_renderer_software.c"

internal void
Win32LoadLibraries()
{
    User32 = LoadLibraryA("User32.dll");
    Assert(User32);
    Gdi32 = LoadLibraryA("Gdi32.dll");
    Assert(Gdi32);
}

internal LRESULT
Win32MainWindowCallback_(win32_state *Win32State, HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CLOSE:
        {
            Win32State->IsRunning = false;
        } break;
        
        case WM_SIZE:
        {
            
            RECT ClientRect;
            if(GetClientRect(Window, &ClientRect))
            {
                win32_offscreen_buffer *Backbuffer = &Win32State->Backbuffer;
                Backbuffer->Width = ClientRect.right - ClientRect.left;
                Backbuffer->Height = ClientRect.bottom - ClientRect.top;
                
                
                if(Backbuffer->Memory)
                {
                    VirtualFree(Backbuffer->Memory, 0, MEM_RELEASE);
                }
                
                Backbuffer->Info.bmiHeader.biSize = sizeof(Backbuffer->Info.bmiHeader);
                Backbuffer->Info.bmiHeader.biWidth = Backbuffer->Width;
                Backbuffer->Info.bmiHeader.biHeight = Backbuffer->Height;
                Backbuffer->Info.bmiHeader.biPlanes = 1;
                Backbuffer->Info.bmiHeader.biBitCount = 32;
                Backbuffer->Info.bmiHeader.biCompression = BI_RGB;
                
                // TODO(kstandbridge): Need to figure out a way we can align this
                //Backbuffer->Pitch = Align16(Backbuffer->Width*BITMAP_BYTES_PER_PIXEL);
                Backbuffer->Pitch = Backbuffer->Width*BITMAP_BYTES_PER_PIXEL;
                
                Backbuffer->Memory = VirtualAlloc(0, sizeof(u32)*Backbuffer->Pitch*Backbuffer->Height,
                                                  MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                
            }
            else
            {
                // TODO(kstandbridge): Error failed to get client rect?
                InvalidCodePath;
            }
            
        } break;
        
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal LRESULT __stdcall
Win32MainWindowCallback(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    win32_state *Win32State = 0;
    
    if(Message == WM_NCCREATE)
    {
        CREATESTRUCTA *CreateStruct = (CREATESTRUCTA *)LParam;
        Win32State = (win32_state *)CreateStruct->lpCreateParams;
        SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)Win32State);
    }
    else
    {
        Win32State = (win32_state *)GetWindowLongPtrA(Window, GWLP_USERDATA);
    }
    
    if(Win32State)
    {        
        Result = Win32MainWindowCallback_(Win32State, Window, Message, WParam, LParam);
    }
    else
    {
        Result = DefWindowProcA(Window, Message, WParam, LParam);
    }
    
    return Result;
    
}

internal void
Win32ProcessPendingMessages(win32_state *Win32State)
{
    Win32State;
    
    MSG Message;
    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                __debugbreak();
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
}



internal FILETIME
Win32GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime;
    ZeroStruct(LastWriteTime);
    
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data))
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

inline void
AppendCString(char *StartAt, char *Text)
{
    while(*Text)
    {
        *StartAt++ = *Text++;
    }
}

internal void
Win32ParseCommandLingArgs(win32_state *Win32State)
{
    char *CommandLingArgs = GetCommandLineA();
    Assert(CommandLingArgs);
    
    char *At = CommandLingArgs;
    char *ParamStart = At;
    u32 ParamLength = 0;
    b32 Parsing = true;
    b32 ExeNameFound = false;
    while(Parsing)
    {
        if((*At == '\0') || IsWhitespace(*At))
        {
            if(*At != '\0')
            {
                while(IsWhitespace(*At))
                {
                    ++At;
                }
            }
            else
            {
                Parsing = false;
            }
            
            if(ExeNameFound)
            {            
                string Parameter;
                Parameter.Size = ParamLength;
                Parameter.Data = (u8 *)ParamStart;
                Parameter;
                // TODO(kstandbridge): Handle parameter
            }
            else
            {
                ExeNameFound = true;
                if(*ParamStart == '\"')
                {
                    ++ParamStart;
                    ParamLength -= 2;
                }
                char *LastSlash = At - 1;
                while(*LastSlash != '\\')
                {
                    --ParamLength;
                    --LastSlash;
                }
                ++ParamLength;
                Copy(ParamLength, ParamStart, Win32State->ExeFilePath);
                Win32State->ExeFilePath[ParamLength] = '\0';
                
                Copy(ParamLength, Win32State->ExeFilePath, Win32State->DllFullFilePath);
                AppendCString(Win32State->DllFullFilePath + ParamLength, "\\kengine.dll");
                
                Copy(ParamLength, Win32State->ExeFilePath, Win32State->TempDllFullFilePath);
                AppendCString(Win32State->TempDllFullFilePath + ParamLength, "\\kengine_temp.dll");
                
                Copy(ParamLength, Win32State->ExeFilePath, Win32State->LockFullFilePath);
                AppendCString(Win32State->LockFullFilePath + ParamLength, "\\lock.tmp");
            }
            
            ParamStart = At;
            ParamLength = 1;
            ++At;
        }
        else
        {
            ++ParamLength;
            ++At;
        }
        
    }
}


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
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal b32
Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
{
    b32 WeShouldSleep = false;
    
    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        u32 Index = _InterlockedCompareExchange(&Queue->NextEntryToRead,
                                                NewNextEntryToRead,
                                                OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {        
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Entry.Data);
            _InterlockedIncrement(&Queue->CompletionCount);
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

internal u32
WorkQueueThread(void *lpParameter)
{
    platform_work_queue *Queue = (platform_work_queue *)lpParameter;
    
    u32 TestThreadId = GetThreadId();
    Assert(TestThreadId == GetCurrentThreadId());
    
    for(;;)
    {
        if(Win32DoNextWorkQueueEntry(Queue))
        {
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, false);
        }
    }
}

internal void
MakeQueue(platform_work_queue *Queue, u32 ThreadCount)
{
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
    
    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;
    
    u32 InitialCount = 0;
    Queue->SemaphoreHandle = CreateSemaphoreExA(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
    {
        u32 ThreadId;
        HANDLE ThreadHandle = CreateThread(0, 0, WorkQueueThread, Queue, 0, &ThreadId);
        CloseHandle(ThreadHandle);
    }
}

void __stdcall 
WinMainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
    Win32LoadLibraries();
    
    HINSTANCE Instance = GetModuleHandleA(0);
    
    WNDCLASSEXA WindowClass;
    ZeroStruct(WindowClass);
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "KengineWindowClass";
    
    win32_state Win32State_;
    ZeroStruct(Win32State_);
    win32_state *Win32State = &Win32State_;
    
#if KENGINE_INTERNAL
    void *BaseAddress = (void *)Terabytes(2);
#else
    void *BaseAddress = 0;
#endif
    app_memory *AppMemory = &Win32State->AppMemory;
    AppMemory->StorageSize = Megabytes(16);
    AppMemory->Storage = VirtualAlloc(BaseAddress, AppMemory->StorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(AppMemory->Storage);
    
    // TODO(kstandbridge): Get number of cores, PerFrameWorkQueue should be cores*1.5, PerFrameWorkQueue should be cores*0.5
    platform_work_queue PerFrameWorkQueue;
    MakeQueue(&PerFrameWorkQueue, 6);
    AppMemory->PlatformAPI.PerFrameWorkQueue = &PerFrameWorkQueue;
    
    platform_work_queue BackgroundWorkQueue;
    MakeQueue(&BackgroundWorkQueue, 2);
    AppMemory->PlatformAPI.BackgroundWorkQueue = &BackgroundWorkQueue;
    
    AppMemory->PlatformAPI.AddWorkEntry = Win32AddWorkEntry;
    AppMemory->PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
    
    Platform = AppMemory->PlatformAPI;
    
    InitializeArena(&Win32State->Arena, AppMemory->StorageSize, AppMemory->Storage);
    
    Win32ParseCommandLingArgs(Win32State);
    
    if(RegisterClassExA(&WindowClass))
    {
        Win32State->Window = CreateWindowExA(0,
                                             WindowClass.lpszClassName,
                                             "kengine",
                                             WS_OVERLAPPEDWINDOW,
                                             CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                             0, 0, Instance, Win32State);
        if(Win32State->Window)
        {
            ShowWindow(Win32State->Window, SW_SHOW);
            
            u32 PushBufferSize = Megabytes(64);
            void *PushBuffer = VirtualAlloc(0, PushBufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            Win32State->IsRunning = true;
            while(Win32State->IsRunning)
            {
                
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(Win32State->DllFullFilePath);
                if(CompareFileTime(&NewDLLWriteTime, &Win32State->LastDLLWriteTime) != 0)
                {
                    WIN32_FILE_ATTRIBUTE_DATA Ignored;
                    if(!GetFileAttributesExA(Win32State->LockFullFilePath, GetFileExInfoStandard, &Ignored))
                    {
                        Win32CompleteAllWork(Platform.PerFrameWorkQueue);
                        Win32CompleteAllWork(Platform.BackgroundWorkQueue);
                        
                        if(Win32State->AppLibrary && !FreeLibrary(Win32State->AppLibrary))
                        {
                            // TODO(kstandbridge): Error freeing app library
                            InvalidCodePath;
                        }
                        Win32State->AppLibrary = 0;
                        Win32State->AppUpdateFrame = 0;
                        
                        if(CopyFileA(Win32State->DllFullFilePath, Win32State->TempDllFullFilePath, false))
                        {
                            Win32State->AppLibrary = LoadLibraryA(Win32State->TempDllFullFilePath);
                            if(Win32State->AppLibrary)
                            {
                                Win32State->AppUpdateFrame = (app_update_frame *)GetProcAddressA(Win32State->AppLibrary, "AppUpdateFrame");
                                if(!Win32State->AppUpdateFrame)
                                {
                                    // TODO(kstandbridge): Error AppUpdateFrame
                                    InvalidCodePath;
                                    if(!FreeLibrary(Win32State->AppLibrary))
                                    {
                                        // TODO(kstandbridge): Error freeing app library
                                        InvalidCodePath;
                                    }
                                }
                                Win32State->LastDLLWriteTime = NewDLLWriteTime;
                            }
                        }
                        else
                        {
                            // TODO(kstandbridge): Error copying temp dll
                            InvalidCodePath;
                        }
                    }
                }
                
                Win32ProcessPendingMessages(Win32State);
                
                HDC DeviceContext = GetDC(Win32State->Window);
                
                win32_offscreen_buffer *Backbuffer = &Win32State->Backbuffer;
                
                loaded_bitmap OutputTarget;
                ZeroStruct(OutputTarget);
                OutputTarget.Memory = Backbuffer->Memory;
                OutputTarget.Width = Backbuffer->Width;
                OutputTarget.Height = Backbuffer->Height;
                OutputTarget.Pitch = Backbuffer->Pitch;
                
                render_commands Commands = RenderCommands(PushBufferSize, 0, PushBuffer, 0, PushBufferSize);
                if(Win32State->AppUpdateFrame)
                {
                    Win32State->AppUpdateFrame(AppMemory->PlatformAPI, &Commands);
                }
                
                sort_entry *Entries = (sort_entry *)(Commands.PushBufferBase + Commands.SortEntryAt);
                BubbleSort(Commands.PushBufferElementCount, Entries);
                
                // NOTE(kstandbridge): Software renderer path
                {                
                    SoftwareRenderCommands(Platform.PerFrameWorkQueue, &Commands, &OutputTarget);
                    if(!StretchDIBits(DeviceContext, 0, 0, Backbuffer->Width, Backbuffer->Height, 0, 0, Backbuffer->Width, Backbuffer->Height,
                                      Backbuffer->Memory, &Backbuffer->Info, DIB_RGB_COLORS, SRCCOPY))
                    {
                        InvalidCodePath;
                    }
                }
                
                if(!ReleaseDC(Win32State->Window, DeviceContext))
                {
                    InvalidCodePath;
                }
                
                Win32CompleteAllWork(Platform.PerFrameWorkQueue);
                
                _mm_pause();
            }
        }
        else
        {
            // TODO(kstandbridge): Error creating window
        }
    }
    else
    {
        // TODO(kstandbridge): Error registering window class
    }
    
    ExitProcess(0);
    
    InvalidCodePath;
}