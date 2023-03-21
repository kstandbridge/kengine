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

#ifdef KENGINE_DIRECTX
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")
#endif

#define COBJMACROS
#include <WS2tcpip.h>
#include <Windows.h>
#include <http.h>
#include <Shlobj.h>
#include <tlhelp32.h>
#include <Wininet.h>
#include <Wincrypt.h>

#ifdef KENGINE_DIRECTX

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#endif

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
#include "kengine_math.h"

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

#ifdef KENGINE_DIRECTX
#include "kengine_renderer.h"
#include "kengine_directx.h"
#endif

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
#include "kengine_vertex_shader_generated.h"
#include "kengine_glyph_pixel_shader_generated.h"
#include "kengine_sprite_pixel_shader_generated.h"
#include "kengine_rect_pixel_shader_generated.h"
#include "kengine_win32.c"
#endif //KENGINE_WIN32

#ifdef KENGINE_DIRECTX
#include "kengine_renderer.c"
#include "kengine_directx.c"
#endif // KENGINE_DIRECTX

string_list *
PlatformGetCommandLineArgs(memory_arena *Arena)
{
    string_list *Result = 0;
    
#if defined(KENGINE_CONSOLE) || defined(KENGINE_TEST)
    for(u32 ArgIndex = 0;
        ArgIndex < GlobalWin32State.ArgCount;
        ++ArgIndex)
    {
        char *Arg = GlobalWin32State.Args[ArgIndex];
        
        string Entry = 
        {
            .Size = GetNullTerminiatedStringLength(Arg),
            .Data = (u8 *)Arg
        };
        
        string_list *New = PushbackStringList(&Result, Arena);
        New->Entry = Entry;
    }
#else // defined(KENGINE_CONSOLE) || defined(KENGINE_TEST)
    LPSTR Args = GlobalWin32State.CmdLine;
    LPSTR At = Args;
    for(;;)
    {
        char C = *At;
        if(C == '"')
        {
            ++At;
            Args = At;
            while((*At != '\0') &&
                  (*At != '"'))
            {
                ++At;
            }
            
            string Entry = 
            {
                .Size = At - Args,
                .Data = (u8 *)Args
            };
            
            string_list *Arg = PushbackStringList(&Result, Arena);
            Arg->Entry = Entry;
            
            At += 2;
            Args = At;
        }
        else
        {
            if((C == ' ') ||
               (C == '\0'))
            {
                
                string Entry = 
                {
                    .Size = At - Args,
                    .Data = (u8 *)Args
                };
                
                string_list *Arg = PushbackStringList(&Result, Arena);
                Arg->Entry = Entry;
                
                ++At;
                Args = At;
                
                if(C == '\0')
                {
                    break;
                }
            }
            else
            {
                ++At;
            }
        }
        
    };
    
#endif // defined(KENGINE_CONSOLE) || defined(KENGINE_TEST)
    
    return Result;
}

#if defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW) || defined(KENGINE_DIRECTX)

#ifndef IDI_ICON
#define IDI_ICON       1000
#endif //IDI_ICON

void
InitApp(app_memory *AppMemory);

#if defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW)
LRESULT
MainWindowCallback(app_memory *AppMemory, HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);
#endif // defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW)

#if defined(KENGINE_DIRECTX)
void
AppUpdateFrame(app_memory *AppMemory, render_group *RenderGroup);
#endif // defined(KENGINE_DIRECTX)

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
            
#if defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW) || defined(KENGINE_DIRECTX)
            GlobalWin32State.Window = Window;
#endif
            
#if defined(KENGINE_DIRECTX)
            if(FAILED(DirectXRenderCreate()))
            {
                Win32LogError("Failed to create renderer");
                Result = -1;
            }
#endif
            
            InitApp(&GlobalAppMemory);
            
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
            
#if defined(KENGINE_DIRECTX)
            DirectXRenderDestroy();
#endif
            
            PostQuitMessage(0);
        } break;
        
#if defined(KENGINE_DIRECTX)
        case WM_SIZE:
        {
            if(FAILED(DirectXRenderResize(LOWORD(LParam), HIWORD(LParam))))
            {
                DestroyWindow(Window);
            }
            
        } break;
#endif
        
        default:
        {
#if defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW)
            if(GlobalAppMemory.AppState)
            {
                Result = MainWindowCallback(&GlobalAppMemory, Window, Message, WParam, LParam);
            }
            else
#endif // defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW)
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
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    {
        LARGE_INTEGER PerfCountFrequencyResult;
        QueryPerformanceFrequency(&PerfCountFrequencyResult);
        GlobalWin32State.PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    }
    GlobalWin32State.CmdLine = CmdLine;
    
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
                                      WS_OVERLAPPEDWINDOW, 
#else //KENGINE_HEADLESS
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
#endif //KENGINE_HEADLESS
                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, WindowClass.hInstance, 0);
        if(Window)
        {
            LogDebug("Begin application loop");
            
#if defined(KENGINE_WINDOW) || defined(KENGINE_DIRECTX)
            if(DarkApi.IsDarkModeSupported)
            {
                DarkApi.AllowDarkModeForWindow(Window, DarkApi.IsDarkModeEnabled);
                Win32RefreshTitleBarThemeColor(Window);
            }
#endif // defined(KENGINE_WINDOW) || defined(KENGINE_DIRECTX)
            
#if defined(KENGINE_WINDOW) || defined(KENGINE_HEADLESS)
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
#endif //defined(KENGINE_WINDOW) || defined(KENGINE_HEADLESS
            
#if defined(KENGINE_DIRECTX)
            
            
            render_command RenderCommands[10240];
            u32 MaxRenderCommands = ArrayCount(RenderCommands);
            
            LogDebug("Begin application loop");
            b32 Running = true;
            while(Running)
            {
                MSG Msg;
                while(PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE))
                {
                    if(Msg.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    TranslateMessage(&Msg);
                    DispatchMessageA(&Msg);
                }
                
                if(Running)
                {
                    render_group RenderGroup =
                    {
                        .Commands = RenderCommands,
                        .CurrentCommand = 0,
                        .MaxCommands = MaxRenderCommands,
                        .Width = GlobalWindowWidth,
                        .Height = GlobalWindowHeight,
#if 1
                        .ClearColor = { 0.1f, 0.2f, 0.6f, 1.0f },
#else
                        .ClearColor = { 1.0f, 1.0f, 1.0f, 1.0f },
#endif
                    };
                    
                    AppUpdateFrame(&GlobalAppMemory, &RenderGroup);
                    
                    DirectXRenderFrame(&RenderGroup);
                    
                    if(FAILED(DirectXRenderPresent()))
                    {
                        Running = false;
                    }
                }
            }
            LogDebug("End application loop");
            
#endif // defined(KENGINE_DIRECTX)
            
            
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

#endif //defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW) || defined(KENGINE_DIRECTX)

#ifdef KENGINE_CONSOLE

s32
MainLoop(app_memory *AppMemory);

s32
main(u32 ArgCount, char **Args)
{
    s32 Result = 0;
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    {
        LARGE_INTEGER PerfCountFrequencyResult;
        QueryPerformanceFrequency(&PerfCountFrequencyResult);
        GlobalWin32State.PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    }
    
    GlobalWin32State.ArgCount = ArgCount - 1;
    GlobalWin32State.Args = Args + 1;
    
    Result = MainLoop(&GlobalAppMemory);
    
    return Result;
}
#endif //KENGINE_CONSOLE


#ifdef KENGINE_TEST

typedef struct test_state
{
    memory_arena Arena;
    
    s32 TotalTests;
    s32 FailedTests;
    
} test_state;

global test_state *GlobalTestState_;

#define AssertTrue(Expression) \
++GlobalTestState_->TotalTests; \
if(!(Expression)) \
{ \
++GlobalTestState_->FailedTests; \
PlatformConsoleOut("%s(%d): expression assert fail.\n", __FILE__, __LINE__); \
}

#define AssertEqualString(Expected, Actual) \
++GlobalTestState_->TotalTests; \
if(!StringsAreEqual(Expected, Actual)) \
{ \
++GlobalTestState_->FailedTests; \
PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%S'\n\t\t\tActual:      '%S'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

#define AssertEqualU32(Expected, Actual) \
++GlobalTestState_->TotalTests; \
if(Expected != Actual) \
{ \
++GlobalTestState_->FailedTests; \
PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%u'\n\t\t\tActual:      '%u'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

#define AssertEqualS32(Expected, Actual) \
++GlobalTestState_->TotalTests; \
if(Expected != Actual) \
{ \
++GlobalTestState_->FailedTests; \
PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%d'\n\t\t\tActual:      '%d'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

#define AssertEqualU64(Expected, Actual) \
++TotalTests; \
if(Expected != Actual) \
{ \
++GlobalTestState_->FailedTests; \
PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%lu'\n\t\t\tActual:      '%lu'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

void
RunAllTests(memory_arena *Arena);

s32
main(u32 ArgCount, char **Args)
{
    s32 Result = 0;
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    {
        LARGE_INTEGER PerfCountFrequencyResult;
        QueryPerformanceFrequency(&PerfCountFrequencyResult);
        GlobalWin32State.PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    }
    
    GlobalWin32State.ArgCount = ArgCount - 1;
    GlobalWin32State.Args = Args + 1;
    
    GlobalTestState_ = BootstrapPushStruct(test_state, Arena);
    
    RunAllTests(&GlobalTestState_->Arena);
    
    Result = (GlobalTestState_->FailedTests == 0);
    
    PlatformConsoleOut("Unit Tests %s: %d/%d passed.\n", 
                       Result ? "Successful" : "Failed"
                       , GlobalTestState_->TotalTests - GlobalTestState_->FailedTests, 
                       GlobalTestState_->TotalTests);
    
    return Result;
}
#endif //KENGINE_CONSOLE


#endif //KENGINE_IMPLEMENTATION

#define KENGINE_H
#endif //KENGINE_H
