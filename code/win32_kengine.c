#include "win32_kengine.h"
#include "win32_kengine_shared.c"

#include "win32_kengine_generated.c"
#include "kengine_generated.c"

global b32 GlobalIsRunning;

internal void
Win32LoadLibraries()
{
    User32 = LoadLibraryA("User32.dll");
    Assert(User32);
}
internal LRESULT __stdcall
Win32MainWindowCallback(HWND Window,
                        u32 Message,
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
    
    if(RegisterClassExA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0,
                                      WindowClass.lpszClassName,
                                      "kengine",
                                      WS_OVERLAPPEDWINDOW,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                      0, 0, Instance, 0);
        if(Window)
        {
            ShowWindow(Window, SW_SHOW);
            
            GlobalIsRunning = true;
            while(GlobalIsRunning)
            {
                Win32ProcessPendingMessages();
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