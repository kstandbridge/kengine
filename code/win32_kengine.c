#include "kengine_platform.h"
#include "kengine_memory.h"
#include "kengine_generated.h"
#include "win32_kengine_types.h"
#include "win32_kengine_opengl.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "kengine_renderer_shared.h"

#ifndef VERSION
#define VERSION 0
#endif // VERSION

#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"

global app_memory GlobalAppMemory;

#include "kengine_debug_shared.h"
#if KENGINE_INTERNAL
#include "kengine_debug.h"
global debug_event_table GlobalDebugEventTable_;
debug_event_table *GlobalDebugEventTable = &GlobalDebugEventTable_;
#include "kengine_debug.c"
#endif

#if KENGINE_INTERNAL
#include "kengine_sort.c"
#include "kengine_telemetry.c"
#else
#include "kengine.h"
#include "kengine.c"
#endif
#include "kengine_renderer.c"
#include "win32_kengine_shared.c"
#if KENGINE_HTTP
#include "win32_kengine_http.c"
#endif

internal LRESULT __stdcall
Win32MainWindowCallback(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CLOSE:
        {
            GlobalAppMemory.IsRunning = false;
        } break;
        
        default:
        {
            Result = Win32DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal void
Win32ProcessPendingMessages(app_input *Input)
{
    MSG Message;
    while(Win32PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
#if KENGINE_INTERNAL
                __debugbreak();
#endif
            } break;
            
            case WM_MOUSEWHEEL:
            {
                Input->MouseZ = GET_WHEEL_DELTA_WPARAM(Message.wParam);
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)Message.wParam;
                
                b32 AltKeyWasDown = (Message.lParam & (1 << 29));
                b32 ShiftKeyWasDown = (Win32GetKeyState(VK_SHIFT) & (1 << 15));
                
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1UL << 31)) == 0);
                if(WasDown != IsDown)
                {
                    // TODO(kstandbridge): Process individual keys?
                }
                
                if(IsDown)
                {
                    if((VKCode == VK_F4) && AltKeyWasDown)
                    {
                        GlobalAppMemory.IsRunning = false;
                    }
                    else if((VKCode >= VK_F1) && (VKCode <= VK_F12))
                    {
                        Input->FKeyPressed[VKCode - VK_F1 + 1] = true;
                    }
                }
                
            } break;
            
            default:
            {
                Win32TranslateMessage(&Message);
                Win32DispatchMessageA(&Message);
            } break;
        }
    }
}

#if KENGINE_CONSOLE
inline void
Win32ConsoleCommandLoopThread(void *Data)
{
    for(;;)
    {
        HANDLE InputHandle = Win32GetStdHandle(STD_INPUT_HANDLE);
        Assert(InputHandle != INVALID_HANDLE_VALUE);
        
        u8 Buffer[MAX_PATH];
        umm BytesRead = 0;
        Win32ReadFile(InputHandle, Buffer, (DWORD)sizeof(Buffer), (LPDWORD)&BytesRead, 0);
        string Command = String_(BytesRead - 2, Buffer);
        
        // TODO(kstandbridge): Seperate command and arguments
        
#if KENGINE_INTERNAL
        if(GlobalWin32State.AppHandleCommand)
        {
            GlobalWin32State.AppHandleCommand(&GlobalAppMemory, Command, 0, 0);
        }
        else
        {
            LogWarning("Ignoring '%S' as no command handler setup", Command);
        }
#else // KENGINE_INTERNAL
        AppHandleCommand(&GlobalAppMemory, Command, 0, 0);
#endif // KENGINE_INTERNAL
    }
}

internal void
Win32InitConsoleCommandLoop(platform_work_queue *Queue)
{
    Win32AddWorkEntry(Queue, Win32ConsoleCommandLoopThread, 0);
}
#endif

#define MAX_GLYPH_COUNT 5000
global KERNINGPAIR *GlobalKerningPairs;
global u32 GlobalKerningPairCount;
global TEXTMETRICA GlobalTextMetric;

global HDC FontDeviceContext;
internal loaded_glyph
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
        
#if 1
        Win32AddFontResourceExA("c:/Windows/Fonts/segoeui.ttf", FR_PRIVATE, 0);
        s32 PointSize = 11;
        s32 FontHeight = -MulDiv(PointSize, Win32GetDeviceCaps(FontDeviceContext, LOGPIXELSY), 72);
        FontHandle = Win32CreateFontA(FontHeight, 0, 0, 0,
                                      FW_NORMAL, // NOTE(kstandbridge): Weight
                                      FALSE, // NOTE(kstandbridge): Italic
                                      FALSE, // NOTE(kstandbridge): Underline
                                      FALSE, // NOTE(kstandbridge): Strikeout
                                      DEFAULT_CHARSET, 
                                      OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, 
                                      ANTIALIASED_QUALITY,
                                      DEFAULT_PITCH|FF_DONTCARE,
                                      "Segoe UI");
#else
        Win32AddFontResourceExA("c:/Windows/Fonts/LiberationMono-Regular.ttf", FR_PRIVATE, 0);
        FontHandle = Win32CreateFontA(20, 0, 0, 0,
                                      FW_NORMAL, // NOTE(kstandbridge): Weight
                                      FALSE, // NOTE(kstandbridge): Italic
                                      FALSE, // NOTE(kstandbridge): Underline
                                      FALSE, // NOTE(kstandbridge): Strikeout
                                      DEFAULT_CHARSET, 
                                      OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, 
                                      ANTIALIASED_QUALITY,
                                      DEFAULT_PITCH|FF_DONTCARE,
                                      "Liberation Mono");
#endif
        
        Win32SelectObject(FontDeviceContext, FontHandle);
        Win32GetTextMetricsA(FontDeviceContext, &GlobalTextMetric);
        
        GlobalKerningPairCount = Win32GetKerningPairsW(FontDeviceContext, 0, 0);
        GlobalKerningPairs = PushArray(Arena, GlobalKerningPairCount, KERNINGPAIR);
        Win32GetKerningPairsW(FontDeviceContext, GlobalKerningPairCount, GlobalKerningPairs);
        
        FontInitialized = true;
    }
    
    loaded_glyph Result;
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
    
    //Win32SetBkMode(FontDeviceContext, TRANSPARENT);
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
            
#if KENGINE_SLOW
            COLORREF RefPixel = Win32GetPixel(FontDeviceContext, X, Y);
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
    
    if(MinX <= MaxX)
    {
        s32 Width = (MaxX - MinX) + 1;
        s32 Height = (MaxY - MinY) + 1;
        
        Result.Bitmap.Width = Width + 2;
        Result.Bitmap.Height = Height + 2;
        Result.Bitmap.WidthOverHeight = SafeRatio1((f32)Result.Bitmap.Width, (f32)Result.Bitmap.Height);
        Result.Bitmap.Pitch = Result.Bitmap.Width*BITMAP_BYTES_PER_PIXEL;
        Result.Bitmap.Memory = PushSize(Arena, Result.Bitmap.Height*Result.Bitmap.Pitch);
        
        ZeroSize(Result.Bitmap.Height*Result.Bitmap.Pitch, Result.Bitmap.Memory);
        
        u8 *DestRow = (u8 *)Result.Bitmap.Memory + (Result.Bitmap.Height - 1 - 1)*Result.Bitmap.Pitch;
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
                
#if KENGINE_SLOW
                COLORREF Pixel = Win32GetPixel(FontDeviceContext, X, Y);
                Assert(Pixel == *Source);
#else
                u32 Pixel = *Source;
#endif
                
                f32 Gray = (f32)(Pixel & 0xFF);
                v4 Texel = V4(255.0f, 255.0f, 255.0f, Gray);
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
            
            DestRow -= Result.Bitmap.Pitch;
            SourceRow -= MAX_FONT_WIDTH;
        }
        
        Result.Bitmap.AlignPercentage.X = (1.0f) / (f32)Result.Bitmap.Width;
        Result.Bitmap.AlignPercentage.Y = (1.0f + (MaxY - (BoundHeight - GlobalTextMetric.tmDescent))) / (f32)Result.Bitmap.Height;
        
        Result.KerningChange = (f32)(MinX - PreStepX);
        
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

internal umm
Win32GetHostname(u8 *Buffer, umm BufferMaxSize)
{
    DWORD Result = BufferMaxSize;
    
    if(!GetComputerNameA((char *)Buffer, &Result))
    {
        Win32LogError("Failed to get computer name");
    }
    
    return Result;
}

internal umm
Win32GetHomeDirectory(u8 *Buffer, umm BufferMaxSize)
{
    DWORD Result = ExpandEnvironmentStringsA("%UserProfile%", (char *)Buffer, BufferMaxSize);
    
    if(Result == 0)
    {
        Win32LogError("Failed to expand environment strings");
    }
    else
    {
        // NOTE(kstandbridge): Remove the null terminator
        --Result;
    }
    
    return Result;
}

internal umm
Win32GetAppConfigDirectory(u8 *Buffer, umm BufferMaxSize)
{
    DWORD Result = ExpandEnvironmentStringsA("%AppData%", (char *)Buffer, BufferMaxSize);
    
    if(Result == 0)
    {
        Win32LogError("Failed to expand environment strings");
    }
    else
    {
        // NOTE(kstandbridge): Remove the null terminator
        --Result;
    }
    
    return Result;
}

internal umm
Win32GetUsername(u8 *Buffer, umm BufferMaxSize)
{
    umm Result = GetEnvironmentVariableA("Username", (char *)Buffer, BufferMaxSize);
    
    if(Result == 0)
    {
        Win32LogError("Failed to get environment variable");
    }
    
    return Result;
}

internal u32
Win32GetProcessId()
{
    u32 Result = GetCurrentProcessId();
    
    return Result;
}

inline void
ProcessInputMessage(app_button_state *NewState, b32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

#if KENGINE_CONSOLE
s32 __stdcall
mainCRTStartup()
#else
void __stdcall 
WinMainCRTStartup()
#endif
{
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    
#if !defined(KENGINE_CONSOLE) && !defined(KENGINE_HEADLESS)
    HINSTANCE Instance = GetModuleHandleA(0);
    
    WNDCLASSEXA WindowClass;
    ZeroStruct(WindowClass);
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "KengineWindowClass";
#endif // !defined(KENGINE_CONSOLE) && !defined(KENGINE_HEADLESS)
    
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalWin32State.PerfCountFrequency = (s64)PerfCountFrequencyResult.QuadPart;
    GlobalAppMemory.PlatformAPI.MakeWorkQueue = Win32MakeWorkQueue;
    GlobalAppMemory.PlatformAPI.AddWorkEntry = Win32AddWorkEntry;
    GlobalAppMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
    
#if KENGINE_CONSOLE
    GlobalAppMemory.PlatformAPI.InitConsoleCommandLoop = Win32InitConsoleCommandLoop;
#endif
    
    GlobalAppMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
    GlobalAppMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;
    GlobalAppMemory.PlatformAPI.GetMemoryStats = Win32GetMemoryStats;
    
    GlobalAppMemory.PlatformAPI.GetGlyphForCodePoint = Win32GetGlyphForCodePoint;
    GlobalAppMemory.PlatformAPI.GetHorizontalAdvance = Win32GetHorizontalAdvance;
    GlobalAppMemory.PlatformAPI.GetVerticleAdvance = Win32GetVerticleAdvance;
    
    GlobalAppMemory.PlatformAPI.DeleteHttpCache = Win32DeleteHttpCache;
    GlobalAppMemory.PlatformAPI.EndHttpClient = Win32EndHttpClient;
    GlobalAppMemory.PlatformAPI.BeginHttpClientWithCreds = Win32BeginHttpClientWithCreds;
    GlobalAppMemory.PlatformAPI.BeginHttpClient = Win32BeginHttpClient;
    GlobalAppMemory.PlatformAPI.SkipHttpMetrics = Win32SkipHttpMetrics;
    GlobalAppMemory.PlatformAPI.SetHttpClientTimeout = Win32SetHttpClientTimeout;
    GlobalAppMemory.PlatformAPI.EndHttpRequest = Win32EndHttpRequest;
    GlobalAppMemory.PlatformAPI.BeginHttpRequest = Win32BeginHttpRequest;
    GlobalAppMemory.PlatformAPI.SetHttpRequestHeaders = Win32SetHttpRequestHeaders;
    GlobalAppMemory.PlatformAPI.SendHttpRequestFromFile = Win32SendHttpRequestFromFile;
    GlobalAppMemory.PlatformAPI.SendHttpRequest = Win32SendHttpRequest;
    GlobalAppMemory.PlatformAPI.GetHttpResponseToFile = Win32GetHttpResponseToFile;
    GlobalAppMemory.PlatformAPI.GetHttpResponse = Win32GetHttpResponse;
    
    GlobalAppMemory.PlatformAPI.ReadEntireFile = Win32ReadEntireFile;
    GlobalAppMemory.PlatformAPI.WriteTextToFile = Win32WriteTextToFile;
    GlobalAppMemory.PlatformAPI.ZipDirectory = Win32ZipDirectory;
    GlobalAppMemory.PlatformAPI.UnzipToDirectory = Win32UnzipToDirectory;
    GlobalAppMemory.PlatformAPI.FileExists = Win32FileExists;
    GlobalAppMemory.PlatformAPI.PermanentDeleteFile = Win32PermanentDeleteFile;
    GlobalAppMemory.PlatformAPI.DirectoryExists = Win32DirectoryExists;
    GlobalAppMemory.PlatformAPI.CreateDirectory = Win32CreateDirectory;
    GlobalAppMemory.PlatformAPI.RequestCloseProcess = Win32RequestCloseProcess;
    GlobalAppMemory.PlatformAPI.KillProcess = Win32KillProcess;
    GlobalAppMemory.PlatformAPI.CheckForProcess = Win32CheckForProcess;
    GlobalAppMemory.PlatformAPI.ExecuteProcess = Win32ExecuteProcess;
    GlobalAppMemory.PlatformAPI.ExecuteProcessWithOutput = Win32ExecuteProcessWithOutput;
    GlobalAppMemory.PlatformAPI.GetHostname = Win32GetHostname;
    GlobalAppMemory.PlatformAPI.GetHomeDirectory = Win32GetHomeDirectory;
    GlobalAppMemory.PlatformAPI.GetAppConfigDirectory = Win32GetAppConfigDirectory;
    GlobalAppMemory.PlatformAPI.GetUsername = Win32GetUsername;
    GlobalAppMemory.PlatformAPI.GetProcessId = Win32GetProcessId;
    GlobalAppMemory.PlatformAPI.GetSystemTimestamp = Win32GetSystemTimestamp;
    GlobalAppMemory.PlatformAPI.GetDateTimeFromTimestamp = Win32GetDateTimeFromTimestamp;
    GlobalAppMemory.PlatformAPI.ConsoleOut = Win32ConsoleOut;
    
    
#if KENGINE_INTERNAL
    GlobalAppMemory.TelemetryState = GlobalTelemetryState;
    GlobalAppMemory.DebugEventTable = GlobalDebugEventTable;
#endif
    
    Platform = GlobalAppMemory.PlatformAPI;
    
#if KENGINE_INTERNAL
    {    
        char Filename[MAX_PATH];
        GetModuleFileNameA(0, Filename, MAX_PATH);
        u32 Length = GetNullTerminiatedStringLength(Filename);
        
        char *At = Filename;
        if(*At == '\"')
        {
            ++At;
            Length -= 2;
        }
        char *lastSlash = At + Length;
        while(*lastSlash != '\\')
        {
            --Length;
            --lastSlash;
        }
        Copy(Length, At, GlobalWin32State.ExeFilePath);
        GlobalWin32State.ExeFilePath[Length] = '\0';
        
        Copy(Length, GlobalWin32State.ExeFilePath, GlobalWin32State.DllFullFilePath);
        AppendCString(GlobalWin32State.DllFullFilePath + Length, "\\kengine.dll");
        
        Copy(Length, GlobalWin32State.ExeFilePath, GlobalWin32State.TempDllFullFilePath);
        AppendCString(GlobalWin32State.TempDllFullFilePath + Length, "\\kengine_temp.dll");
        
        Copy(Length, GlobalWin32State.ExeFilePath, GlobalWin32State.LockFullFilePath);
        AppendCString(GlobalWin32State.LockFullFilePath + Length, "\\lock.tmp");
    }
#endif
    
    GlobalAppMemory.ArgCount = 1;
    char *CommandLingArgs = GetCommandLineA();
    // NOTE(kstandbridge): Parse command line args
    {    
        Assert(CommandLingArgs);
        
        {
            char *At = CommandLingArgs;
            while(*At)
            {
                if(At[0] == ' ')
                {
                    ++GlobalAppMemory.ArgCount;
                }
                ++At;
            }
        }
        
        GlobalAppMemory.Args = PushArray(&GlobalWin32State.Arena, GlobalAppMemory.ArgCount, string);
        u32 CurrentArg = 0;
        {    
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
                    
                    if(*ParamStart == '\"')
                    {
                        ++ParamStart;
                        ParamLength -= 2;
                    }
                    
                    
                    string *Parameter = GlobalAppMemory.Args + CurrentArg++;
                    Parameter->Size = ParamLength;
                    Parameter->Data = (u8 *)ParamStart;
                    
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
        
        // NOTE(kstandbridge): Trim last argument if whitespace
        if(!GlobalAppMemory.Args[GlobalAppMemory.ArgCount - 1].Data)
        {
            --GlobalAppMemory.ArgCount;
        }
        
    }
    
#if KENGINE_HTTP
    win32_http_state *HttpState = PushStruct(&GlobalWin32State.Arena, win32_http_state);
    
#define OUTSTANDING_REQUESTS 16
#define REQUESTS_PER_PROCESSOR 4
    
    u16 RequestCount;
    
    HANDLE ProcessHandle = Win32GetCurrentProcess();
    DWORD_PTR ProcessAffintyMask;
    DWORD_PTR SystemAffinityMask;
    if(Win32GetProcessAffinityMask(ProcessHandle, &ProcessAffintyMask, &SystemAffinityMask))
    {
        for(RequestCount = 0;
            ProcessAffintyMask;
            ProcessAffintyMask >>= 1)
        {
            if(ProcessAffintyMask & 0x1)
            {
                ++RequestCount;
            }
        }
        
        RequestCount *= REQUESTS_PER_PROCESSOR;
    }
    else
    {
        LogWarning("Could not get process affinty mask");
        RequestCount = OUTSTANDING_REQUESTS;
    }
    
    LogVerbose("Set request count to %u", RequestCount);
    
    
    HttpState->ResponseCount = RequestCount;
    HttpState->NextResponseIndex = 0;
    HttpState->Responses = PushArray(&GlobalWin32State.Arena, RequestCount, win32_http_io_response);
    
    HttpState->RequestCount = RequestCount;
    HttpState->NextRequestIndex = 0;
    HttpState->Requests = PushArray(&GlobalWin32State.Arena, RequestCount, win32_http_io_request *);
    
    for(u32 RequestIndex = 0;
        RequestIndex < RequestCount;
        ++RequestIndex)
    {
        win32_http_io_request **Request = HttpState->Requests + RequestIndex;
        *Request = BootstrapPushStruct(win32_http_io_request, Arena);
    }
    
    u32 Result = Win32InitializeHttpServer(HttpState);
    if(Result == NO_ERROR)
    {
        if(Win32InitializeIoThreadPool(HttpState))
        {
            if(!Win32StartHttpServer(HttpState))
            {
                LogError("Failed to start server");
            }
        }
        else
        {
            Win32UninitializeIoCompletionContext(HttpState);
        }
    }
    else
    {
        LogError("Failed to initialize http server");
        Win32UnitializeHttpServer(HttpState);
    }
#endif
    
#if defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
    
    // NOTE(kstandbridge): 2 ticks per second
    f32 TargetSecondsPerFrame = 1.0f / 2.0f;
    
#else 
    
    
    b32 WindowClassIsRegistered = Win32RegisterClassExA(&WindowClass);
    Assert(WindowClassIsRegistered);
    
    GlobalWin32State.Window = Win32CreateWindowExA(0,
                                                   WindowClass.lpszClassName,
                                                   "kengine",
                                                   WS_OVERLAPPEDWINDOW,
                                                   CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                                   0, 0, Instance, 0);
    Assert(GlobalWin32State.Window);
    Win32ShowWindow(GlobalWin32State.Window, SW_SHOW);
    
    HDC OpenGLDC = Win32GetDC(GlobalWin32State.Window);
    
    u8 PushBuffer[65536];
    umm PushBufferSize = sizeof(PushBuffer);
    
    u32 CurrentSortMemorySize = Kilobytes(64);
    void *SortMemory = VirtualAlloc(0, CurrentSortMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    
    u32 CurrentClipMemorySize = Kilobytes(64);
    void *ClipMemory = VirtualAlloc(0, CurrentClipMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    
    
    app_input Input[2];
    ZeroArray(ArrayCount(Input), Input);
    app_input *NewInput = &Input[0];
    app_input *OldInput = &Input[1];
    
    RECT ClientRect;
    Win32GetClientRect(GlobalWin32State.Window, &ClientRect);
    
    v2i WindowDimensions = V2i((ClientRect.right - ClientRect.left),
                               (ClientRect.bottom - ClientRect.top));
    
    s32 MonitorRefreshHz = 60;
    s32 Win32RefreshRate = Win32GetDeviceCaps(OpenGLDC, VREFRESH);
    if(Win32RefreshRate > 1)
    {
        MonitorRefreshHz = Win32RefreshRate;
    }
    
    f32 TargetSecondsPerFrame = 1.0f / MonitorRefreshHz;
#endif // defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
    
    LARGE_INTEGER LastCounter = Win32GetWallClock();
    
    // NOTE(kstandbridge): Otherwise Sleep will be ignored for requests less than 50? citation needed
    UINT MinSleepPeriod = 1;
    Win32timeBeginPeriod(MinSleepPeriod);
    
    GlobalAppMemory.IsRunning = true;
    while(GlobalAppMemory.IsRunning)
    {
        
#if KENGINE_INTERNAL
        b32 DllNeedsToBeReloaded = false;
        FILETIME NewDLLWriteTime = Win32GetLastWriteTime(GlobalWin32State.DllFullFilePath);
        if(CompareFileTime(&NewDLLWriteTime, &GlobalWin32State.LastDLLWriteTime) != 0)
        {
            WIN32_FILE_ATTRIBUTE_DATA Ignored;
            if(!GetFileAttributesExA(GlobalWin32State.LockFullFilePath, GetFileExInfoStandard, &Ignored))
            {
                DllNeedsToBeReloaded = true;
            }
        }
#endif // KENGINE_INTERNAL
        
#if !defined(KENGINE_CONSOLE) && !defined(KENGINE_HEADLESS)
        
        BEGIN_BLOCK("ProcessPendingMessages");
        NewInput->MouseZ = 0;
        ZeroStruct(NewInput->FKeyPressed);
        Win32ProcessPendingMessages(NewInput);
        END_BLOCK();
        
        BEGIN_BLOCK("ProcessMouseInput");
        POINT MouseP;
        Win32GetCursorPos(&MouseP);
        Win32ScreenToClient(GlobalWin32State.Window, &MouseP);
        NewInput->MouseX = (f32)MouseP.x;
        NewInput->MouseY = (f32)((WindowDimensions.Y - 1) - MouseP.y);
        
        NewInput->ShiftDown = (Win32GetKeyState(VK_SHIFT) & (1 << 15));
        NewInput->AltDown = (Win32GetKeyState(VK_MENU) & (1 << 15));
        NewInput->ControlDown = (Win32GetKeyState(VK_CONTROL) & (1 << 15));
        
        // NOTE(kstandbridge): The order of these needs to match the order on enum app_input_mouse_button_type
        DWORD ButtonVKs[MouseButton_Count] =
        {
            VK_LBUTTON,
            VK_MBUTTON,
            VK_RBUTTON,
            VK_XBUTTON1,
            VK_XBUTTON2,
        };
        
        for(u32 ButtonIndex = 0;
            ButtonIndex < MouseButton_Count;
            ++ButtonIndex)
        {
            NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
            NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
            ProcessInputMessage(&NewInput->MouseButtons[ButtonIndex],
                                Win32GetKeyState(ButtonVKs[ButtonIndex]) & (1 << 15));
        }
        END_BLOCK();
        
        HDC DeviceContext = Win32GetDC(GlobalWin32State.Window);
        
        render_commands Commands_ = BeginRenderCommands(PushBufferSize, PushBuffer, WindowDimensions.X, WindowDimensions.Y);
        render_commands *Commands = &Commands_;
        
#endif // !defined(KENGINE_CONSOLE) && !defined(KENGINE_HEADLESS)
        
#if KENGINE_INTERNAL
        BEGIN_BLOCK("AppLoop");
        if(GlobalWin32State.AppTick_)
        {
#if defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
            GlobalWin32State.AppTick_(&GlobalAppMemory, TargetSecondsPerFrame);
#else
            GlobalWin32State.AppTick_(&GlobalAppMemory, Commands, NewInput, TargetSecondsPerFrame);
#endif
        }
        END_BLOCK();
        if(DllNeedsToBeReloaded)
        {
            SetDebugEventRecording(false);
        }
        
        BEGIN_BLOCK("DebugUpdateFrame");
        DebugUpdateFrame(&GlobalAppMemory);
        END_BLOCK();
        
        if(DllNeedsToBeReloaded)
        {
            if(GlobalWin32State.AppLibrary && !FreeLibrary(GlobalWin32State.AppLibrary))
            {
                Win32LogError("Failed to free app library");
            }
            GlobalWin32State.AppLibrary = 0;
            GlobalWin32State.AppTick_ = 0;
#if KENGINE_CONSOLE
            GlobalWin32State.AppHandleCommand = 0;
#endif
#if KENGINE_HTTP
            GlobalWin32State.AppHandleHttpRequest = 0;
#endif // KENGINE_HTTP
            
            if(CopyFileA(GlobalWin32State.DllFullFilePath, GlobalWin32State.TempDllFullFilePath, false))
            {
                LogDebug("App code reloaded!");
                GlobalWin32State.AppLibrary = LoadLibraryA(GlobalWin32State.TempDllFullFilePath);
                if(GlobalWin32State.AppLibrary)
                {
                    GlobalWin32State.AppTick_ = (app_tick_ *)GetProcAddress(GlobalWin32State.AppLibrary, "AppTick_");
                    Assert(GlobalWin32State.AppTick_);
#if KENGINE_CONSOLE
                    // NOTE(kstandbridge): Command handler is optional
                    GlobalWin32State.AppHandleCommand = (app_handle_command *)GetProcAddress(GlobalWin32State.AppLibrary, "AppHandleCommand");
#endif // KENGINE_CONSOLE
                    
#if KENGINE_HTTP
                    GlobalWin32State.AppHandleHttpRequest = (app_handle_http_request *)Win32GetProcAddressA(GlobalWin32State.AppLibrary, "AppHandleHttpRequest");
                    Assert(GlobalWin32State.AppHandleHttpRequest);
#endif // KENGINE_HTTP
                    
                    GlobalWin32State.LastDLLWriteTime = NewDLLWriteTime;
                }
                else
                {
                    Win32LogError("Failed to load app library");
                }
            }
            else
            {
                Win32LogError("Failed to copy %S to %S", GlobalWin32State.DllFullFilePath, GlobalWin32State.TempDllFullFilePath);
            }
            SetDebugEventRecording(true);
        }
        
#else // KENGINE_INTERNAL
        
#if defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
        AppTick_(&GlobalAppMemory, TargetSecondsPerFrame)
#else
        AppTick_(&GlobalAppMemory, Commands, Input, TargetSecondsPerFrame)
#endif
        
#endif // KENGINE_INTERNAL
        
#if !defined(KENGINE_CONSOLE) && !defined(KENGINE_HEADLESS)
        BEGIN_BLOCK("ExpandRenderStorage");
        u32 NeededSortMemorySize = Commands->PushBufferElementCount * sizeof(sort_entry);
        if(CurrentSortMemorySize < NeededSortMemorySize)
        {
            VirtualFree(SortMemory, 0, MEM_RELEASE);
            while(CurrentSortMemorySize < NeededSortMemorySize)
            {
                CurrentSortMemorySize *= 2;
            }
            SortMemory = VirtualAlloc(0, CurrentSortMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        }
        
        // TODO(kstandbridge): Can we merge sort/clip and push buffer memory together?
        u32 NeededClipMemorySize = Commands->PushBufferElementCount * sizeof(sort_entry);
        if(CurrentClipMemorySize < NeededClipMemorySize)
        {
            VirtualFree(ClipMemory, 0, MEM_RELEASE);
            while(CurrentClipMemorySize < NeededClipMemorySize)
            {
                CurrentClipMemorySize *= 2;
            }
            ClipMemory = VirtualAlloc(0, CurrentClipMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        }
        END_BLOCK();
        
        BEGIN_BLOCK("Render");
        SortRenderCommands(Commands, SortMemory);
        LinearizeClipRects(Commands, ClipMemory);
        
        Win32SwapBuffers(DeviceContext);
        
        EndRenderCommands(Commands);
        
        if(!Win32ReleaseDC(GlobalWin32State.Window, DeviceContext))
        {
            InvalidCodePath;
        }
        END_BLOCK();
        
        app_input *Temp = NewInput;
        NewInput = OldInput;
        OldInput = Temp;
#endif // !defined(KENGINE_CONSOLE) && !defined(KENGINE_HEADLESS)
        
        BEGIN_BLOCK("FrameWait");
        f32 FrameSeconds = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock(), GlobalWin32State.PerfCountFrequency);
        
        if(FrameSeconds < TargetSecondsPerFrame)
        {
            DWORD Miliseconds = (DWORD)(1000.0f * (TargetSecondsPerFrame - FrameSeconds));
            if(Miliseconds > 0)
            {
                Sleep(Miliseconds);
            }
            
            FrameSeconds = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock(), GlobalWin32State.PerfCountFrequency);
            
            // NOTE(kstandbridge): FINE I'll make my own sleep function, with blackjack and hookers!
            while(FrameSeconds < TargetSecondsPerFrame)
            {
                FrameSeconds = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock(), GlobalWin32State.PerfCountFrequency);
                _mm_pause();
            }
        }
        END_BLOCK();
        
        LARGE_INTEGER ThisCounter = Win32GetWallClock();
        f32 MeasuredSecondsPerFrame = Win32GetSecondsElapsed(LastCounter, ThisCounter, GlobalWin32State.PerfCountFrequency);
        DEBUG_FRAME_END(MeasuredSecondsPerFrame);
        LastCounter = ThisCounter;
    }
    
#if KENGINE_HTTP
    Win32StopHttpServer(HttpState);
    Win32UnitializeHttpServer(HttpState);
    Win32UninitializeIoCompletionContext(HttpState);
#endif
    
    LogInfo("Exiting process...");
    
    PostTelemetryThread(0);
    
    ExitProcess(0);
    
#if KENGINE_CONSOLE
    return 0;
#endif
}