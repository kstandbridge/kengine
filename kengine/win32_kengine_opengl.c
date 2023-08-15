typedef struct win32_offscreen_buffer
{
    // NOTE(kstandbridge): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    s32 Width;
    s32 Height;
    s32 Pitch;
} win32_offscreen_buffer;

typedef struct win32_window_dimension
{
    s32 Width;
    s32 Height;
} win32_window_dimension;

global memory_arena GlobalArena;
global b32 GlobalRunning;
global win32_offscreen_buffer GlobalBackbuffer;

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, s32 BlueOffset, s32 GreenOffset)
{    
    u8 *Row = (u8 *)Buffer.Memory;    
    for(s32 Y = 0;
        Y < Buffer.Height;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(s32 X = 0;
            X < Buffer.Width;
            ++X)
        {
            u8 Blue = (X + BlueOffset);
            u8 Green = (Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }
        
        Row += Buffer.Pitch;
    }
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, s32 Width, s32 Height)
{
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
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = PushSize_(&GlobalArena, BitmapMemorySize, DefaultArenaPushParams());
    Buffer->Pitch = Width*BytesPerPixel;

    // TODO(kstandbridge): Probably clear this to black
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext,
                           int WindowWidth, int WindowHeight,
                           win32_offscreen_buffer Buffer)
{
    // TODO(kstandbridge): Aspect ratio correction
    // TODO(kstandbridge): Plat with stetch mnodes
    StretchDIBits(DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory,
        &Buffer.Info,
        DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
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
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                                       GlobalBackbuffer);
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
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;

    WNDCLASS WindowClass = 
    {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = Win32MainWindowCallback,
        .hInstance = hInstance,
        // .hIcon,
        .lpszClassName = "HandmadeHeroWindowClass",
    };
    
    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
    
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
            
            GlobalRunning = true;
            while(GlobalRunning)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }
                    
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                                           GlobalBackbuffer);
                
                ++XOffset;
                YOffset += 2;
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