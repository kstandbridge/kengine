#include "win32_kengine.h"
#include "win32_kengine_shared.c"

global platform_api Platform;

#include "win32_kengine_generated.c"
#include "kengine_sort.c"
#include "kengine_renderer_software.c"
#include "kengine_renderer.c"

internal void
Win32LoadLibraries()
{
    User32 = Win32LoadLibraryA("User32.dll");
    Assert(User32);
    Gdi32 = Win32LoadLibraryA("Gdi32.dll");
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
            if(Win32GetClientRect(Window, &ClientRect))
            {
                win32_offscreen_buffer *Backbuffer = &Win32State->Backbuffer;
                Backbuffer->Width = ClientRect.right - ClientRect.left;
                Backbuffer->Height = ClientRect.bottom - ClientRect.top;
                
                
                if(Backbuffer->Memory)
                {
                    Win32VirtualFree(Backbuffer->Memory, 0, MEM_RELEASE);
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
                
                Backbuffer->Memory = Win32VirtualAlloc(0, sizeof(u32)*Backbuffer->Pitch*Backbuffer->Height,
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
            Result = Win32DefWindowProcA(Window, Message, WParam, LParam);
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
        Win32SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)Win32State);
    }
    else
    {
        Win32State = (win32_state *)Win32GetWindowLongPtrA(Window, GWLP_USERDATA);
    }
    
    if(Win32State)
    {        
        Result = Win32MainWindowCallback_(Win32State, Window, Message, WParam, LParam);
    }
    else
    {
        Result = Win32DefWindowProcA(Window, Message, WParam, LParam);
    }
    
    return Result;
    
}

internal void
Win32ProcessPendingMessages(win32_state *Win32State)
{
    Win32State;
    
    MSG Message;
    while(Win32PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                __debugbreak();
            } break;
            
            default:
            {
                Win32TranslateMessage(&Message);
                Win32DispatchMessageA(&Message);
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
    char *CommandLingArgs = Win32GetCommandLineA();
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


#define MAX_GLYPH_COUNT 5000
global KERNINGPAIR *GlobalKerningPairs;
global u32 GlobalKerningPairCount;
global TEXTMETRICW GlobalTextMetric;

global HDC FontDeviceContext;
internal loaded_bitmap
Win32GetGlyphForCodePoint(memory_arena *Arena, u32 CodePoint)
{
    
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024
    
    local_persist b32 FontInitialized;
    local_persist void *FontBits;
    
    local_persist HFONT FontHandle;
    
    local_persist u32 CurrentGlyphIndex = 0;
    if(!FontInitialized)
    {
        FontDeviceContext = Win32CreateCompatibleDC(Win32GetDC(0));
        
        BITMAPINFO Info;
        ZeroStruct(Info);
        Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
        Info.bmiHeader.biWidth = MAX_FONT_WIDTH;
        Info.bmiHeader.biHeight = MAX_FONT_HEIGHT;
        Info.bmiHeader.biPlanes = 1;
        Info.bmiHeader.biBitCount = 32;
        Info.bmiHeader.biCompression = BI_RGB;
        Info.bmiHeader.biSizeImage = 0;
        Info.bmiHeader.biXPelsPerMeter = 0;
        Info.bmiHeader.biYPelsPerMeter = 0;
        Info.bmiHeader.biClrUsed = 0;
        Info.bmiHeader.biClrImportant = 0;
        HBITMAP Bitmap = Win32CreateDIBSection(FontDeviceContext, &Info, DIB_RGB_COLORS, &FontBits, 0, 0);
        Win32SelectObject(FontDeviceContext, Bitmap);
        Win32SetBkColor(FontDeviceContext, RGB(0, 0, 0));
        Win32AddFontResourceExA("c:/Windows/Fonts/segoeui.ttf", FR_PRIVATE, 0);
        s32 PointSize = 11;
        s32 FontHeight = -Win32MulDiv(PointSize, Win32GetDeviceCaps(FontDeviceContext, LOGPIXELSY), 72);
        FontHandle = Win32CreateFontA(FontHeight, 0, 0, 0,
                                      FW_NORMAL, // NOTE(kstandbridge): Weight
                                      false, // NOTE(kstandbridge): Italic
                                      false, // NOTE(kstandbridge): Underline
                                      false, // NOTE(kstandbridge): Strikeout
                                      DEFAULT_CHARSET, 
                                      OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, 
                                      ANTIALIASED_QUALITY,
                                      DEFAULT_PITCH|FF_DONTCARE,
                                      "Segoe UI");
        
        Win32SelectObject(FontDeviceContext, FontHandle);
        Win32GetTextMetricsW(FontDeviceContext, &GlobalTextMetric);
        
        GlobalKerningPairCount = Win32GetKerningPairsW(FontDeviceContext, 0, 0);
        GlobalKerningPairs = PushArray(Arena, GlobalKerningPairCount, KERNINGPAIR);
        Win32GetKerningPairsW(FontDeviceContext, GlobalKerningPairCount, GlobalKerningPairs);
        
        FontInitialized = true;
    }
    
    loaded_bitmap Result;
    ZeroStruct(Result);
    
    Win32SelectObject(FontDeviceContext, FontHandle);
    
    ZeroSize(MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32), FontBits);
    
    wchar_t CheesePoint = (wchar_t)CodePoint;
    
    SIZE Size;
    Win32GetTextExtentPoint32W(FontDeviceContext, &CheesePoint, 1, &Size);
    
    s32 PreStepX = 128;
    
    s32 BoundWidth = Size.cx + 2*PreStepX;
    if(BoundWidth > MAX_FONT_WIDTH)
    {
        BoundWidth = MAX_FONT_WIDTH;
    }
    s32 BoundHeight = Size.cy;
    if(BoundHeight > MAX_FONT_HEIGHT)
    {
        BoundHeight = MAX_FONT_HEIGHT;
    }
    
    Win32SetBkMode(FontDeviceContext, TRANSPARENT);
    Win32SetTextColor(FontDeviceContext, RGB(255, 255, 255));
    Win32TextOutW(FontDeviceContext, PreStepX, 0, &CheesePoint, 1);
    
    s32 MinX = 10000;
    s32 MinY = 10000;
    s32 MaxX = -10000;
    s32 MaxY = -10000;
    
    u32 *Row = (u32 *)FontBits + (MAX_FONT_HEIGHT - 1)*MAX_FONT_WIDTH;
    for(s32 Y = 0;
        Y < BoundHeight;
        ++Y)
    {
        u32 *Pixel = Row;
        for(s32 X = 0;
            X < BoundWidth;
            ++X)
        {
            
#if 0            
            COLORREF RefPixel = GetPixel(FontDeviceContext, X, Y);
            Assert(RefPixel == *Pixel);
#endif
            
            if(*Pixel != 0)
            {
                if(MinX > X)
                {
                    MinX = X;                    
                }
                
                if(MinY > Y)
                {
                    MinY = Y;                    
                }
                
                if(MaxX < X)
                {
                    MaxX = X;                    
                }
                
                if(MaxY < Y)
                {
                    MaxY = Y;                    
                }
            }
            
            ++Pixel;
        }
        Row -= MAX_FONT_WIDTH;
    }
    
    f32 KerningChange = 0;
    if(MinX <= MaxX)
    {
        s32 Width = (MaxX - MinX) + 1;
        s32 Height = (MaxY - MinY) + 1;
        
        Result.Width = Width + 2;
        Result.Height = Height + 2;
        Result.WidthOverHeight = SafeRatio1((f32)Result.Width, (f32)Result.Height);
        Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
        Result.Memory = PushSize(Arena, Result.Height*Result.Pitch);
        
        ZeroSize(Result.Height*Result.Pitch, Result.Memory);
        
        u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch;
        u32 *SourceRow = (u32 *)FontBits + (MAX_FONT_HEIGHT - 1 - MinY)*MAX_FONT_WIDTH;
        for(s32 Y = MinY;
            Y <= MaxY;
            ++Y)
        {
            u32 *Source = (u32 *)SourceRow + MinX;
            u32 *Dest = (u32 *)DestRow + 1;
            for(s32 X = MinX;
                X <= MaxX;
                ++X)
            {
                u32 Pixel = *Source;
                
                f32 Alpha = (f32)(Pixel & 0xFF);
                if(Pixel == 0)
                {
                    Alpha = 0.0f;
                }
                
                v4 Texel = V4(255.0f, 255.0f, 255.0f, Alpha);
                
                Texel = SRGB255ToLinear1(Texel);
                Texel.R *= Texel.A;
                Texel.G *= Texel.A;
                Texel.B *= Texel.A;
                Texel = Linear1ToSRGB255(Texel);
                
                *Dest++ = (((u32)(Texel.A + 0.5f) << 24) |
                           ((u32)(Texel.R + 0.5f) << 16) |
                           ((u32)(Texel.G + 0.5f) << 8) |
                           ((u32)(Texel.B + 0.5f) << 0));
                
                ++Source;
            }
            
            DestRow -= Result.Pitch;
            SourceRow -= MAX_FONT_WIDTH;
        }
        
        Result.AlignPercentage.X = (1.0f) / (f32)Result.Width;
        Result.AlignPercentage.Y = (1.0f + (MaxY - (BoundHeight - GlobalTextMetric.tmDescent))) / (f32)Result.Height;
        
        KerningChange = (f32)(MinX - PreStepX);
        
    }
    
    return Result;
}

internal f32
Win32GetHorizontalAdvance(u32 PrevCodePoint, u32 CodePoint)
{
    s32 ThisWidth;
    Win32GetCharWidth32W(FontDeviceContext, PrevCodePoint, CodePoint, &ThisWidth);
    f32 Result = (f32)ThisWidth; 
    
    for(DWORD KerningPairIndex = 0;
        KerningPairIndex < GlobalKerningPairCount;
        ++KerningPairIndex)
    {
        KERNINGPAIR *Pair = GlobalKerningPairs + KerningPairIndex;
        if((Pair->wFirst == PrevCodePoint) &&
           (Pair->wSecond == CodePoint))
        {
            Result += Pair->iKernAmount;
            break;
        }
    }
    
    return Result;
}

internal f32
Win32GetVerticleAdvance()
{
    f32 Result = (f32)GlobalTextMetric.tmAscent + (f32)GlobalTextMetric.tmDescent + (f32)GlobalTextMetric.tmDescent;
    
    return Result;
}


void __stdcall 
WinMainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
    Win32LoadLibraries();
    
    HINSTANCE Instance = Win32GetModuleHandleA(0);
    
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
    
    platform_api PlatformAPI;
    
    // TODO(kstandbridge): Get number of cores, PerFrameWorkQueue should be cores*1.5, PerFrameWorkQueue should be cores*0.5
    platform_work_queue PerFrameWorkQueue;
    Win32MakeQueue(&PerFrameWorkQueue, 6);
    PlatformAPI.PerFrameWorkQueue = &PerFrameWorkQueue;
    
    platform_work_queue BackgroundWorkQueue;
    Win32MakeQueue(&BackgroundWorkQueue, 2);
    PlatformAPI.BackgroundWorkQueue = &BackgroundWorkQueue;
    
    PlatformAPI.AddWorkEntry = Win32AddWorkEntry;
    PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
    PlatformAPI.GetGlyphForCodePoint = Win32GetGlyphForCodePoint;
    PlatformAPI.GetHorizontalAdvance = Win32GetHorizontalAdvance;
    PlatformAPI.GetVerticleAdvance = Win32GetVerticleAdvance;;
    
    Platform = PlatformAPI;
    
    u64 StorageSize = Megabytes(16);
    void *Storage = Win32VirtualAlloc(BaseAddress, StorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Storage);
    InitializeArena(&Win32State->Arena, StorageSize, Storage);
    
    Win32ParseCommandLingArgs(Win32State);
    
    if(Win32RegisterClassExA(&WindowClass))
    {
        Win32State->Window = Win32CreateWindowExA(0,
                                                  WindowClass.lpszClassName,
                                                  "kengine",
                                                  WS_OVERLAPPEDWINDOW,
                                                  CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                                  0, 0, Instance, Win32State);
        if(Win32State->Window)
        {
            Win32ShowWindow(Win32State->Window, SW_SHOW);
            
            u32 PushBufferSize = Megabytes(64);
            void *PushBuffer = Win32VirtualAlloc(0, PushBufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            u32 CurrentSortMemorySize = Kilobytes(64);
            void *SortMemory = Win32VirtualAlloc(0, CurrentSortMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            u32 CurrentClipMemorySize = Kilobytes(64);
            void *ClipMemory = Win32VirtualAlloc(0, CurrentClipMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            Win32State->IsRunning = true;
            while(Win32State->IsRunning)
            {
                
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(Win32State->DllFullFilePath);
                if(Win32CompareFileTime(&NewDLLWriteTime, &Win32State->LastDLLWriteTime) != 0)
                {
                    WIN32_FILE_ATTRIBUTE_DATA Ignored;
                    if(!Win32GetFileAttributesExA(Win32State->LockFullFilePath, GetFileExInfoStandard, &Ignored))
                    {
                        Win32CompleteAllWork(Platform.PerFrameWorkQueue);
                        Win32CompleteAllWork(Platform.BackgroundWorkQueue);
                        
                        if(Win32State->AppLibrary && !Win32FreeLibrary(Win32State->AppLibrary))
                        {
                            // TODO(kstandbridge): Error freeing app library
                            InvalidCodePath;
                        }
                        Win32State->AppLibrary = 0;
                        Win32State->AppUpdateFrame = 0;
                        
                        if(Win32CopyFileA(Win32State->DllFullFilePath, Win32State->TempDllFullFilePath, false))
                        {
                            Win32State->AppLibrary = Win32LoadLibraryA(Win32State->TempDllFullFilePath);
                            if(Win32State->AppLibrary)
                            {
                                Win32State->AppUpdateFrame = (app_update_frame *)Win32GetProcAddressA(Win32State->AppLibrary, "AppUpdateFrame");
                                if(!Win32State->AppUpdateFrame)
                                {
                                    // TODO(kstandbridge): Error AppUpdateFrame
                                    InvalidCodePath;
                                    if(!Win32FreeLibrary(Win32State->AppLibrary))
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
                
                HDC DeviceContext = Win32GetDC(Win32State->Window);
                
                win32_offscreen_buffer *Backbuffer = &Win32State->Backbuffer;
                
                loaded_bitmap OutputTarget;
                ZeroStruct(OutputTarget);
                OutputTarget.Memory = Backbuffer->Memory;
                OutputTarget.Width = Backbuffer->Width;
                OutputTarget.Height = Backbuffer->Height;
                OutputTarget.Pitch = Backbuffer->Pitch;
                
                render_commands Commands_ = BeginRenderCommands(PushBufferSize, PushBuffer, Backbuffer->Width, Backbuffer->Height);
                render_commands *Commands = &Commands_;
                
                if(Win32State->AppUpdateFrame)
                {
                    Win32State->AppUpdateFrame(&PlatformAPI, Commands, &Win32State->Arena);
                }
                
                
                u32 NeededSortMemorySize = Commands->PushBufferElementCount * sizeof(sort_entry);
                if(CurrentSortMemorySize < NeededSortMemorySize)
                {
                    Win32VirtualFree(SortMemory, 0, MEM_RELEASE);
                    while(CurrentSortMemorySize < NeededSortMemorySize)
                    {
                        CurrentSortMemorySize *= 2;
                    }
                    SortMemory = Win32VirtualAlloc(0, CurrentSortMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                }
                
                // TODO(kstandbridge): Can we merge sort/clip and push buffer memory together?
                u32 NeededClipMemorySize = Commands->PushBufferElementCount * sizeof(sort_entry);
                if(CurrentClipMemorySize < NeededClipMemorySize)
                {
                    Win32VirtualFree(ClipMemory, 0, MEM_RELEASE);
                    while(CurrentClipMemorySize < NeededClipMemorySize)
                    {
                        CurrentClipMemorySize *= 2;
                    }
                    ClipMemory = Win32VirtualAlloc(0, CurrentClipMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                }
                
                SortRenderCommands(Commands, SortMemory);
                LinearizeClipRects(Commands, ClipMemory);
                
                // NOTE(kstandbridge): Software renderer path
                {                
                    SoftwareRenderCommands(Platform.PerFrameWorkQueue, Commands, &OutputTarget);
                    if(!Win32StretchDIBits(DeviceContext, 0, 0, Backbuffer->Width, Backbuffer->Height, 0, 0, Backbuffer->Width, Backbuffer->Height,
                                           Backbuffer->Memory, &Backbuffer->Info, DIB_RGB_COLORS, SRCCOPY))
                    {
                        InvalidCodePath;
                    }
                }
                
                EndRenderCommands(Commands);
                
                if(!Win32ReleaseDC(Win32State->Window, DeviceContext))
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
    
    Win32ExitProcess(0);
    
    InvalidCodePath;
}