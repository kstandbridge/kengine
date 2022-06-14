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
    char EXEFileName[MAX_PATH];
    char *OnePastLastEXEFileNameSlash;
    
    HMODULE Library;
    FILETIME LastDLLWriteTime;
    
    app_update_and_render *AppUpdateAndRender;
} app_code;


global b32 GlobalIsRunning;
global s64 GlobalPerfCountFrequency;
global win32_offscreen_buffer GlobalBackbuffer;

#define MAX_GLYPH_COUNT 5000
global f32 *GlobalCodePointHoriziontalAdvance;
global u32 *GlobalGlyphIndexFromCodePoint;
global KERNINGPAIR *GlobalKerningPairs;
global DWORD GlobalKerningPairCount;
global TEXTMETRIC GlobalTextMetric;

internal loaded_bitmap
DEBUGGetGlyphForCodePoint(memory_arena *Arena, u32 CodePoint)
{
    
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024
    
    local_persist b32 FontInitialized;
    local_persist VOID *FontBits;
    local_persist HDC FontDeviceContext;
    local_persist HFONT FontHandle;
    
    local_persist u32 CurrentGlyphIndex = 0;
    if(!FontInitialized)
    {
        memory_index CodePointSize = sizeof(f32) * MAX_GLYPH_COUNT * MAX_GLYPH_COUNT;
        GlobalCodePointHoriziontalAdvance = PushSize(Arena, CodePointSize);
        ZeroSize(CodePointSize, GlobalCodePointHoriziontalAdvance);
        
        memory_index GlyphIndexSize = sizeof(u32)*(MAX_GLYPH_COUNT * MAX_GLYPH_COUNT + 1);
        GlobalGlyphIndexFromCodePoint = PushSize(Arena, GlyphIndexSize);
        ZeroSize(GlyphIndexSize, GlobalGlyphIndexFromCodePoint);
        
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
        
        NONCLIENTMETRICS NonClientMetrics;
        NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
        if(SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, NonClientMetrics.cbSize, &NonClientMetrics, 0))
        {
            LOGFONTA LogFont = NonClientMetrics.lfMessageFont;
            // TODO(kstandbridge): Figure out how to specify pixels properly here
            s32 FontHeight = 128;
            //s32 PointSize = 72;
            //s32 FontHeight = -MulDiv(PointSize, GetDeviceCaps(FontDeviceContext, LOGPIXELSY), 72);
            //s32 FontHeight = LogFont.lfHeight;
            FontHandle = CreateFontA(FontHeight, LogFont.lfWidth, LogFont.lfEscapement, LogFont.lfOrientation,
                                     LogFont.lfWeight, LogFont.lfItalic, LogFont.lfUnderline, LogFont.lfStrikeOut, 
                                     LogFont.lfCharSet, LogFont.lfOutPrecision, LogFont.lfClipPrecision, LogFont.lfQuality, 
                                     LogFont.lfPitchAndFamily, LogFont.lfFaceName);
        }
        else
        {
            // NOTE(kstandbridge): Failed to load system font so just use arial
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
        }
        
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
        s32 Width = (MaxX - MinX) + 1;
        s32 Height = (MaxY - MinY) + 1;
        
        Result.Width = Width + 2;
        Result.Height = Height + 2;
        Result.WidthOverHeight = SafeRatio1((f32)Result.Width, (f32)Result.Height);
        Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
        Result.Memory = PushSize(Arena, Result.Height*Result.Pitch);
        
        memset(Result.Memory, 0, Result.Height*Result.Pitch);
        
        u32 RedMask = 0xff;
        u32 GreenMask = 0xff00;
        u32 BlueMask = 0xff0000;
        u32 AlphaMask = 0xff000000;        
        
        bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
        
        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);
        
        s32 RedShiftDown = (s32)RedScan.Index;
        s32 GreenShiftDown = (s32)GreenScan.Index;
        s32 BlueShiftDown = (s32)BlueScan.Index;
        s32 AlphaShiftDown = (s32)AlphaScan.Index;
        
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
                
                f32 Alpha = 255.0f;
                if(Pixel == 0)
                {
                    Alpha = 0.0f;
                }
                
                v4 Texel = V4((f32)((Pixel & RedMask) >> RedShiftDown),
                              (f32)((Pixel & GreenMask) >> GreenShiftDown),
                              (f32)((Pixel & BlueMask) >> BlueShiftDown),
                              Alpha);
                
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
    
    u32 GlyphIndex = GlobalGlyphIndexFromCodePoint[CodePoint] = ++CurrentGlyphIndex;
    
    INT ThisWidth;
    GetCharWidth32W(FontDeviceContext, CodePoint, CodePoint, &ThisWidth);
    f32 CharAdvance = (f32)ThisWidth;
    
    for(u32 OtherGlyphIndex = 0;
        OtherGlyphIndex < MAX_GLYPH_COUNT;
        ++OtherGlyphIndex)
    {
        GlobalCodePointHoriziontalAdvance[GlyphIndex*MAX_GLYPH_COUNT + OtherGlyphIndex] += CharAdvance - KerningChange;
        if(OtherGlyphIndex != 0)
        {
            //GlobalCodePointHoriziontalAdvance[OtherGlyphIndex*MAX_GLYPH_COUNT + GlyphIndex] += KerningChange;
        }
    }
    
    return Result;
}

internal f32
DEBUGGetHorizontalAdvanceForPair(u32 PrevCodePoint, u32 CodePoint)
{
    u32 PrevGlyphIndex = GlobalGlyphIndexFromCodePoint[PrevCodePoint];
    u32 GlyphIndex = GlobalGlyphIndexFromCodePoint[CodePoint];
    
    f32 Result = GlobalCodePointHoriziontalAdvance[PrevGlyphIndex*MAX_GLYPH_COUNT + GlyphIndex];
    
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
            memcpy(StringCopy, Text.Data, Text.Size * sizeof(char));
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
    
#define MonitorRefreshHz 30
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
            
            GlobalIsRunning = true;
            while(GlobalIsRunning)
            {
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
                
                NewInput->dtForFrame = SecondsElapsedForFrame;
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
                    AppCode.AppUpdateAndRender(&AppMemory, NewInput, &AppBuffer);
                }
                
                HDC DeviceContext = GetDC(WindowHwnd);
                StretchDIBits(DeviceContext, 0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height, 0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height, GlobalBackbuffer.Memory, &GlobalBackbuffer.Info, DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(WindowHwnd, DeviceContext);
                
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