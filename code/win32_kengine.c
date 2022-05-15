#include "kengine_platform.h"
#include "kengine_shared.h"
#include "kengine_generated.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "win32_kengine_shared.h"

internal s32
Win32DisplayLastError()
{
    __debugbreak();
    
    DWORD Result = GetLastError();
    char Buffer[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, Result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                   Buffer, (sizeof(Buffer) / sizeof(char)), 0);
    MessageBoxEx(0, Buffer, "kEngine win32 Error", MB_OK|MB_ICONERROR, 0);
    
    return Result;
}

typedef struct win32_offscreen_buffer
{
    // NOTE(kstandbridge): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    
    s32 Width;
    s32 Height;
    s32 Pitch;
} win32_offscreen_buffer;

typedef struct app_code
{
    HMODULE Library;
    FILETIME LastDLLWriteTime;
    
    app_update_and_render *AppUpdateAndRender;
} app_code;


global b32 GlobalIsRunning;
global s64 GlobalPerfCountFrequency;
global win32_offscreen_buffer GlobalBackbuffer;

internal loaded_bitmap
DEBUGGetGlyphForCodepoint(memory_arena *Arena, u32 CodePoint)
{
    
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024
    
    local_persist b32 FontInitialized;
    local_persist VOID *FontBits;
    local_persist HDC FontDeviceContext;
    local_persist HFONT FontHandle;
    local_persist TEXTMETRIC TextMetric;
    
    if(!FontInitialized)
    {
        FontDeviceContext = CreateCompatibleDC(GetDC(0));
        
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
        HBITMAP Bitmap = CreateDIBSection(FontDeviceContext, &Info, DIB_RGB_COLORS, &FontBits, 0, 0);
        SelectObject(FontDeviceContext, Bitmap);
        SetBkColor(FontDeviceContext, RGB(0, 0, 0));
        
        AddFontResourceExA("c:/Windows/Fonts/arial.ttf", FR_PRIVATE, 0);
        s32 Height = 128; // TODO(kstandbridge): Figure out how to specify pixels properly here
        FontHandle = CreateFontA(Height, 0, 0, 0,
                                 FW_NORMAL, // NOTE(kstandbridge): Weight
                                 FALSE, // NOTE(kstandbridge): Italic
                                 FALSE, // NOTE(kstandbridge): Underline
                                 FALSE, // NOTE(kstandbridge): Strikeout
                                 DEFAULT_CHARSET, 
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, 
                                 ANTIALIASED_QUALITY,
                                 DEFAULT_PITCH|FF_DONTCARE,
                                 "Arial");
        
        SelectObject(FontDeviceContext, FontHandle);
        GetTextMetrics(FontDeviceContext, &TextMetric);
        
        FontInitialized = true;
    }
    
    loaded_bitmap Result;
    ZeroStruct(Result);
    
    SelectObject(FontDeviceContext, FontHandle);
    
    memset(FontBits, 0x00, MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32));
    
    wchar_t CheesePoint = (wchar_t)CodePoint;
    
    SIZE Size;
    GetTextExtentPoint32W(FontDeviceContext, &CheesePoint, 1, &Size);
    
    int PreStepX = 128;
    
    int BoundWidth = Size.cx + 2*PreStepX;
    if(BoundWidth > MAX_FONT_WIDTH)
    {
        BoundWidth = MAX_FONT_WIDTH;
    }
    int BoundHeight = Size.cy;
    if(BoundHeight > MAX_FONT_HEIGHT)
    {
        BoundHeight = MAX_FONT_HEIGHT;
    }
    
    SetTextColor(FontDeviceContext, RGB(255, 255, 255));
    TextOutW(FontDeviceContext, PreStepX, 0, &CheesePoint, 1);
    
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
        int Width = (MaxX - MinX) + 1;
        int Height = (MaxY - MinY) + 1;
        
        Result.Width = Width + 2;
        Result.Height = Height + 2;
        Result.WidthOverHeight = SafeRatio1((f32)Result.Width, (f32)Result.Height);
        Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
        Result.Memory = PushSize(Arena, Result.Height*Result.Pitch);
        
        memset(Result.Memory, 0, Result.Height*Result.Pitch);
        
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
            
            DestRow -= Result.Pitch;
            SourceRow -= MAX_FONT_WIDTH;
        }
        
        Result.AlignPercentage.X = (1.0f) / (f32)Result.Width;
        Result.AlignPercentage.Y = (1.0f + (MaxY - (BoundHeight - TextMetric.tmDescent))) / (f32)Result.Height;
        
        KerningChange = (f32)(MinX - PreStepX);
        
    }
    
    INT ThisWidth;
    GetCharWidth32W(FontDeviceContext, CodePoint, CodePoint, &ThisWidth);
    f32 CharAdvance = (f32)ThisWidth;
    
#if 0    
    for(u32 OtherGlyphIndex = 0;
        OtherGlyphIndex < Font->MaxGlyphCount;
        ++OtherGlyphIndex)
    {
        Font->HorizontalAdvance[GlyphIndex*Font->MaxGlyphCount + OtherGlyphIndex] += CharAdvance - KerningChange;
        if(OtherGlyphIndex != 0)
        {
            Font->HorizontalAdvance[OtherGlyphIndex*Font->MaxGlyphCount + GlyphIndex] += KerningChange;
        }
    }
#endif
    
    
    return Result;
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{       
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CLOSE:
        {
            GlobalIsRunning = false;
        } break;
        
        
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            GlobalBackbuffer.Width = ClientRect.right - ClientRect.left;
            GlobalBackbuffer.Height = ClientRect.bottom - ClientRect.top;
            
            if(GlobalBackbuffer.Memory)
            {
                VirtualFree(GlobalBackbuffer.Memory, 0, MEM_RELEASE);
            }
            
            GlobalBackbuffer.Info.bmiHeader.biSize = sizeof(GlobalBackbuffer.Info.bmiHeader);
            GlobalBackbuffer.Info.bmiHeader.biWidth = GlobalBackbuffer.Width;
            GlobalBackbuffer.Info.bmiHeader.biHeight = GlobalBackbuffer.Height;
            GlobalBackbuffer.Info.bmiHeader.biPlanes = 1;
            GlobalBackbuffer.Info.bmiHeader.biBitCount = 32;
            GlobalBackbuffer.Info.bmiHeader.biCompression = BI_RGB;
            
            // TODO(kstandbridge): Need to figure out a way we can align this
            //GlobalBackbuffer.Pitch = Align16(GlobalBackbuffer.Width*BITMAP_BYTES_PER_PIXEL);
            GlobalBackbuffer.Pitch = GlobalBackbuffer.Width*BITMAP_BYTES_PER_PIXEL;
            
            GlobalBackbuffer.Memory = VirtualAlloc(0, sizeof(u32)*GlobalBackbuffer.Pitch*GlobalBackbuffer.Height,
                                                   MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            
        } break;
        
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
    
}

internal void
Win32ProcessPendingMessages()
{
    MSG Msg;
    ZeroStruct(Msg);
    while (PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE))
    {
        switch(Msg.message)
        {
            case WM_QUIT:
            {
                // NOTE(kstandbridge): This actually never gets hit, but we need at least 1 case otherwise compiler error
                __debugbreak();
            } break;
            
            default: 
            {
                TranslateMessage(&Msg);
                DispatchMessageA(&Msg);
            } break;
        }
    }
}

internal LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

internal f32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    f32 Result = ((f32)(End.QuadPart - Start.QuadPart) /
                  (f32)GlobalPerfCountFrequency);
    return Result;
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
    
    return(LastWriteTime);
}

internal void
Win32UnloadAppCode(app_code *AppCode)
{
    if(AppCode->Library)
    {
        FreeLibrary(AppCode->Library);
        AppCode->Library = 0;
    }
    
    AppCode->AppUpdateAndRender = 0;
}

internal app_code
Win32LoadAppCode(char *SourceDLLName, char *TempDLLName, char *LockName)
{
    app_code Result;
    ZeroStruct(Result);
    
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if(!GetFileAttributesExA(LockName, GetFileExInfoStandard, &Ignored))
    {
        Result.LastDLLWriteTime = Win32GetLastWriteTime(SourceDLLName);
        
        CopyFile(SourceDLLName, TempDLLName, FALSE);
        
        Result.Library = LoadLibraryA(TempDLLName);
        if(Result.Library)
        {
            Result.AppUpdateAndRender = (app_update_and_render *)GetProcAddress(Result.Library, "AppUpdateAndRender");
        }
    }
    
    return Result;
}

s32 __stdcall
WinMainCRTStartup()
{
    HINSTANCE Instance = GetModuleHandle(0);
    s32 ExitCode = EXIT_SUCCESS;
    
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    LARGE_INTEGER LastCounter = Win32GetWallClock();
    
#define MonitorRefreshHz 60
#define AppUpdateHz (MonitorRefreshHz / 2)
    f32 TargetSecondsPerFrame = 1.0f / (f32)AppUpdateHz;
    
    UINT DesiredSchedulerMS = 1;
    b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
    
    WNDCLASSEXA WindowClass = {sizeof(WNDCLASSEXA)};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "KengineWindowClass";
    
    b32 Result = RegisterClassExA(&WindowClass);
    if(Result)
    {
        HWND WindowHwnd = CreateWindowExA(0,
                                          WindowClass.lpszClassName,
                                          "kEngine",
                                          WS_OVERLAPPEDWINDOW,
                                          CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                          0, 0, Instance, 0);
        if(WindowHwnd)
        {
            ShowWindow(WindowHwnd, SW_SHOW);
            
            platform_work_queue PerFrameWorkQueue;
            MakeQueue(&PerFrameWorkQueue, 6);
            platform_work_queue BackgroundWorkQueue;
            MakeQueue(&BackgroundWorkQueue, 2);
            
            
            char *SourceAppCodeDLLFullPath = "D:/build/kengine.dll";
            char *TempAppCodeDLLFullPath = "D:/build/kengine_temp.dll";
            char *AppCodeLockFullPath = "D:/build/lock.tmp";
            
            app_code AppCode = Win32LoadAppCode(SourceAppCodeDLLFullPath, TempAppCodeDLLFullPath, AppCodeLockFullPath);
            
#if KENGINE_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif
            app_memory AppMemory;
            ZeroStruct(AppMemory);
            AppMemory.StorageSize = Megabytes(256);
            AppMemory.Storage = VirtualAlloc(BaseAddress, (size_t)AppMemory.StorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            AppMemory.PlatformAPI.PerFrameWorkQueue = &PerFrameWorkQueue;
            AppMemory.PlatformAPI.BackgroundWorkQueue = &BackgroundWorkQueue;
            AppMemory.PlatformAPI.AddWorkEntry = AddWorkEntry;
            AppMemory.PlatformAPI.CompleteAllWork = CompleteAllWork;
            AppMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGReadEntireFile;
            AppMemory.PlatformAPI.DEBUGGetGlyphForCodepoint = DEBUGGetGlyphForCodepoint;
            
            GlobalIsRunning = true;
            while(GlobalIsRunning)
            {
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceAppCodeDLLFullPath);
                if(CompareFileTime(&NewDLLWriteTime, &AppCode.LastDLLWriteTime) != 0)
                {
                    Win32UnloadAppCode(&AppCode);
                    AppCode = Win32LoadAppCode(SourceAppCodeDLLFullPath, TempAppCodeDLLFullPath, AppCodeLockFullPath);
                }
                
                Win32ProcessPendingMessages();
                
                LARGE_INTEGER WorkCounter = Win32GetWallClock();
                f32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                
                f32 SecondsElapsedForFrame = WorkSecondsElapsed;
                if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                {
                    if (SleepIsGranular)
                    {
                        DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                           SecondsElapsedForFrame));
                        if (SleepMS > 0)
                        {
                            Sleep(SleepMS);
                        }
                    }
                    
                    while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        
                        _mm_pause();
                    }
                }
                
                LARGE_INTEGER EndCounter = Win32GetWallClock();
                LastCounter = EndCounter;
                
                // NOTE(kstandbridge): Render
                
                app_offscreen_buffer AppBuffer;
                ZeroStruct(AppBuffer);
                AppBuffer.Memory = GlobalBackbuffer.Memory;
                AppBuffer.Width = GlobalBackbuffer.Width;
                AppBuffer.Height = GlobalBackbuffer.Height;
                AppBuffer.Pitch = GlobalBackbuffer.Pitch;
                
                if(AppCode.AppUpdateAndRender)
                {
                    AppCode.AppUpdateAndRender(&AppMemory, &AppBuffer, TargetSecondsPerFrame);
                }
                
                HDC DeviceContext = GetDC(WindowHwnd);
                StretchDIBits(DeviceContext, 0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height, 0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height, GlobalBackbuffer.Memory, &GlobalBackbuffer.Info, DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(WindowHwnd, DeviceContext);
                
            }
            
        }
        else
        {
            // TODO(kstandbridge): Log error creating window
            ExitCode = Win32DisplayLastError();
        }
    }
    else
    {
        // TODO(kstandbridge): Log error registering window class
        ExitCode = Win32DisplayLastError();
    }
    
    ExitProcess(ExitCode);
}