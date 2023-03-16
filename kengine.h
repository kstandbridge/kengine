#ifndef KENGINE_H
#define KENGINE_H

#include <stdarg.h>
#include <intrin.h>

#ifndef VERSION
#define VERSION 0
#endif // VERSION

#ifdef KENGINE_WIN32
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "HttpApi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Wininet.lib")

#include <WS2tcpip.h>
#include <Windows.h>
#include <http.h>
#include <Shlobj.h>
#include <tlhelp32.h>
#include <Wininet.h>
#include <Wincrypt.h>
#endif

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

typedef struct app_memory
{
    struct platform_state *PlatformState;
    struct app_state *AppState;
} app_memory;

extern app_memory GlobalAppMemory;

#ifdef KENGINE_WIN32
#include "kengine_win32.h"
#endif

inline date_time
GetDateTime()
{
    u64 Timestamp = PlatformGetSystemTimestamp();
    date_time Result = PlatformGetDateTimeFromTimestamp(Timestamp);
    
    return Result;
}

typedef struct platform_state
{
    memory_arena Arena;
} platform_state;


#ifdef KENGINE_IMPLEMENTATION
#include "kengine_memory.c"
#include "kengine_log.c"
#include "kengine_string.c"
#include "kengine_sha512.c"
#include "kengine_eddsa.c"
#include "kengine_json_parser.c"

#ifdef KENGINE_WIN32
#include "kengine_win32.c"
#endif

#ifdef KENGINE_HEADLESS

#ifndef IDI_ICON
#define IDI_ICON       1000
#endif

void
InitApp(app_memory *AppMemory, HWND Window);

LRESULT
HandleMessage(app_memory *AppMemory, u32 Message, WPARAM WParam, LPARAM LParam);

internal LRESULT CALLBACK
Win32WindowProc(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CREATE:
        {
            InitApp(&GlobalAppMemory, Window);
        } break;
        
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;
        
        default:
        {
            if(!HandleMessage(&GlobalAppMemory, Message, WParam, LParam))
            {
                Result = DefWindowProcW(Window, Message, WParam, LParam);
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
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    {
        LARGE_INTEGER PerfCountFrequencyResult;
        QueryPerformanceFrequency(&PerfCountFrequencyResult);
        GlobalWin32State.PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    }
    
    WNDCLASSEXW WindowClass =
    {
        .cbSize = sizeof(WindowClass),
        .lpfnWndProc = Win32WindowProc,
        .hInstance = Instance,
        .hIcon = LoadIconA(Instance, MAKEINTRESOURCE(IDI_ICON)),
        .hCursor = LoadCursorA(NULL, IDC_ARROW),
        .lpszClassName = L"KengineWindowClass",
    };
    LogDebug("Registering %ls window class", WindowClass.lpszClassName);
    if(RegisterClassExW(&WindowClass))
    {
        HWND Window = CreateWindowExW(WS_EX_APPWINDOW, WindowClass.lpszClassName, L"kengine",
                                      WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, WindowClass.hInstance, 0);
        if(Window)
        {
            LogDebug("Begin application loop");
        }
        else
        {
            Win32LogError("Failed to create window");
        }
        
        UpdateWindow(Window);
        
        b32 HasMessage = false;
        MSG Msg;
        while((HasMessage = GetMessageW(&Msg, 0, 0, 0)) != 0)
        {
            if(HasMessage == -1)
            {
                Win32LogError("Failed to get message");
                break;
            }
            else
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
        
        LogDebug("Unregistering %ls window class", WindowClass.lpszClassName);
        UnregisterClassW(WindowClass.lpszClassName, WindowClass.hInstance);
    }
    else
    {
        Win32LogError("Failed to register window class");
    }
    
    return Result;
}

#endif

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
#endif


#endif

#endif //KENGINE_H
