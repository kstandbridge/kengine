#ifndef KENGINE_H

#pragma warning( disable : 4115 4116 4127 4201 )

#include <stdarg.h>
#include <intrin.h>

#ifndef VERSION
#define VERSION 0
#endif //VERSION

#ifdef KENGINE_WIN32
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "HttpApi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Wininet.lib")

#include <WS2tcpip.h>
#include <Windows.h>
#include <http.h>
#include <Shlobj.h>
#include <tlhelp32.h>
#include <Wininet.h>
#include <Wincrypt.h>

#endif //KENGINE_WIN32

#define introspect(...)
#include "kengine_types.h"
#include "kengine_memory.h"
#include "kengine_string.h"
#include "kengine_json_parser.h"
#include "kengine_log.h"
#include "kengine_http.h"
#include "kengine_sha512.h"
#include "kengine_eddsa.h"
#include "kengine_random.h"
#include "kengine_tokenizer.h"
#include "kengine_xml_parser.h"

typedef void platform_work_queue_callback(void *Data);

typedef struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
} platform_work_queue_entry;

typedef struct platform_work_queue 
{
    platform_work_queue_entry Entries[256];
} platform_work_queue;

typedef struct platform_state
{
    memory_arena Arena;
} platform_state;

typedef struct app_memory
{
    platform_state *PlatformState;
    struct app_state *AppState;
} app_memory;

extern app_memory GlobalAppMemory;

#ifdef KENGINE_WIN32
#include "kengine_win32.h"
#endif //KENGINE_WIN32

inline date_time
GetDateTime()
{
    u64 Timestamp = PlatformGetSystemTimestamp();
    date_time Result = PlatformGetDateTimeFromTimestamp(Timestamp);
    
    return Result;
}

#ifdef KENGINE_IMPLEMENTATION
#include "kengine_memory.c"
#include "kengine_log.c"
#include "kengine_string.c"
#include "kengine_sha512.c"
#include "kengine_eddsa.c"
#include "kengine_tokenizer.c"
#include "kengine_json_parser.c"

#ifdef KENGINE_WIN32
#include "kengine_win32.c"
#endif //KENGINE_WIN32

#if defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW)

#ifndef IDI_ICON
#define IDI_ICON       1000
#endif //IDI_ICON

void
InitApp(app_memory *AppMemory, HWND Window, char *CommandLine);

LRESULT
MainWindowCallback(app_memory *AppMemory, HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

char *GlobalCommandLine;

internal LRESULT CALLBACK
Win32WindowProc(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    COLORREF LightBkColor = 0xFFFFFF;
    COLORREF DarkBkColor = 0x383838;
    COLORREF DarkTextColor = 0xFFFFFF;
    local_persist HBRUSH DarkBkBrush = 0;
    local_persist HBRUSH LightBkBrush = 0;
    
    switch(Message)
    {
        case WM_CREATE:
        {
            if(DarkApi.IsDarkModeSupported)
            {
                DarkApi.SetWindowTheme(Window, L"DarkMode_Explorer", 0);
                DarkApi.AllowDarkModeForWindow(Window, DarkApi.IsDarkModeEnabled);
                Win32RefreshTitleBarThemeColor(Window);
            }
            
            InitApp(&GlobalAppMemory, Window, GlobalCommandLine);
            
        } break;
        
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        {
            if (DarkApi.IsDarkModeSupported && DarkApi.IsDarkModeEnabled)
            {
                HDC Hdc = (HDC)WParam;
                SetTextColor(Hdc, DarkTextColor);
                SetBkColor(Hdc, DarkBkColor);
                if (!DarkBkBrush)
                {
                    DarkBkBrush = CreateSolidBrush(DarkBkColor);
                }
                Result = (LRESULT)DarkBkBrush;
            }
        } break;
        
        case WM_ERASEBKGND:
        {
            RECT Rect;
            GetClientRect(Window, &Rect);
            if (DarkApi.IsDarkModeSupported && DarkApi.IsDarkModeEnabled)
            {
                if (!DarkBkBrush)
                {
                    DarkBkBrush = CreateSolidBrush(DarkBkColor);
                }
                FillRect((HDC)WParam, &Rect, DarkBkBrush);
            }
            else
            {
                if (!LightBkBrush)
                {
                    LightBkBrush = CreateSolidBrush(LightBkColor);
                }
                FillRect((HDC)WParam, &Rect, LightBkBrush);
            }
            Result = 0;
            
        } break;
        
        case WM_SETTINGCHANGE:
        {
            // TODO(kstandbridge): Win32IsColorSchemeChangeMessage
#if 0            
            if(Win32IsColorSchemeChangeMessage(LParam))
            {
                Win32.IsDarkModeEnabled = Win32.ShouldAppsUseDarkMode() && !IsHighContrast();
                RefreshTitleBarThemeColor(Window);
                
                // NOTE(kstandbridge): Pass through to dll layer
                if(GlobalGalaQCode.MainWindowCallback)
                {
                    Result = GlobalGalaQCode.MainWindowCallback(&GlobalAppMemory, Window, Message, WParam, LParam);
                }
                
                RECT Rect;
                Win32.GetClientRect(Window, &Rect);
                Win32.InvalidateRect(Window, &Rect, true);
            }
#endif //0
            
        } break;
        
        case WM_DESTROY:
        {
            if (DarkBkBrush)
            {
                DeleteObject(DarkBkBrush);
                DarkBkBrush = 0;
            }
            
            PostQuitMessage(0);
        } break;
        
        default:
        {
            if(GlobalAppMemory.AppState)
            {
                Result = MainWindowCallback(&GlobalAppMemory, Window, Message, WParam, LParam);
            }
            else
            {
                Result = DefWindowProcA(Window, Message, WParam, LParam);
            }
        } break;
    }
    
    return Result;
}

s32 WINAPI
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, s32 CmdShow)
{
    UNREFERENCED_PARAMETER(PrevInstance);
    UNREFERENCED_PARAMETER(CmdLine);
    UNREFERENCED_PARAMETER(CmdShow);
    
    s32 Result = 0;
    
    CoInitialize(0);
    
    GlobalCommandLine = CmdLine;
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    {
        LARGE_INTEGER PerfCountFrequencyResult;
        QueryPerformanceFrequency(&PerfCountFrequencyResult);
        GlobalWin32State.PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    }
    
    InitDarkApi();
    
    WNDCLASSEX WindowClass =
    {
        .cbSize = sizeof(WindowClass),
        .lpfnWndProc = Win32WindowProc,
        .hInstance = Instance,
        .hIcon = LoadIconA(Instance, MAKEINTRESOURCE(IDI_ICON)),
        .hCursor = LoadCursorA(NULL, IDC_ARROW),
        .lpszClassName = "KengineWindowClass",
    };
    LogDebug("Registering %ls window class", WindowClass.lpszClassName);
    if(RegisterClassExA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "kengine",
#ifdef KENGINE_HEADLESS
                                      WS_DISABLED, 
#else //KENGINE_HEADLESS
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
#endif //KENGINE_HEADLESS
                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, WindowClass.hInstance, 0);
        if(Window)
        {
            LogDebug("Begin application loop");
            
#ifdef KENGINE_WINDOW
            if(DarkApi.IsDarkModeSupported)
            {
                DarkApi.AllowDarkModeForWindow(Window, DarkApi.IsDarkModeEnabled);
                Win32RefreshTitleBarThemeColor(Window);
            }
#endif //KENGINE_WINDOW
            b32 HasMessage = false;
            MSG Msg;
            while((HasMessage = GetMessageA(&Msg, 0, 0, 0)) != 0)
            {
                if(HasMessage == -1)
                {
                    Win32LogError("Failed to get message");
                    break;
                }
                else
                {
                    TranslateMessage(&Msg);
                    DispatchMessageA(&Msg);
                }
            }
        }
        else
        {
            Win32LogError("Failed to create window");
        }
        
        LogDebug("Unregistering %ls window class", WindowClass.lpszClassName);
        UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
    }
    else
    {
        Win32LogError("Failed to register window class");
    }
    
    CoUninitialize();
    
    return Result;
}

#endif //defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW)

#ifdef KENGINE_CONSOLE

s32
MainLoop(app_memory *AppMemory, s32 ArgCount, string *Arguments);

s32
main(s32 ArgCount, char **Args)
{
    s32 Result = 0;
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    {
        LARGE_INTEGER PerfCountFrequencyResult;
        QueryPerformanceFrequency(&PerfCountFrequencyResult);
        GlobalWin32State.PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    }
    
    platform_state *PlatformState = GlobalAppMemory.PlatformState;
    if(PlatformState == 0)
    {
        PlatformState = GlobalAppMemory.PlatformState = BootstrapPushStruct(platform_state, Arena);
    }
    
    string *Arguments = PushArray(&PlatformState->Arena, ArgCount, string);
    for(s32 ArgIndex = 0;
        ArgIndex < ArgCount;
        ++ArgIndex)
    {
        Arguments[ArgIndex] = String_(GetNullTerminiatedStringLength(Args[ArgIndex]), (u8 *)Args[ArgIndex]);
    }
    
    Result = MainLoop(&GlobalAppMemory, ArgCount, Arguments);
    
    return Result;
}
#endif //KENGINE_CONSOLE

#endif //KENGINE_IMPLEMENTATION

#define KENGINE_H
#endif //KENGINE_H
