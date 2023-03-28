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
#include "kengine/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "kengine/stb_truetype.h"

#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#endif

#endif //KENGINE_WIN32

#define introspect(...)
#include "kengine/kengine_types.h"
#include "kengine/kengine_memory.h"
#include "kengine/kengine_generated.h"
#include "kengine/kengine_math.h"
#include "kengine/kengine_string.h"
#include "kengine/kengine_json_parser.h"
#include "kengine/kengine_log.h"
#include "kengine/kengine_http.h"
#include "kengine/kengine_sha512.h"
#include "kengine/kengine_eddsa.h"
#include "kengine/kengine_random.h"
#include "kengine/kengine_tokenizer.h"
#include "kengine/kengine_xml_parser.h"
#include "kengine/kengine_c_parser.h"


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
    struct app_state *AppState;
    
#if defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW) || defined(KENGINE_DIRECTX)
    char *CmdLine;
    HWND Window;
#endif // defined(KENGINE_HEADLESS) || defined(KENGINE_WINDOW) || defined(KENGINE_DIRECTX)
} app_memory;

extern app_memory GlobalAppMemory;

#ifdef KENGINE_DIRECTX
#include "kengine/kengine_renderer.h"
#include "kengine/kengine_directx.h"
#endif

#ifdef KENGINE_WIN32
#include "kengine/kengine_win32.h"
#endif //KENGINE_WIN32

inline date_time
GetDateTime()
{
    u64 Timestamp = PlatformGetSystemTimestamp();
    date_time Result = PlatformGetDateTimeFromTimestamp(Timestamp);
    
    return Result;
}

#ifdef KENGINE_IMPLEMENTATION
#include "kengine/kengine_memory.c"
#include "kengine/kengine_log.c"
#include "kengine/kengine_string.c"
#include "kengine/kengine_sha512.c"
#include "kengine/kengine_eddsa.c"
#include "kengine/kengine_tokenizer.c"
#include "kengine/kengine_json_parser.c"
#include "kengine/kengine_c_parser.c"

#ifdef KENGINE_WIN32
#include "kengine/kengine_vertex_shader_generated.h"
#include "kengine/kengine_glyph_pixel_shader_generated.h"
#include "kengine/kengine_sprite_pixel_shader_generated.h"
#include "kengine/kengine_rect_pixel_shader_generated.h"
#include "kengine/kengine_win32.c"
#endif //KENGINE_WIN32

#ifdef KENGINE_DIRECTX
#include "kengine/kengine_renderer.c"
#include "kengine/kengine_directx.c"
#endif // KENGINE_DIRECTX

#if defined(KENGINE_CONSOLE) || defined(KENGINE_WINDOW)

string_list *
PlatformGetCommandLineArgs(memory_arena *Arena)
{
    string_list *Result = 0;
    
#if defined(KENGINE_CONSOLE)
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
#else
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
#endif // defined(KENGINE_CONSOLE)
    return Result;
}

#endif // defined(KENGINE_CONSOLE) || defined(KENGINE_WINDOW)

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

// TODO(kstandbridge): Relocate this

typedef struct app_button_state
{
    s32 HalfTransitionCount;
    b32 EndedDown;
} app_button_state;

typedef enum mouse_button_type
{
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Extended0,
    MouseButton_Extended1,
    
    MouseButton_Count,
} mouse_button_type;

typedef struct app_input
{
    app_button_state MouseButtons[MouseButton_Count];
    v2 MouseP;
    f32 MouseWheel;
    
    b32 ShiftDown;
    b32 AltDown;
    b32 ControlDown;
    b32 FKeyPressed[13];
} app_input;

inline b32 
WasPressed(app_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
    
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


void
AppUpdateFrame(app_memory *AppMemory, render_group *RenderGroup, app_input *Input, f32 DeltaTime);
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
            GlobalAppMemory.Window = Window;
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
    LogDebug("Registering %s window class", WindowClass.lpszClassName);
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
            
            LARGE_INTEGER LastCounter = Win32GetWallClock();
            
            app_input Input[2] = {0};
            app_input *NewInput = Input;
            app_input *OldInput = Input + 1;
            
            b32 Running = true;
            
            // TODO(kstandbridge): Platform arena?
            memory_arena Arena = {0};
            render_group *RenderGroup = PushStruct(&Arena, render_group);
            
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
                    RenderGroup->CurrentCommand = 0;
                    RenderGroup->Width = GlobalWindowWidth;
                    RenderGroup->Height = GlobalWindowHeight;
                    RenderGroup->VertexBufferAt = 0;
                    
                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    f32 DeltaTime = Win32GetSecondsElapsed(LastCounter, EndCounter);
                    
                    POINT Point;
                    if(!GetCursorPos(&Point))
                    {
                        Win32LogError("Failed to get cursor position");
                    }
                    else
                    {
                        if(!ScreenToClient(Window, &Point))
                        {
                            Win32LogError("Failed to convert cursor position to client coordinates");
                        }
                        else
                        {
                            NewInput->MouseP = V2(Point.x, Point.y);
                        }
                    }
                    
                    DWORD Win32ButtonIds[MouseButton_Count] =
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
                                            GetKeyState(Win32ButtonIds[ButtonIndex]) & (1 << 15));
                    }
                    
                    
                    
                    AppUpdateFrame(&GlobalAppMemory, RenderGroup, NewInput, DeltaTime);
                    
                    DirectXRenderFrame(RenderGroup);
                    
                    if(FAILED(DirectXRenderPresent()))
                    {
                        Running = false;
                    }
                    
                    LastCounter = EndCounter;
                    
                    app_input *TempInput = NewInput;
                    NewInput = OldInput;
                    OldInput = TempInput;
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

#define AssertFalse(Expression) \
++GlobalTestState_->TotalTests; \
if((Expression)) \
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

#define AssertNotEqualString(Expected, Actual) \
++GlobalTestState_->TotalTests; \
if(StringsAreEqual(Expected, Actual)) \
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
++GlobalTestState_->TotalTests; \
if(Expected != Actual) \
{ \
++GlobalTestState_->FailedTests; \
PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%lu'\n\t\t\tActual:      '%lu'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

void
RunAllTests(memory_arena *Arena);

s32
main()
{
    s32 Result = 0;
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    {
        LARGE_INTEGER PerfCountFrequencyResult;
        QueryPerformanceFrequency(&PerfCountFrequencyResult);
        GlobalWin32State.PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    }
    
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

#if defined(KENGINE_PREPROCESSOR)

typedef struct preprocessor_state
{
    memory_arena Arena;
} preprocessor_state;

inline void
GenerateCtor(c_struct Struct)
{
    PlatformConsoleOut("\n#define %S(", Struct.Name);
    
    b32 First = true;
    for(c_member *Member = Struct.Members;
        Member;
        Member = Member->Next)
    {
        if(!First)
        {
            PlatformConsoleOut(", ");
        }
        PlatformConsoleOut("%S", Member->Name);
        First = false;
    }
    
    PlatformConsoleOut(") (%S){", Struct.Type);
    First = true;
    for(c_member *Member = Struct.Members;
        Member;
        Member = Member->Next)
    {
        if(!First)
        {
            PlatformConsoleOut(", ");
        }
        if(Member->Type == C_Custom)
        {
            PlatformConsoleOut("(%S)", Member->Name);
        }
        else
        {
            PlatformConsoleOut("(%S)(%S)", Member->TypeName, Member->Name);
        }
        First = false;
    }
    PlatformConsoleOut("}");
}

inline void
GenerateSet1(c_struct Struct)
{
    PlatformConsoleOut("\n#define %SSet1(Value) (%S){", Struct.Name, Struct.Type);
    
    b32 First = true;
    for(c_member *Member = Struct.Members;
        Member;
        Member = Member->Next)
    {
        if(!First)
        {
            PlatformConsoleOut(", ");
        }
        PlatformConsoleOut("(%S)(Value)", Member->TypeName);
        First = false;
    }
    PlatformConsoleOut("}");
}

inline void
GenerateMath_(c_struct Struct, string Name, string Op)
{
    PlatformConsoleOut("\n#define %S%S(A, B) (%S){", Struct.Name, Name, Struct.Type);
    
    b32 First = true;
    for(c_member *Member = Struct.Members;
        Member;
        Member = Member->Next)
    {
        if(!First)
        {
            PlatformConsoleOut(", ");
        }
        PlatformConsoleOut("(A.%S %S B.%S)", Member->Name, Op, Member->Name);
        First = false;
    }
    PlatformConsoleOut("}");
}

inline void
GenerateMath(c_struct Struct)
{
    GenerateMath_(Struct, String("Add"), String("+"));
    GenerateMath_(Struct, String("Subtract"), String("-"));
    GenerateMath_(Struct, String("Multiply"), String("*"));
}

inline void
GenerateLinkedList(c_struct Struct)
{
    string Name = Struct.Name;
    string Type = Struct.Type;
    
    PlatformConsoleOut("\ninline u32\nGet%SCount(%S *Head)\n", Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    u32 Result = 0;\n\n");
    PlatformConsoleOut("    while(Head != 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        Head = Head->Next;\n");
    PlatformConsoleOut("        ++Result;\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    
    PlatformConsoleOut("\ninline %S *\nGet%SByIndex(%S *Head, s32 Index)\n", Type, Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("        %S *Result = Head;\n\n", Type);
    PlatformConsoleOut("        if(Result != 0)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            while(Result && Index)\n");
    PlatformConsoleOut("            {\n");
    PlatformConsoleOut("                Result = Result->Next;\n");
    PlatformConsoleOut("            --Index;\n");
    PlatformConsoleOut("            }\n");
    PlatformConsoleOut("        }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninline s32\nGetIndexOf%S(%S *Head, %S *%S)\n", Name, Type, Type, Name);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    s32 Result = -1;\n\n");
    PlatformConsoleOut("    for(s32 Index = 0;\n");
    PlatformConsoleOut("        Head;\n");
    PlatformConsoleOut("        ++Index, Head = Head->Next)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("        if(Head == %S)\n", Name);
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Index;\n");
    PlatformConsoleOut("            break;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ntypedef b32 %S_predicate(void *Context, %S *A, %S *B);\n", Type, Type, Type);
    
    PlatformConsoleOut("\ninline %S *\nGet%S(%S *Head, %S_predicate *Predicate, void *Context, %S *Match)\n", Type, Name, Type, Type, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = 0;\n\n", Type);
    PlatformConsoleOut("    while(Head)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        if(Predicate(Context, Head, Match))\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Head;\n");
    PlatformConsoleOut("            break;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("        else\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Head = Head->Next;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninline %S *\nGet%STail(%S *Head)\n", Type, Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = Head;\n\n", Type);
    PlatformConsoleOut("    if(Result != 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        while(Result->Next != 0)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Result->Next;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninline %S *\nPush%S(%S **HeadRef, memory_arena *Arena)\n", Type, Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = PushStruct(Arena, %S);\n\n", Type, Type);
    PlatformConsoleOut("    Result->Next = *HeadRef;\n");
    PlatformConsoleOut("    *HeadRef = Result;\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninline %S *\nPushback%S(%S **HeadRef, memory_arena *Arena)\n", Type, Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = PushStruct(Arena, %S);\n\n", Type, Type);
    PlatformConsoleOut("    Result->Next = 0;\n");
    PlatformConsoleOut("    if(*HeadRef == 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        *HeadRef = Result;\n");
    PlatformConsoleOut("    }\n");
    PlatformConsoleOut("    else\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        %S *Tail = Get%STail(*HeadRef);\n", Type, Name);
    PlatformConsoleOut("        Tail->Next = Result;\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninline %S *\n%SMergeSort_(%S *Front, %S *Back, %S_predicate *Predicate, void *Context, sort_type SortType)\n", 
                       Type, Name, Type, Type, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = 0;\n", Type);
    PlatformConsoleOut("    if(Front == 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        Result = Back;\n");
    PlatformConsoleOut("    }\n");
    PlatformConsoleOut("    else if(Back == 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        Result = Front;\n");
    PlatformConsoleOut("    }\n");
    PlatformConsoleOut("    else\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        b32 PredicateResult = Predicate(Context, Front, Back);\n");
    PlatformConsoleOut("        if(SortType == Sort_Descending)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            PredicateResult = !PredicateResult;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("        else\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Assert(SortType == Sort_Ascending);\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("        if(PredicateResult)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Front;\n");
    PlatformConsoleOut("            Result->Next = %SMergeSort_(Front->Next, Back, Predicate, Context, SortType);\n", Name);
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("        else\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Back;\n");
    PlatformConsoleOut("            Back->Next = %SMergeSort_(Front, Back->Next, Predicate, Context, SortType);\n", Name);
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    
    PlatformConsoleOut("\ninline void\n%SFrontBackSplit(%S *Head, %S **FrontRef, %S **BackRef)\n", Name, Type, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Fast;\n", Type);
    PlatformConsoleOut("    %S *Slow;\n", Type);
    PlatformConsoleOut("    Slow = Head;\n");
    PlatformConsoleOut("    Fast = Head->Next;\n\n");
    PlatformConsoleOut("    while(Fast != 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        Fast = Fast->Next;\n");
    PlatformConsoleOut("        if(Fast != 0)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Slow = Slow->Next;\n");
    PlatformConsoleOut("            Fast = Fast->Next;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    *FrontRef = Head;\n");
    PlatformConsoleOut("    *BackRef = Slow->Next;\n");
    PlatformConsoleOut("    Slow->Next = 0;\n");
    PlatformConsoleOut("}\n");
    
    
    PlatformConsoleOut("\ninline void\n%SMergeSort(%S **HeadRef, %S_predicate *Predicate, void *Context, sort_type SortType)\n", Name, Type, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Head = *HeadRef;\n\n", Type);
    PlatformConsoleOut("    if((Head!= 0) &&\n");
    PlatformConsoleOut("       (Head->Next != 0))\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        %S *Front;\n", Type);
    PlatformConsoleOut("        %S *Back;\n", Type);
    PlatformConsoleOut("        %SFrontBackSplit(Head, &Front, &Back);\n\n", Name);
    PlatformConsoleOut("        %SMergeSort(&Front, Predicate, Context, SortType);\n", Name);
    PlatformConsoleOut("        %SMergeSort(&Back, Predicate, Context, SortType);\n\n", Name);
    PlatformConsoleOut("        *HeadRef = %SMergeSort_(Front, Back, Predicate, Context, SortType);\n", Name);
    PlatformConsoleOut("    }\n");
    PlatformConsoleOut("}\n");
}

void
GenerateCodeFor(memory_arena *Arena, c_struct Struct, string_list *Options);

internal void
PreprocessSourceFile(memory_arena *Arena, string FileName, string FileData)
{
    tokenizer Tokenizer_ = Tokenize(FileData, FileName);
    tokenizer *Tokenizer = &Tokenizer_;
    
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_Unknown)
        {
            TokenError(Tokenizer, Token, "Unknown token: \"%S\"", Token.Text);
        }
        else
        {
            if(Token.Type == Token_Identifier)
            {
                if(StringsAreEqual(Token.Text, String("introspect")))
                {
                    Token = RequireToken(Tokenizer, Token_OpenParenthesis);
                    
                    string_list *Options = 0;
                    
                    while(Parsing(Tokenizer))
                    {
                        Token = GetToken(Tokenizer);
                        if(Token.Type == Token_CloseParenthesis)
                        {
                            break;
                        }
                        else if(Token.Type == Token_Comma)
                        {
                        }
                        else if(Token.Type == Token_Identifier)
                        {
                            string Option = Token.Text;
                            PushStringToStringList(&Options, Arena, Option);
                        }
                        else
                        {
                            TokenError(Tokenizer, Token, "Expecting parameter list for introspect");
                        }
                    }
                    
                    if(Parsing(Tokenizer))
                    {
                        RequireIdentifierToken(Tokenizer, String("typedef"));
                        RequireIdentifierToken(Tokenizer, String("struct"));
                        Token = RequireToken(Tokenizer, Token_Identifier);
                        
                        if(Parsing(Tokenizer))
                        {                    
                            c_struct Struct = ParseStruct(Arena, Tokenizer, Token.Text);
                            
                            string_list *CustomOptions = 0;
                            
                            for(string_list *Option = Options;
                                Option;
                                Option = Option->Next)
                            {
                                if(StringsAreEqual(String("ctor"), Option->Entry))
                                {
                                    GenerateCtor(Struct);
                                }
                                else if(StringsAreEqual(String("set1"), Option->Entry))
                                {
                                    GenerateSet1(Struct);
                                }
                                else if(StringsAreEqual(String("math"), Option->Entry))
                                {
                                    GenerateMath(Struct);
                                }
                                else if(StringsAreEqual(String("linked_list"), Option->Entry))
                                {
                                    GenerateLinkedList(Struct);
                                }
                                else
                                {
                                    PushStringToStringList(&CustomOptions, Arena, Option->Entry);
                                }
                            }
                            
                            GenerateCodeFor(Arena, Struct, CustomOptions);
                        }
                        
                        PlatformConsoleOut("\n");
                    }
                }
            }
        }
    }
}

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
    
    if(ArgCount == 1)
    {
        PlatformConsoleOut("#error No input file specified\n");
    }
    else
    {    
        preprocessor_state *PreprocessorState = BootstrapPushStruct(preprocessor_state, Arena);
        
        for(u32 ArgIndex = 1;
            ArgIndex < ArgCount;
            ++ArgIndex)
        {
            char *Arg = Args[ArgIndex];
            string FileName = String_(GetNullTerminiatedStringLength(Arg), (u8 *)Arg);
            if(PlatformFileExists(FileName))
            {
                PlatformConsoleOut("\n// %S\n", FileName);
                
                temporary_memory MemoryFlush = BeginTemporaryMemory(&PreprocessorState->Arena);
                
                string FileData = PlatformReadEntireFile(MemoryFlush.Arena, FileName);
                
                PreprocessSourceFile(MemoryFlush.Arena, FileName, FileData);
                
                EndTemporaryMemory(MemoryFlush);
            }
            else
            {
                PlatformConsoleOut("\n#error invalid filename specified: %S\n", FileName);
            }
        }
    }
    
    return Result;
}

#endif // defined(KENGINE_PREPROCESSOR)


#endif //KENGINE_IMPLEMENTATION


#define KENGINE_H
#endif //KENGINE_H
