#include "win32_kengine.h"

global platform_api Platform;

global GLuint GlobalBlitTextureHandle;

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013

#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_FULL_ACCELERATION_ARB               0x2027

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB        0x20A9

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC hDC, HGLRC hShareContext,
                                                    const int *attribList);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_iv_arb(HDC hdc,
                                                       int iPixelFormat,
                                                       int iLayerPlane,
                                                       UINT nAttributes,
                                                       const int *piAttributes,
                                                       int *piValues);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_fv_arb(HDC hdc,
                                                       int iPixelFormat,
                                                       int iLayerPlane,
                                                       UINT nAttributes,
                                                       const int *piAttributes,
                                                       FLOAT *pfValues);

typedef BOOL WINAPI wgl_choose_pixel_format_arb(HDC hdc,
                                                const int *piAttribIList,
                                                const FLOAT *pfAttribFList,
                                                UINT nMaxFormats,
                                                int *piFormats,
                                                UINT *nNumFormats);

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
typedef const char * WINAPI wgl_get_extensions_string_ext(void);

global wgl_create_context_attribs_arb *wglCreateContextAttribsARB;
global wgl_choose_pixel_format_arb *wglChoosePixelFormatARB;
global wgl_swap_interval_ext *wglSwapIntervalEXT;
global wgl_get_extensions_string_ext *wglGetExtensionsStringEXT;
global b32 OpenGLSupportsSRGBFramebuffer;
global GLuint OpenGLDefaultInternalTextureFormat;
global b32 HardwareRendering = false;
#include "kengine_render.c"
#include "kengine_opengl.c"

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


global b32 GlobalIsRunning;
global s64 GlobalPerfCountFrequency;
global win32_offscreen_buffer GlobalBackbuffer;

#define MAX_GLYPH_COUNT 5000
global KERNINGPAIR *GlobalKerningPairs;
global DWORD GlobalKerningPairCount;
global TEXTMETRIC GlobalTextMetric;

global HDC FontDeviceContext;
internal loaded_bitmap
DEBUGGetGlyphForCodePoint(memory_arena *Arena, u32 CodePoint)
{
    
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024
    
    local_persist b32 FontInitialized;
    local_persist VOID *FontBits;
    
    local_persist HFONT FontHandle;
    
    local_persist u32 CurrentGlyphIndex = 0;
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
        AddFontResourceExA("c:/Windows/Fonts/segoeui.ttf", FR_PRIVATE, 0);
        s32 PointSize = 11;
        s32 FontHeight = -MulDiv(PointSize, GetDeviceCaps(FontDeviceContext, LOGPIXELSY), 72);
        FontHandle = CreateFontA(FontHeight, 0, 0, 0,
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
        
        SelectObject(FontDeviceContext, FontHandle);
        GetTextMetrics(FontDeviceContext, &GlobalTextMetric);
        
        GlobalKerningPairCount = GetKerningPairsW(FontDeviceContext, 0, 0);
        GlobalKerningPairs = PushArray(Arena, GlobalKerningPairCount, KERNINGPAIR);
        GetKerningPairsW(FontDeviceContext, GlobalKerningPairCount, GlobalKerningPairs);
        
        FontInitialized = true;
    }
    
    loaded_bitmap Result;
    ZeroStruct(Result);
    
    SelectObject(FontDeviceContext, FontHandle);
    
    memset(FontBits, 0x00, MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32));
    
    wchar_t CheesePoint = (wchar_t)CodePoint;
    
    SIZE Size;
    GetTextExtentPoint32W(FontDeviceContext, &CheesePoint, 1, &Size);
    
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
    
    SetBkMode(FontDeviceContext, TRANSPARENT);
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
DEBUGGetHorizontalAdvanceForPair(u32 PrevCodePoint, u32 CodePoint)
{
    INT ThisWidth;
    GetCharWidth32W(FontDeviceContext, PrevCodePoint, CodePoint, &ThisWidth);
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
DEBUGGetLineAdvance()
{
    f32 Result = (f32)GlobalTextMetric.tmAscent + (f32)GlobalTextMetric.tmDescent + (f32)GlobalTextMetric.tmDescent;
    
    return Result;
}

internal void
Win32SetPixelFormat(HDC WindowDC)
{
    s32 SuggestedPixelFormatIndex = 0;
    GLuint ExtendedPick = 0;
    if(wglChoosePixelFormatARB)
    {
        int IntAttribList[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0,
        };
        
        if(!OpenGLDefaultInternalTextureFormat)
        {
            IntAttribList[10] = 0;
        }
        
        wglChoosePixelFormatARB(WindowDC, IntAttribList, 0, 1, 
                                &SuggestedPixelFormatIndex, &ExtendedPick);
    }
    
    if(!ExtendedPick)
    {
        PIXELFORMATDESCRIPTOR DesiredPixelFormat;
        DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
        DesiredPixelFormat.nVersion = 1;
        DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        DesiredPixelFormat.cColorBits = 32;
        DesiredPixelFormat.cAlphaBits = 8;
        DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
        
        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    }
    
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
}

internal void
Win32LoadWGLExtensions()
{
    WNDCLASSA WindowClass;
    ZeroStruct(WindowClass);
    
    WindowClass.lpfnWndProc = DefWindowProcA;
    WindowClass.hInstance = GetModuleHandle(0);
    WindowClass.lpszClassName = "kEngineWGLLoader";
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
                                      0,
                                      WindowClass.lpszClassName,
                                      "kEngine",
                                      0,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      0,
                                      0,
                                      WindowClass.hInstance,
                                      0);
        
        HDC WindowDC = GetDC(Window);
        Win32SetPixelFormat(WindowDC);
        HGLRC OpenGLRC = wglCreateContext(WindowDC);
        if(wglMakeCurrent(WindowDC, OpenGLRC))        
        {
            wglChoosePixelFormatARB = 
            (wgl_choose_pixel_format_arb *)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB =
            (wgl_create_context_attribs_arb *)wglGetProcAddress("wglCreateContextAttribsARB");
            wglSwapIntervalEXT = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
            wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *)wglGetProcAddress("wglGetExtensionsStringEXT");
            
            if(wglGetExtensionsStringEXT)
            {
                char *Extensions = (char *)wglGetExtensionsStringEXT();
                char *At = Extensions;
                while(*At)
                {
                    while(IsWhitespace(*At)) {++At;}
                    char *End = At;
                    while(*End && !IsWhitespace(*End)) {++End;}
                    
                    umm Count = End - At;        
                    
                    if(0) {}
                    else if(StringsAreEqual(StringInternal(Count, (u8 *)At), String("WGL_EXT_framebuffer_sRGB"))) {OpenGLSupportsSRGBFramebuffer = true;}
                    
                    At = End;
                }
            }
            
            wglMakeCurrent(0, 0);
        }
        
        wglDeleteContext(OpenGLRC);
        ReleaseDC(Window, WindowDC);
        DestroyWindow(Window);
    }
}

s32 Win32OpenGLAttribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    WGL_CONTEXT_FLAGS_ARB, 0 // NOTE(kstandbridge): Enable for testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if KENGINE_INTERNAL
    |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    0,
};


internal HGLRC
Win32InitOpenGL(HDC WindowDC)
{
    Win32LoadWGLExtensions();
    
    b32 ModernContext = true;
    HGLRC OpenGLRC = 0;
    if(wglCreateContextAttribsARB)
    {
        Win32SetPixelFormat(WindowDC);
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }
    
    if(!OpenGLRC)
    {
        ModernContext = false;
        OpenGLRC = wglCreateContext(WindowDC);
    }
    
    if(wglMakeCurrent(WindowDC, OpenGLRC))
    {
        OpenGLInit(ModernContext);
        if(wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(1);
        }
        HardwareRendering = true;
    }
    
    return OpenGLRC;
}

inline b32
TryParseCodePoint(u32 CodePoint, char *Buffer)
{
    b32 Result = true;
    
    if (CodePoint <= 0x7F) 
    {
        Buffer[0] = (char) CodePoint;
        Buffer[1] = '\0';
    }
    else if (CodePoint <= 0x7FF)
    {
        Buffer[0] = 0xC0 | (char) ((CodePoint >> 6) & 0x1F);
        Buffer[1] = 0x80 | (char) (CodePoint & 0x3F);
        Buffer[2] = '\0';
    } 
    else if (CodePoint <= 0xFFFF)
    {
        Buffer[0] = 0xE0 | (char) ((CodePoint >> 12) & 0x0F);
        Buffer[1] = 0x80 | (char) ((CodePoint >> 6) & 0x3F);
        Buffer[2] = 0x80 | (char) (CodePoint & 0x3F);
        Buffer[3] = '\0';
    } 
    else if (CodePoint <= 0x10FFFF)
    {
        Buffer[0] = 0xF0 | (char) ((CodePoint >> 18) & 0x0F);
        Buffer[1] = 0x80 | (char) ((CodePoint >> 12) & 0x3F);
        Buffer[2] = 0x80 | (char) ((CodePoint >> 6) & 0x3F);
        Buffer[3] = 0x80 | (char) (CodePoint & 0x3F);
        Buffer[4] = '\0';
    }
    else
    {
        Result = false;
    }
    
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

inline void
ProcessInputMessage(app_button_state *NewState, b32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
Win32ProcessPendingMessages(app_input *Input)
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
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)Msg.wParam;
                b32 WasDown = ((Msg.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Msg.lParam & (1 << 31)) == 0);
                
                switch(VKCode)
                {
                    case VK_DELETE:   { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Delete, IsDown); } break;
                    case VK_BACK:   { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Backspace, IsDown); } break;
                    case VK_RETURN: { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Return, IsDown); } break;
                    case VK_UP:     { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Up, IsDown); } break;
                    case VK_LEFT:   { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Left, IsDown); } break;
                    case VK_DOWN:   { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Down, IsDown); } break;
                    case VK_RIGHT:  { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Right, IsDown); } break;
                    case VK_ESCAPE: { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Escape, IsDown); } break;
                    case VK_HOME: { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_Home, IsDown); } break;
                    case VK_END: { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_End, IsDown); } break;
                    
                    
                    case 'A': { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_A, IsDown); } break;
                    case 'C': { ProcessInputMessage(Input->KeyboardButtons + KeyboardButton_C, IsDown); } break;
                }
                
                if(IsDown)
                {
                    b32 AltKeyWasDown = (Msg.lParam & (1 << 29));
                    if((VKCode == VK_F4) && AltKeyWasDown)
                    {
                        GlobalIsRunning = false;
                    }
                    if((VKCode == VK_RETURN) && AltKeyWasDown)
                    {
                        if(Msg.hwnd)
                        {
                            // NOTE(kstandbridge): http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
                            local_persist WINDOWPLACEMENT WindowPosition;
                            WindowPosition.length = sizeof(WINDOWPLACEMENT);
                            
                            HWND WindowHwnd = Msg.hwnd;
                            
                            DWORD Style = GetWindowLong(WindowHwnd, GWL_STYLE);
                            if(Style & WS_OVERLAPPEDWINDOW)
                            {
                                MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
                                if(GetWindowPlacement(WindowHwnd, &WindowPosition) &&
                                   GetMonitorInfo(MonitorFromWindow(WindowHwnd, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
                                {
                                    SetWindowLong(WindowHwnd, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
                                    SetWindowPos(WindowHwnd, HWND_TOP,
                                                 MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                                                 MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                                                 MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                                                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                                }
                            }
                            else
                            {
                                SetWindowLong(WindowHwnd, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
                                SetWindowPlacement(WindowHwnd, &WindowPosition);
                                SetWindowPos(WindowHwnd, 0, 0, 0, 0, 0,
                                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                                             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                            }
                        }
                    }
                }
                
                // NOTE(kstandbridge): Dispatch messages so we can pickup WM_CHAR for text input
                TranslateMessage(&Msg);
                DispatchMessageA(&Msg);
                
            } break;
            
            case WM_CHAR:
            {
                WORD VKCode = LOWORD(Msg.wParam);
                WORD KeyFlags = HIWORD(Msg.lParam);
                WORD ScanCode = LOBYTE(KeyFlags);
                BOOL IsExtendedKey = (KeyFlags & KF_EXTENDED) == KF_EXTENDED;
                if (IsExtendedKey)
                {
                    ScanCode = MAKEWORD(ScanCode, 0xE0);
                }
                BOOL RepeatFlag = (KeyFlags & KF_REPEAT) == KF_REPEAT;
                WORD RepeatCount = LOWORD(Msg.lParam);
                BOOL UpFlag = (KeyFlags & KF_UP) == KF_UP;
                
                if(!TryParseCodePoint((u32)Msg.wParam, Input->Text))
                {
                    __debugbreak();
                }
                
                // NOTE(kstandbridge): Only printable characters
                if (*Input->Text < ' ' || *Input->Text == 127) 
                {
                    Input->Text[0] = '\0';
                }
                
            } break;
            
            case WM_MOUSEWHEEL:
            {
                Input->MouseZ = GET_WHEEL_DELTA_WPARAM(Msg.wParam);
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
    
    AppCode->AppUpdateFrame = 0;
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
            Result.AppUpdateFrame = (app_update_frame *)GetProcAddress(Result.Library, "AppUpdateFrame");
        }
    }
    
    return Result;
}

internal string
GetCommandLineArgs(memory_arena *Arena)
{
    LPSTR Args = GetCommandLineA();
    
    string Result = FormatString(Arena, "%s", Args);
    
    return Result;
}

internal b32
SetClipboardText(string Text)
{
    b32 Result = false;
    
    if(OpenClipboard(0))
    {
        if(EmptyClipboard())
        {
            HGLOBAL GlobalCopy = GlobalAlloc(GMEM_MOVEABLE, (Text.Size + 1) * sizeof(char));
            Assert(GlobalCopy);
            LPTSTR StringCopy = (LPTSTR)GlobalLock(GlobalCopy);
            Copy(Text.Size * sizeof(char), Text.Data, StringCopy);
            StringCopy[Text.Size] = '\0';
            GlobalUnlock(GlobalCopy);
            
            HANDLE CopyHandle = SetClipboardData(CF_TEXT, GlobalCopy);
            Assert(CopyHandle);
            Result = true;
            
            CloseClipboard();
        }
    }
    
    return Result;
}

internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           char *Dest)
{
    for(int Index = 0;
        Index < SourceACount;
        ++Index)
    {
        *Dest++ = *SourceA++;
    }
    
    for(int Index = 0;
        Index < SourceBCount;
        ++Index)
    {
        *Dest++ = *SourceB++;
    }
    
    *Dest++ = 0;
}

internal void
BuildEXEPathFileName(app_code  *State, char *FileName, char *Dest)
{
    // TODO(kstandbridge): Replace this with string format
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
               GetNullTerminiatedStringLength(FileName), FileName, Dest);
}

internal void
GetEXEFileName(app_code *Code)
{
    DWORD SizeOfFilename = GetModuleFileNameA(0, Code->EXEFileName, sizeof(Code->EXEFileName));
    Code->OnePastLastEXEFileNameSlash = Code->EXEFileName;
    for(char *Scan = Code->EXEFileName;
        *Scan;
        ++Scan)
    {
        if(*Scan == '\\')
        {
            Code->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
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
            HDC OpenGLDC = GetDC(WindowHwnd);
            HGLRC OpenGLRC = Win32InitOpenGL(OpenGLDC);
            
            s32 PointSize = 8;
            s32 FontHeight = -MulDiv(PointSize, GetDeviceCaps(OpenGLDC, LOGPIXELSY), 72);
            HFONT FontHandle = CreateFontA(FontHeight, 0, 0, 0,
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
            
            HFONT OldFont = (HFONT)SelectObject(OpenGLDC, FontHandle);
            wglUseFontBitmaps(OpenGLDC, 0, 255, 1000);
            SelectObject(OpenGLDC, OldFont);
            DeleteObject(FontHandle);
            
            u32 MonitorRefreshHz = 60;
            u32 Win32RefreshRate = GetDeviceCaps(OpenGLDC, VREFRESH);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            f32 AppUpdateHz = (f32)(MonitorRefreshHz);
            
            
            platform_work_queue PerFrameWorkQueue;
            MakeQueue(&PerFrameWorkQueue, 6);
            platform_work_queue BackgroundWorkQueue;
            MakeQueue(&BackgroundWorkQueue, 2);
            
#if KENGINE_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif
            app_memory AppMemory;
            ZeroStruct(AppMemory);
#if KENGINE_INTERNAL
            AppMemory.DebugStorageSize = Megabytes(1024);
            AppMemory.DebugStorage = VirtualAlloc(0, (size_t)AppMemory.DebugStorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#endif
            AppMemory.StorageSize = Megabytes(1024);
            AppMemory.Storage = VirtualAlloc(BaseAddress, (size_t)AppMemory.StorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            AppMemory.PlatformAPI.PerFrameWorkQueue = &PerFrameWorkQueue;
            AppMemory.PlatformAPI.BackgroundWorkQueue = &BackgroundWorkQueue;
            AppMemory.PlatformAPI.AddWorkEntry = AddWorkEntry;
            AppMemory.PlatformAPI.CompleteAllWork = CompleteAllWork;
            
            AppMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGReadEntireFile;
            AppMemory.PlatformAPI.DEBUGGetGlyphForCodePoint = DEBUGGetGlyphForCodePoint;
            AppMemory.PlatformAPI.DEBUGGetHorizontalAdvanceForPair = DEBUGGetHorizontalAdvanceForPair;
            AppMemory.PlatformAPI.DEBUGGetLineAdvance = DEBUGGetLineAdvance;
            
            AppMemory.PlatformAPI.GetCommandLineArgs = GetCommandLineArgs;
            AppMemory.PlatformAPI.SetClipboardText = SetClipboardText;
            
            Platform = AppMemory.PlatformAPI;
            
            app_input Input[2];
            ZeroArray(ArrayCount(Input), Input);
            app_input *NewInput = &Input[0];
            app_input *OldInput = &Input[1];
            
            app_code AppCode; // = Win32LoadAppCode(SourceAppCodeDLLFullPath, TempAppCodeDLLFullPath, AppCodeLockFullPath);
            ZeroStruct(AppCode);
            
            GetEXEFileName(&AppCode);
            
            char SourceAppCodeDLLFullPath[MAX_PATH];
            BuildEXEPathFileName(&AppCode, "kengine.dll", SourceAppCodeDLLFullPath);
            
            char TempAppCodeDLLFullPath[MAX_PATH];
            BuildEXEPathFileName(&AppCode, "kengine_temp.dll", TempAppCodeDLLFullPath);
            
            char AppCodeLockFullPath[MAX_PATH];
            BuildEXEPathFileName(&AppCode, "lock.tmp", AppCodeLockFullPath);
            
            umm CurrentSortMemorySize = Megabytes(1);
            void *SortMemory = VirtualAlloc(0, CurrentSortMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
            umm PushBufferSize = Megabytes(4);
            void *PushBuffer = VirtualAlloc(0, PushBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
            GlobalIsRunning = true;
            while(GlobalIsRunning)
            {
                u32 ExpectedFramesPerUpdate = 1;
                f32 TargetSecondsPerFrame = (f32)ExpectedFramesPerUpdate / (f32)AppUpdateHz;
                
                AppMemory.ExecutableReloaded = false;
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceAppCodeDLLFullPath);
                if(CompareFileTime(&NewDLLWriteTime, &AppCode.LastDLLWriteTime) != 0)
                {
                    CompleteAllWork(&PerFrameWorkQueue);
                    CompleteAllWork(&BackgroundWorkQueue);
                    
                    Win32UnloadAppCode(&AppCode);
                    AppCode = Win32LoadAppCode(SourceAppCodeDLLFullPath, TempAppCodeDLLFullPath, AppCodeLockFullPath);
                    AppMemory.ExecutableReloaded = true;
                }
                
                NewInput->Text[0] = '\0';
                ZeroArray(KeyboardButton_Count, NewInput->KeyboardButtons);
                NewInput->ShiftDown = (GetKeyState(VK_SHIFT) & (1 << 15));
                NewInput->AltDown = (GetKeyState(VK_MENU) & (1 << 15));
                NewInput->ControlDown = (GetKeyState(VK_CONTROL) & (1 << 15));
                NewInput->MouseZ = 0;
                Win32ProcessPendingMessages(NewInput);
                
                POINT MouseP;
                GetCursorPos(&MouseP);
                ScreenToClient(WindowHwnd, &MouseP);
                NewInput->MouseX = (f32)MouseP.x;
                NewInput->MouseY = (f32)((GlobalBackbuffer.Height - 1) - MouseP.y);
                
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
                                        GetKeyState(ButtonVKs[ButtonIndex]) & (1 << 15));
                }
                // NOTE(kstandbridge): Render
                
                // TODO(kstandbridge): preprocessor to generate rendercommands ctor
                app_render_commands RenderCommands;
                RenderCommands.Width = GlobalBackbuffer.Width;
                RenderCommands.Height = GlobalBackbuffer.Height;
                RenderCommands.MaxPushBufferSize = PushBufferSize;
                RenderCommands.PushBufferSize = 0;
                RenderCommands.PushBufferBase = (u8 *)PushBuffer;
                RenderCommands.PushBufferElementCount = 0;
                RenderCommands.SortEntryAt = PushBufferSize;
                
                if(AppCode.AppUpdateFrame)
                {
                    AppCode.AppUpdateFrame(&AppMemory, NewInput, &RenderCommands);
                }
                
                umm NeededSortMemorySize = RenderCommands.PushBufferElementCount * sizeof(tile_sort_entry);
                if(CurrentSortMemorySize < NeededSortMemorySize)
                {
                    VirtualFree(SortMemory, 0, MEM_FREE);
                    CurrentSortMemorySize = NeededSortMemorySize;
                    SortMemory = VirtualAlloc(0, CurrentSortMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
                }
                
                // TODO(kstandbridge): Enable element sorting
                SortRenderCommands(&RenderCommands, SortMemory);
                HDC DeviceContext = GetDC(WindowHwnd);
                if(HardwareRendering)
                {
#if 1                    
                    OpenGLRenderCommands(&RenderCommands);
#else
                    glViewport(0, 0, RenderCommands.Width, RenderCommands.Height);
                    
                    glEnable(GL_TEXTURE_2D);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    
                    glMatrixMode(GL_TEXTURE);
                    glLoadIdentity();
                    
                    OpenGLSetScreenspace(RenderCommands.Width, RenderCommands.Height);
                    
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    
                    glRasterPos2f(30.0f, 300.0f); 
                    glListBase(1000);
                    glCallLists (26, GL_UNSIGNED_BYTE, "Hello Windows OpenGL World");
#endif
                    
                    SwapBuffers(DeviceContext);
                }
                else
                {
                    loaded_bitmap OutputTarget;
                    ZeroStruct(OutputTarget);
                    OutputTarget.Memory = GlobalBackbuffer.Memory;
                    OutputTarget.Width = GlobalBackbuffer.Width;
                    OutputTarget.Height = GlobalBackbuffer.Height;
                    OutputTarget.Pitch = GlobalBackbuffer.Pitch;
                    
                    
                    SoftwareRenderCommands(Platform.PerFrameWorkQueue, &RenderCommands, &OutputTarget);
                    
                    StretchDIBits(DeviceContext, 0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height, 0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height, GlobalBackbuffer.Memory, &GlobalBackbuffer.Info, DIB_RGB_COLORS, SRCCOPY);
                    
                }
                ReleaseDC(WindowHwnd, DeviceContext);
                
                LARGE_INTEGER EndCounter = Win32GetWallClock();
                LastCounter = EndCounter;
                
                app_input *Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
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