#include <windows.h>
#include <xinput.h>

typedef struct win32_state
{
    memory_arena Arena;
    b32 Running;
    offscreen_buffer Backbuffer;

    BITMAPINFO Info;
} win32_state;
global win32_state *GlobalWin32State;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(0);
}
global x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(0);
}
global x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(void)    
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
    }
}

internal window_dimension
Win32GetWindowDimension(HWND Window)
{
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    
    window_dimension Result = 
    {
        .Width = ClientRect.right - ClientRect.left,
        .Height = ClientRect.bottom - ClientRect.top
    };

    return Result;
}

internal void
Win32ResizeDIBSection(win32_state *State, s32 Width, s32 Height)
{
    offscreen_buffer *Buffer = &State->Backbuffer;

    // TODO(kstandbridge): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if(Buffer->Memory)
    {
        // TODO(kstandbridge): Free the Buffer-Memory
        // VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;

    // NOTE(kstandbridge): When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    State->Info.bmiHeader.biSize = sizeof(State->Info.bmiHeader);
    State->Info.bmiHeader.biWidth = Buffer->Width;
    State->Info.bmiHeader.biHeight = -Buffer->Height;
    State->Info.bmiHeader.biPlanes = 1;
    State->Info.bmiHeader.biBitCount = 32;
    State->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = PushSize_(&State->Arena, BitmapMemorySize, DefaultArenaPushParams());
    Buffer->Pitch = Width*BytesPerPixel;

    // TODO(kstandbridge): Probably clear this to black
}

internal void
Win32DisplayBufferInWindow(win32_state *State,
                           HDC DeviceContext, s32 WindowWidth, s32 WindowHeight)
{
    offscreen_buffer *Buffer = &State->Backbuffer;

    // TODO(kstandbridge): Aspect ratio correction
    // TODO(kstandbridge): Plat with stetch mnodes
    StretchDIBits(DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &State->Info,
        DIB_RGB_COLORS, SRCCOPY);
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
            GlobalWin32State->Running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            GlobalWin32State->Running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u32 VKCode = WParam;
            b32 WasDown = ((LParam & (1 << 30)) != 0);
            b32 IsDown = ((LParam & (1 << 31)) == 0);
            if(WasDown != IsDown)
            {
                if(VKCode == 'W')
                {
                }
                else if(VKCode == 'A')
                {
                }
                else if(VKCode == 'S')
                {
                }
                else if(VKCode == 'D')
                {
                }
                else if(VKCode == 'Q')
                {
                }
                else if(VKCode == 'E')
                {
                }
                else if(VKCode == VK_UP)
                {
                }
                else if(VKCode == VK_LEFT)
                {
                }
                else if(VKCode == VK_DOWN)
                {
                }
                else if(VKCode == VK_RIGHT)
                {
                }
                else if(VKCode == VK_ESCAPE)
                {
                    PlatformConsoleOut("ESCAPE: ");
                    if(IsDown)
                    {
                        PlatformConsoleOut("IsDown ");
                    }
                    if(WasDown)
                    {
                        PlatformConsoleOut("WasDown");
                    }
                    PlatformConsoleOut("\n");
                }
                else if(VKCode == VK_SPACE)
                {
                }
            }
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(GlobalWin32State, DeviceContext,
                                       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

int CALLBACK
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;

    Win32LoadXInput();

    WNDCLASS WindowClass = 
    {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = Win32MainWindowCallback,
        .hInstance = hInstance,
        // .hIcon,
        .lpszClassName = "HandmadeHeroWindowClass",
    };
    
    GlobalWin32State = BootstrapPushStruct(win32_state, Arena);

    Win32ResizeDIBSection(GlobalWin32State, 1280, 720);
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                hInstance,
                0);

        if(Window)
        {
            HDC DeviceContext = GetDC(Window);

            s32 XOffset = 0;
            s32 YOffset = 0;
            
            GlobalWin32State->Running = true;
            while(GlobalWin32State->Running)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        GlobalWin32State->Running = false;
                    }
                    
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                // TODO(kstandbridge): Should we poll this more frequently
                for (DWORD ControllerIndex = 0;
                     ControllerIndex < XUSER_MAX_COUNT;
                     ++ControllerIndex)
                {
                    XINPUT_STATE ControllerState;
                    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        // NOTE(kstandbridge): This controller is plugged in
                        // TODO(kstandbridge): See if ControllerState.dwPacketNumber increments too rapidly
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        b32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        b32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        b32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        b32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        b32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        b32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        b32 LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        b32 RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        b32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        b32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        b32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        b32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
        
                        s16 StickX = Pad->sThumbLX;
                        s16 StickY = Pad->sThumbLY;

                        // TODO(kstandbridge): Check controller input
                        XOffset += StickX >> 12;
                        YOffset += StickY >> 12;
                    }
                    else
                    {
                        // NOTE(kstandbridge): The controller is not available
                    }
                }

                RenderWeirdGradient(&GlobalWin32State->Backbuffer, XOffset, YOffset);

                window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(GlobalWin32State, DeviceContext,
                                            Dimension.Width, Dimension.Height);
            }
        }
        else
        {
            // TODO(kstandbridge): Logging
        }
    }
    else
    {
        // TODO(kstandbridge): Logging
    }
    
    return 0;
}