#include "kengine_platform.h"
#include "kengine_memory.h"
#include "kengine_generated.h"
#include "kengine_debug_shared.h"
#include "win32_kengine_types.h"
#include "win32_kengine_opengl.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "kengine_renderer_shared.h"

#ifndef VERSION
#define VERSION 0
#endif // VERSION

#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"
#include "win32_kengine_shared.c"
#include "kengine_telemetry.c"

#if KENGINE_INTERNAL
global debug_event_table GlobalDebugEventTable_;
debug_event_table *GlobalDebugEventTable = &GlobalDebugEventTable_;
#endif

global GLuint GlobalBlitTextureHandle;
global wgl_create_context_attribs_arb *wglCreateContextAttribsARB;
global wgl_choose_pixel_format_arb *wglChoosePixelFormatARB;
global wgl_swap_interval_ext *wglSwapIntervalEXT;
global wgl_get_extensions_string_ext *wglGetExtensionsStringEXT;
global b32 OpenGLSupportsSRGBFramebuffer;
global GLuint OpenGLDefaultInternalTextureFormat;
global app_memory AppMemory;

#if KENGINE_INTERNAL
#include "kengine_sort.c"
#else
#include "kengine.h"
#include "kengine.c"
#endif
#include "kengine_renderer.c"
#include "win32_kengine_opengl.c"

internal LRESULT __stdcall
Win32MainWindowCallback(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CLOSE:
        {
            GlobalWin32State.IsRunning = false;
        } break;
        
        default:
        {
            Result = Win32DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal void
Win32ProcessPendingMessages(app_input *Input)
{
    MSG Message;
    while(Win32PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                __debugbreak();
            } break;
            
            case WM_MOUSEWHEEL:
            {
                Input->MouseZ = GET_WHEEL_DELTA_WPARAM(Message.wParam);
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)Message.wParam;
                
                b32 AltKeyWasDown = (Message.lParam & (1 << 29));
                b32 ShiftKeyWasDown = (Win32GetKeyState(VK_SHIFT) & (1 << 15));
                
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1UL << 31)) == 0);
                if(WasDown != IsDown)
                {
                    // TODO(kstandbridge): Process individual keys?
                }
                
                if(IsDown)
                {
                    if((VKCode == VK_F4) && AltKeyWasDown)
                    {
                        GlobalWin32State.IsRunning = false;
                    }
                    else if((VKCode >= VK_F1) && (VKCode <= VK_F12))
                    {
                        Input->FKeyPressed[VKCode - VK_F1 + 1] = true;
                    }
                }
                
            } break;
            
            default:
            {
                Win32TranslateMessage(&Message);
                Win32DispatchMessageA(&Message);
            } break;
        }
    }
}

internal void
Win32ParseCommandLingArgs()
{
    char *CommandLingArgs = Win32GetCommandLineA();
    Assert(CommandLingArgs);
    
    char *At = CommandLingArgs;
    char *ParamStart = At;
    u32 ParamLength = 0;
    b32 Parsing = true;
    b32 ExeNameFound = false;
    while(Parsing)
    {
        if((*At == '\0') || IsWhitespace(*At))
        {
            if(*At != '\0')
            {
                while(IsWhitespace(*At))
                {
                    ++At;
                }
            }
            else
            {
                Parsing = false;
            }
            
            if(ExeNameFound)
            {            
                string Parameter;
                Parameter.Size = ParamLength;
                Parameter.Data = (u8 *)ParamStart;
                Parameter;
                // TODO(kstandbridge): Handle parameter
            }
            else
            {
                ExeNameFound = true;
                
#if KENGINE_INTERNAL
                if(*ParamStart == '\"')
                {
                    ++ParamStart;
                    ParamLength -= 2;
                }
                char *LastSlash = At - 1;
                while(*LastSlash != '\\')
                {
                    --ParamLength;
                    --LastSlash;
                }
                ++ParamLength;
                Copy(ParamLength, ParamStart, GlobalWin32State.ExeFilePath);
                GlobalWin32State.ExeFilePath[ParamLength] = '\0';
                
                Copy(ParamLength, GlobalWin32State.ExeFilePath, GlobalWin32State.DllFullFilePath);
                AppendCString(GlobalWin32State.DllFullFilePath + ParamLength, "\\kengine.dll");
                
                Copy(ParamLength, GlobalWin32State.ExeFilePath, GlobalWin32State.TempDllFullFilePath);
                AppendCString(GlobalWin32State.TempDllFullFilePath + ParamLength, "\\kengine_temp.dll");
                
                Copy(ParamLength, GlobalWin32State.ExeFilePath, GlobalWin32State.LockFullFilePath);
                AppendCString(GlobalWin32State.LockFullFilePath + ParamLength, "\\lock.tmp");
#endif
                
            }
            
            ParamStart = At;
            ParamLength = 1;
            ++At;
        }
        else
        {
            ++ParamLength;
            ++At;
        }
        
    }
}

#define MAX_GLYPH_COUNT 5000
global KERNINGPAIR *GlobalKerningPairs;
global u32 GlobalKerningPairCount;
global TEXTMETRICA GlobalTextMetric;

global HDC FontDeviceContext;
internal loaded_glyph
Win32GetGlyphForCodePoint(memory_arena *Arena, u32 CodePoint)
{
    
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024
    
    local_persist b32 FontInitialized;
    local_persist void *FontBits;
    
    local_persist HFONT FontHandle;
    
    local_persist u32 CurrentGlyphIndex = 0;
    if(!FontInitialized)
    {
        FontDeviceContext = Win32CreateCompatibleDC(Win32GetDC(0));
        
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
        HBITMAP Bitmap = Win32CreateDIBSection(FontDeviceContext, &Info, DIB_RGB_COLORS, &FontBits, 0, 0);
        Win32SelectObject(FontDeviceContext, Bitmap);
        Win32SetBkColor(FontDeviceContext, RGB(0, 0, 0));
        
#if 1
        Win32AddFontResourceExA("c:/Windows/Fonts/segoeui.ttf", FR_PRIVATE, 0);
        s32 PointSize = 11;
        s32 FontHeight = -Win32MulDiv(PointSize, Win32GetDeviceCaps(FontDeviceContext, LOGPIXELSY), 72);
        FontHandle = Win32CreateFontA(FontHeight, 0, 0, 0,
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
#else
        Win32AddFontResourceExA("c:/Windows/Fonts/LiberationMono-Regular.ttf", FR_PRIVATE, 0);
        FontHandle = Win32CreateFontA(20, 0, 0, 0,
                                      FW_NORMAL, // NOTE(kstandbridge): Weight
                                      FALSE, // NOTE(kstandbridge): Italic
                                      FALSE, // NOTE(kstandbridge): Underline
                                      FALSE, // NOTE(kstandbridge): Strikeout
                                      DEFAULT_CHARSET, 
                                      OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, 
                                      ANTIALIASED_QUALITY,
                                      DEFAULT_PITCH|FF_DONTCARE,
                                      "Liberation Mono");
#endif
        
        Win32SelectObject(FontDeviceContext, FontHandle);
        Win32GetTextMetricsA(FontDeviceContext, &GlobalTextMetric);
        
        GlobalKerningPairCount = Win32GetKerningPairsW(FontDeviceContext, 0, 0);
        GlobalKerningPairs = PushArray(Arena, GlobalKerningPairCount, KERNINGPAIR);
        Win32GetKerningPairsW(FontDeviceContext, GlobalKerningPairCount, GlobalKerningPairs);
        
        FontInitialized = true;
    }
    
    loaded_glyph Result;
    ZeroStruct(Result);
    
    Win32SelectObject(FontDeviceContext, FontHandle);
    
    ZeroSize(MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32), FontBits);
    
    wchar_t CheesePoint = (wchar_t)CodePoint;
    
    SIZE Size;
    Win32GetTextExtentPoint32W(FontDeviceContext, &CheesePoint, 1, &Size);
    
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
    
    //Win32SetBkMode(FontDeviceContext, TRANSPARENT);
    Win32SetTextColor(FontDeviceContext, RGB(255, 255, 255));
    Win32TextOutW(FontDeviceContext, PreStepX, 0, &CheesePoint, 1);
    
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
            
#if KENGINE_SLOW
            COLORREF RefPixel = Win32GetPixel(FontDeviceContext, X, Y);
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
    
    if(MinX <= MaxX)
    {
        s32 Width = (MaxX - MinX) + 1;
        s32 Height = (MaxY - MinY) + 1;
        
        Result.Bitmap.Width = Width + 2;
        Result.Bitmap.Height = Height + 2;
        Result.Bitmap.WidthOverHeight = SafeRatio1((f32)Result.Bitmap.Width, (f32)Result.Bitmap.Height);
        Result.Bitmap.Pitch = Result.Bitmap.Width*BITMAP_BYTES_PER_PIXEL;
        Result.Bitmap.Memory = PushSize(Arena, Result.Bitmap.Height*Result.Bitmap.Pitch);
        
        ZeroSize(Result.Bitmap.Height*Result.Bitmap.Pitch, Result.Bitmap.Memory);
        
        u8 *DestRow = (u8 *)Result.Bitmap.Memory + (Result.Bitmap.Height - 1 - 1)*Result.Bitmap.Pitch;
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
                
#if KENGINE_SLOW
                COLORREF Pixel = Win32GetPixel(FontDeviceContext, X, Y);
                Assert(Pixel == *Source);
#else
                u32 Pixel = *Source;
#endif
                
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
            
            DestRow -= Result.Bitmap.Pitch;
            SourceRow -= MAX_FONT_WIDTH;
        }
        
        Result.Bitmap.AlignPercentage.X = (1.0f) / (f32)Result.Bitmap.Width;
        Result.Bitmap.AlignPercentage.Y = (1.0f + (MaxY - (BoundHeight - GlobalTextMetric.tmDescent))) / (f32)Result.Bitmap.Height;
        
        Result.KerningChange = (f32)(MinX - PreStepX);
        
    }
    
    return Result;
}

internal f32
Win32GetHorizontalAdvance(u32 PrevCodePoint, u32 CodePoint)
{
    s32 ThisWidth;
    Win32GetCharWidth32W(FontDeviceContext, PrevCodePoint, CodePoint, &ThisWidth);
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
Win32GetVerticleAdvance()
{
    f32 Result = (f32)GlobalTextMetric.tmAscent + (f32)GlobalTextMetric.tmDescent + (f32)GlobalTextMetric.tmDescent;
    
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

typedef void win32_http_completion_function(struct win32_http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult);

typedef struct win32_http_io_context
{
    OVERLAPPED Overlapped;
    win32_http_completion_function *CompletionFunction;
    struct win32_http_state *HttpState;
    
} win32_http_io_context;

typedef struct win32_http_io_request
{
    b32 InUse;
    memory_arena Arena;
    
    temporary_memory MemoryFlush;
    
    win32_http_io_context IoContext;
    
    u8 *Buffer;
    HTTP_REQUEST *HttpRequest;
} win32_http_io_request;

typedef struct win32_http_io_response
{
    win32_http_io_context IoContext;
    
    HTTP_RESPONSE HttpResponse;
    
    HTTP_DATA_CHUNK HttpDataChunk;
    
} win32_http_io_response;

typedef struct win32_http_state
{
    HTTPAPI_VERSION Version;
    HTTP_SERVER_SESSION_ID SessionId;
    HTTP_URL_GROUP_ID UrlGroupId;
    HANDLE RequestQueue;
    PTP_IO IoThreadpool;
    
    b32 HttpInit;
    b32 StopServer;
    
    u32 RequestCount;
    u32 volatile NextRequestIndex;
    win32_http_io_request *Requests;
    
    u32 ResponseCount;
    u32 volatile NextResponseIndex;
    win32_http_io_response *Responses;
} win32_http_state;


internal void Win32ReceiveCompletionCallback(win32_http_io_context *IoContext, PTP_IO IoThreadpool, u32 IoResult);
internal void Win32SendCompletionCallback(win32_http_io_context *IoContext, PTP_IO IoThreadpool, u32 IoResult);

inline win32_http_io_request *
Win32SafeGetNextRequest(win32_http_state *HttpState)
{
    win32_http_io_request *Result = 0;
    for(;;)
    {
        u32 OrigionalNext = HttpState->NextRequestIndex;
        u32 NewNext = (OrigionalNext + 1) % HttpState->RequestCount;
        u32 Index = AtomicCompareExchangeU32(&HttpState->NextRequestIndex, NewNext, OrigionalNext);
        if(Index == OrigionalNext)
        {
            Result = HttpState->Requests + Index;
            if(!Result->InUse)
            {
                Result->InUse = true;
                break;
            }
        }
        else
        {
            _mm_pause();
        }
    }
    return Result;
}

inline win32_http_io_request *
Win32AllocateHttpIoRequest(win32_http_state *HttpState)
{
    win32_http_io_request *Result = Win32SafeGetNextRequest(HttpState);
    Result->IoContext.HttpState = HttpState;
    Result->IoContext.CompletionFunction = Win32ReceiveCompletionCallback;
    Result->MemoryFlush = BeginTemporaryMemory(&Result->Arena);
    Result->Buffer = BeginPushSize(Result->MemoryFlush.Arena);
    Result->HttpRequest = (HTTP_REQUEST *)Result->Buffer;
    
    Assert(Result->Arena.TempCount == 2);
    
    return Result;
}

inline win32_http_io_response *
Win32SafeGetNextResponse(win32_http_state *HttpState)
{
    win32_http_io_response *Result = 0;
    for(;;)
    {
        u32 OrigionalNext = HttpState->NextResponseIndex;
        u32 NewNext = (OrigionalNext + 1) % HttpState->ResponseCount;
        u32 Index = AtomicCompareExchangeU32(&HttpState->NextResponseIndex, NewNext, OrigionalNext);
        if(Index == OrigionalNext)
        {
            Result = HttpState->Responses + Index;
            break;
        }
        else
        {
            _mm_pause();
        }
    }
    return Result;
}

internal void
Win32SendCompletionCallback(win32_http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult)
{
    win32_http_io_response *IoResponse = CONTAINING_RECORD(pIoContext, win32_http_io_response, IoContext);
}

inline win32_http_io_response *
Win32AllocateHttpIoResponse(win32_http_state *HttpState)
{
    win32_http_io_response *Result = Win32SafeGetNextResponse(HttpState);
    Result->IoContext.HttpState = HttpState;
    Result->IoContext.CompletionFunction = Win32SendCompletionCallback;
    
    Result->HttpResponse.EntityChunkCount = 1;
    Result->HttpResponse.pEntityChunks = &Result->HttpDataChunk;
    
    char *ContentType = "application/json";
    Result->HttpResponse.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = ContentType;
    Result->HttpResponse.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = GetNullTerminiatedStringLength(ContentType);
    
    return Result;
}

internal void
Win32UnitializeHttpServer(win32_http_state *HttpState)
{
    LogVerbose("Uninitializing HTTP server");
    
    if(HttpState->UrlGroupId != 0)
    {
        u32 Result = Win32HttpCloseUrlGroup(HttpState->UrlGroupId);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseUrlGroup failed with error code %u", Result);
        }
        HttpState->UrlGroupId = 0;
    }
    
    if(HttpState->SessionId != 0)
    {
        u32 Result = Win32HttpCloseServerSession(HttpState->SessionId);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseServerSession failed with error code %u", Result);
        }
        HttpState->SessionId = 0;
    }
    
    if(HttpState->HttpInit)
    {
        u32 Result = Win32HttpTerminate(HTTP_INITIALIZE_SERVER, 0);
        if(Result != NO_ERROR)
        {
            LogError("HttpTerminate failed with error code %u", Result);
        }
        HttpState->HttpInit = false;
    }
}

internal void
Win32IoCallbackThread(PTP_CALLBACK_INSTANCE Instance, PVOID Context, void *pOverlapped, ULONG IoResult, ULONG_PTR BytesTransferred, PTP_IO IoThreadpool)
{
    win32_http_state *HttpState;
    
    win32_http_io_context *IoContext = CONTAINING_RECORD(pOverlapped, win32_http_io_context, Overlapped);
    
    HttpState = IoContext->HttpState;
    
    IoContext->CompletionFunction(IoContext, IoThreadpool, IoResult);
    
}

internal void
Win32UninitializeIoCompletionContext(win32_http_state *HttpState)
{
    LogVerbose("Uninitializing I/O completion context");
    
    if(HttpState->RequestQueue != 0)
    {
        LogVerbose("Closing request queue");
        u32 Result = Win32HttpCloseRequestQueue(HttpState->RequestQueue);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseRequestQueue failed with error code %u", Result);
        }
        HttpState->RequestQueue = 0;
    }
    
    if(HttpState->IoThreadpool != 0)
    {
        LogVerbose("Closing I/O thread pool");
        Win32CloseThreadpoolIo(HttpState->IoThreadpool);
        HttpState->IoThreadpool = 0;
    }
}

inline win32_http_io_response *
Win32CreateMessageResponse(win32_http_state *HttpState, u16 StatusCode, string Reason, string Message)
{
    win32_http_io_response *Result = Win32AllocateHttpIoResponse(HttpState);
    
    if(Result)
    {
        Result->HttpResponse.StatusCode = StatusCode;
        Result->HttpResponse.pReason = (char *)Reason.Data;
        Result->HttpResponse.ReasonLength = Reason.Size;
        
        Result->HttpResponse.pEntityChunks[0].DataChunkType = HttpDataChunkFromMemory;
        Result->HttpResponse.pEntityChunks[0].FromMemory.pBuffer = (char *)Message.Data;
        Result->HttpResponse.pEntityChunks[0].FromMemory.BufferLength = Message.Size;
    }
    else
    {
        LogError("Failed to allocate http io response");
    }
    
    return Result;
}


internal void
Win32ProcessReceiveAndPostResponse(win32_http_io_request *IoRequest, PTP_IO IoThreadpool, u32 IoResult)
{
    Assert(IoRequest->Arena.TempCount == 2);
    win32_http_state *HttpState = IoRequest->IoContext.HttpState;
    win32_http_io_response *IoResponse = 0;
    
    switch(IoResult)
    {
        case NO_ERROR:
        {
            HTTP_REQUEST *Request = IoRequest->HttpRequest;
            umm TotalSize = sizeof(HTTP_REQUEST);
            TotalSize += Request->UnknownVerbLength;
            TotalSize += Request->RawUrlLength;
            TotalSize += Request->CookedUrl.FullUrlLength;
            for(u32 HeaderIndex = 0;
                HeaderIndex < Request->Headers.UnknownHeaderCount;
                ++HeaderIndex)
            {
                TotalSize += sizeof(HTTP_UNKNOWN_HEADER);
                HTTP_UNKNOWN_HEADER *Header = Request->Headers.pUnknownHeaders + HeaderIndex;
                TotalSize += Header->NameLength;
                TotalSize += Header->RawValueLength;
            }
            for(u32 HeaderIndex = 0;
                HeaderIndex < ArrayCount(Request->Headers.KnownHeaders);
                ++HeaderIndex)
            {
                TotalSize += sizeof(HTTP_KNOWN_HEADER);
                HTTP_KNOWN_HEADER *Header = Request->Headers.KnownHeaders + HeaderIndex;
                TotalSize += Header->RawValueLength;
                
            }
            for(u32 EntityChunkIndex = 0;
                EntityChunkIndex < Request->EntityChunkCount;
                ++EntityChunkIndex)
            {
                TotalSize += sizeof(HTTP_DATA_CHUNK);
                HTTP_DATA_CHUNK *DataChunk = Request->pEntityChunks + EntityChunkIndex;
                // TODO(kstandbridge): Other entitiy chunk types?
                Assert(DataChunk->DataChunkType == HttpDataChunkFromMemory);
                TotalSize += DataChunk->FromMemory.BufferLength;
            }
            for(u32 RequestInfoIndex = 0;
                RequestInfoIndex < Request->RequestInfoCount;
                ++RequestInfoIndex)
            {
                TotalSize += sizeof(HTTP_REQUEST_INFO);
                HTTP_REQUEST_INFO *RequestInfo = Request->pRequestInfo + RequestInfoIndex;
                TotalSize += RequestInfo->InfoLength;
            }
            EndPushSize(IoRequest->MemoryFlush.Arena, TotalSize);
            Assert(IoRequest->Arena.TempCount == 1);
            if(GlobalWin32State.AppHandleHttpRequest)
            {
                if(Request->Verb == HttpVerbGET)
                {
                    
                    
                    platform_http_request PlatformRequest;
                    PlatformRequest.Endpoint = String_(Request->RawUrlLength,
                                                       (u8 *)Request->pRawUrl);
                    PlatformRequest.Payload = (Request->EntityChunkCount == 0) 
                        ? String("") : String_(Request->pEntityChunks->FromMemory.BufferLength, 
                                               Request->pEntityChunks->FromMemory.pBuffer);
                    platform_http_response PlatformResponse = GlobalWin32State.AppHandleHttpRequest(&AppMemory, &IoRequest->Arena,
                                                                                                    PlatformRequest);
                    IoResponse = Win32CreateMessageResponse(HttpState, 
                                                            PlatformResponse.Code, 
                                                            String("OK"),  // TODO(kstandbridge): Code to string?
                                                            PlatformResponse.Payload);
                }
                else
                {
                    IoResponse = Win32CreateMessageResponse(HttpState, 501, String("Not Implemented"), 
                                                            String("{ \"Error\": \"Not Implemented\" }"));
                }
            }
            else
            {
                IoResponse = Win32CreateMessageResponse(HttpState, 200, String("OK"), 
                                                        String("{ \"Message\": \"Server starting...\" }"));
            }
        } break;
        
        case ERROR_MORE_DATA:
        {
            EndPushSize(IoRequest->MemoryFlush.Arena, 0);
            IoResponse = Win32CreateMessageResponse(HttpState, 413, String("Payload Too Large"), 
                                                    String("{ \"Error\": \"The request is larger than the server is willing or able to process\" }"));
        } break;
        
        default:
        {
            EndPushSize(IoRequest->MemoryFlush.Arena, 0);
            LogError("HttpReceiveHttpRequest call failed asynchronously with error code %u", IoResult);
        } break;
    }
    
    if(IoResponse != 0)
    {
        Win32StartThreadpoolIo(IoThreadpool);
        
        u32 Result = Win32HttpSendHttpResponse(HttpState->RequestQueue, IoRequest->HttpRequest->RequestId, 0, &IoResponse->HttpResponse,
                                               0, 0, 0, 0, &IoResponse->IoContext.Overlapped, 0);
        
        if((Result != NO_ERROR) &&
           (Result != ERROR_IO_PENDING))
        {
            Win32CancelThreadpoolIo(IoThreadpool);
            
            LogError("HttpSendHttpResponse failed with error code %u", Result);
        }
    }
}

internal void
Win32PostNewReceive(win32_http_state *HttpState, PTP_IO IoThreadpool)
{
    win32_http_io_request *IoRequest = Win32AllocateHttpIoRequest(HttpState);
    Assert(IoRequest->Arena.TempCount == 2);
    
    if(IoRequest != 0)
    {
        Win32StartThreadpoolIo(IoThreadpool);
        
        umm BufferMaxSize = (IoRequest->Arena.CurrentBlock->Size - IoRequest->Arena.CurrentBlock->Used);
        u32 Result = Win32HttpReceiveHttpRequest(HttpState->RequestQueue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                                 IoRequest->HttpRequest, BufferMaxSize, 0,
                                                 &IoRequest->IoContext.Overlapped);
        
        if((Result != ERROR_IO_PENDING) &&
           (Result != NO_ERROR))
        {
            Win32CancelThreadpoolIo(IoThreadpool);
            
            LogError("HttpReceiveHttpRequest failed with error code %u", Result);
            
            if(Result == ERROR_MORE_DATA)
            {
                Win32ProcessReceiveAndPostResponse(IoRequest, IoThreadpool, ERROR_MORE_DATA);
            }
            
            EndPushSize(&IoRequest->Arena, 0);
            Assert(IoRequest->Arena.TempCount == 1);
            EndTemporaryMemory(IoRequest->MemoryFlush);
            Assert(IoRequest->Arena.TempCount == 0);
            CheckArena(&IoRequest->Arena);
            CompletePreviousWritesBeforeFutureWrites;
            IoRequest->InUse = false;
        }
    }
    else
    {
        LogError("Allocation of IoRequest failed");
    }
}

internal void
Win32ReceiveCompletionCallback(win32_http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult)
{
    win32_http_io_request *IoRequest = CONTAINING_RECORD(pIoContext, win32_http_io_request, IoContext);
    Assert(IoRequest->Arena.TempCount == 2);
    
    win32_http_state *HttpState = IoRequest->IoContext.HttpState;
    
    if(HttpState->StopServer == false)
    {
        Win32ProcessReceiveAndPostResponse(IoRequest, IoThreadpool, IoResult);
        
        Win32PostNewReceive(HttpState, IoThreadpool);
    }
    else
    {
        EndPushSize(&IoRequest->Arena, 0);
    }
    
    Assert(IoRequest->Arena.TempCount == 1);
    EndTemporaryMemory(IoRequest->MemoryFlush);
    Assert(IoRequest->Arena.TempCount == 0);
    CheckArena(&IoRequest->Arena);
    CompletePreviousWritesBeforeFutureWrites;
    IoRequest->InUse = false;
}

internal b32
Win32StartHttpServer(win32_http_state *HttpState)
{
    b32 Result = true;
    
    u32 RequestCount = 0.75f*HttpState->RequestCount;
    
    for(; RequestCount > 0;
        --RequestCount)
    {
        win32_http_io_request *IoRequest = Win32AllocateHttpIoRequest(HttpState);
        if(IoRequest)
        {
            Assert(IoRequest->Arena.TempCount == 2);
            Win32StartThreadpoolIo(HttpState->IoThreadpool);
            
            umm BufferMaxSize = (IoRequest->Arena.CurrentBlock->Size - IoRequest->Arena.CurrentBlock->Used);
            Result = Win32HttpReceiveHttpRequest(HttpState->RequestQueue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                                 IoRequest->HttpRequest, BufferMaxSize, 0,
                                                 &IoRequest->IoContext.Overlapped);
            
            if((Result != ERROR_IO_PENDING) &&
               (Result != NO_ERROR))
            {
                Win32CancelThreadpoolIo(HttpState->IoThreadpool);
                
                if(Result == ERROR_MORE_DATA)
                {
                    Win32ProcessReceiveAndPostResponse(IoRequest, HttpState->IoThreadpool, ERROR_MORE_DATA);
                }
                
                Assert(IoRequest->Arena.TempCount == 1);
                EndTemporaryMemory(IoRequest->MemoryFlush);
                Assert(IoRequest->Arena.TempCount == 0);
                CheckArena(&IoRequest->Arena);
                CompletePreviousWritesBeforeFutureWrites;
                IoRequest->InUse = false;
                
                LogError("HttpReceiveHttpRequest failed with error code %u", Result);
                
                Result = false;
            }
        }
        else
        {
            LogError("Allocation of IoRequest %u failed", RequestCount);
            Result = false;
            break;
        }
        
    }
    
    return Result;
}

internal void
Win32StopHttpServer(win32_http_state *HttpState)
{
    LogVerbose("Stopping http server");
    
    if(HttpState->RequestQueue != 0)
    {
        LogVerbose("Shutting down request queue");
        HttpState->StopServer = true;
        u32 Result = Win32HttpShutdownRequestQueue(HttpState->RequestQueue);
        if(Result != NO_ERROR)
        {
            LogError("HttpShutdownRequestQueue failed with error code %u", Result);
        }
    }
    
    if(HttpState->IoThreadpool != 0)
    {
        LogVerbose("Waiting for all IO completion threads to complete");
        b32 CancelPendingCallbacks = false;
        Win32WaitForThreadpoolIoCallbacks(HttpState->IoThreadpool, CancelPendingCallbacks);
    }
}
internal u32
Win32InitializeHttpServer_(win32_http_state *HttpState)
{
    LogVerbose("Initializing HTTP server");
    
    HttpState->Version.HttpApiMajorVersion = 2;
    HttpState->Version.HttpApiMinorVersion = 0;
    
    u32 Result = Win32HttpInitialize(HttpState->Version, HTTP_INITIALIZE_SERVER, 0);
    if(Result == NO_ERROR)
    {
        HttpState->HttpInit = true;
        LogVerbose("Creating server session");
        Result = Win32HttpCreateServerSession(HttpState->Version, &HttpState->SessionId, 0);
        if(Result == NO_ERROR)
        {
            LogVerbose("Creating url group");
            Result = Win32HttpCreateUrlGroup(HttpState->SessionId, &HttpState->UrlGroupId, 0);
            if(Result == NO_ERROR)
            {
                LogVerbose("Adding url to url group");
                Result = Win32HttpAddUrlToUrlGroup(HttpState->UrlGroupId, L"http://localhost:8090/", 0, 0);
                if(Result != NO_ERROR)
                {
                    LogError("HttpAddUrlToUrlGroup failed with error code %u", Result);
                }
            }
            else
            {
                LogError("HttpCreateUrlGroup failed with error code %u", Result);
            }
        }
        else
        {
            LogError("HttpCreateServerSession failed with error code %u", Result);
        }
    }
    else
    {
        LogError("HttpInitialize failed with error code %u", Result);
    }
    
    return Result;
}

internal b32
Win32InitializeIoThreadPool(win32_http_state *HttpState)
{
    LogVerbose("Initializing I/O completion context");
    
    LogVerbose("Creating request queue");
    u32 Result = Win32HttpCreateRequestQueue(HttpState->Version, L"AsyncHttpServer", 0, 0, &HttpState->RequestQueue);
    if(Result == NO_ERROR)
    {
        HTTP_BINDING_INFO HttpBindingInfo;
        ZeroStruct(HttpBindingInfo);
        HttpBindingInfo.Flags.Present = 1;
        HttpBindingInfo.RequestQueueHandle = HttpState->RequestQueue;
        LogVerbose("Setting url group property");
        Result = Win32HttpSetUrlGroupProperty(HttpState->UrlGroupId, HttpServerBindingProperty, &HttpBindingInfo, sizeof(HttpBindingInfo));
        
        if(Result == NO_ERROR)
        {
            LogVerbose("Creating I/O thread pool");
            HttpState->IoThreadpool = Win32CreateThreadpoolIo(HttpState->RequestQueue, Win32IoCallbackThread, 0, 0);
            
            if(HttpState->IoThreadpool == 0)
            {
                LogError("Creating I/O thread pool failed");
                Result = false;
            }
            else
            {
                Result = true;
            }
        }
        else
        {
            LogError("HttpSetUrlGroupProperty failed with error code %u", Result);
        }
    }
    else
    {
        LogError("HttpCreateRequestQueue failed with error code %u", Result);
    }
    
    return Result;
}

void __stdcall 
WinMainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    
    HINSTANCE Instance = Win32GetModuleHandleA(0);
    
    WNDCLASSEXA WindowClass;
    ZeroStruct(WindowClass);
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "KengineWindowClass";
    
    LARGE_INTEGER PerfCountFrequencyResult;
    Win32QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalWin32State.PerfCountFrequency = (s64)PerfCountFrequencyResult.QuadPart;
    
    SYSTEM_INFO SystemInfo;
    Win32GetSystemInfo(&SystemInfo);
    u32 ProcessorCount = SystemInfo.dwNumberOfProcessors;
    
    platform_work_queue PerFrameWorkQueue;
    u32 PerFrameThreadCount = RoundF32ToU32((f32)ProcessorCount*1.5f);
    Win32MakeQueue(&PerFrameWorkQueue, PerFrameThreadCount);
    AppMemory.PerFrameWorkQueue = &PerFrameWorkQueue;
    
    platform_work_queue BackgroundWorkQueue;
    u32 BackgroundThreadCount = RoundF32ToU32((f32)ProcessorCount/2.0f);
    Win32MakeQueue(&BackgroundWorkQueue, BackgroundThreadCount);
    AppMemory.BackgroundWorkQueue = &BackgroundWorkQueue;
    
    AppMemory.PlatformAPI.AddWorkEntry = Win32AddWorkEntry;
    AppMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
    
    AppMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
    AppMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;
    AppMemory.PlatformAPI.GetMemoryStats = Win32GetMemoryStats;
    
    AppMemory.PlatformAPI.GetGlyphForCodePoint = Win32GetGlyphForCodePoint;
    AppMemory.PlatformAPI.GetHorizontalAdvance = Win32GetHorizontalAdvance;
    AppMemory.PlatformAPI.GetVerticleAdvance = Win32GetVerticleAdvance;;
    
#if KENGINE_INTERNAL
    AppMemory.DebugEventTable = GlobalDebugEventTable;
#endif
    
    Platform = AppMemory.PlatformAPI;
    
    Win32ParseCommandLingArgs();
    
#if KENGINE_HTTP
    win32_http_state HttpState_;
    ZeroStruct(HttpState_);
    win32_http_state *HttpState = &HttpState_;
    
#define OUTSTANDING_REQUESTS 16
#define REQUESTS_PER_PROCESSOR 4
    
    
    u16 RequestCount;
    
    HANDLE ProcessHandle = Win32GetCurrentProcess();
    DWORD_PTR ProcessAffintyMask;
    DWORD_PTR SystemAffinityMask;
    if(Win32GetProcessAffinityMask(ProcessHandle, &ProcessAffintyMask, &SystemAffinityMask))
    {
        for(RequestCount = 0;
            ProcessAffintyMask;
            ProcessAffintyMask >>= 1)
        {
            if(ProcessAffintyMask & 0x1)
            {
                ++RequestCount;
            }
        }
        
        RequestCount *= REQUESTS_PER_PROCESSOR;
    }
    else
    {
        LogWarning("Could not get process affinty mask");
        RequestCount = OUTSTANDING_REQUESTS;
    }
    
    LogVerbose("Set request count to %u", RequestCount);
    
    
    HttpState->ResponseCount = RequestCount;
    HttpState->NextResponseIndex = 0;
    HttpState->Responses = PushArray(&GlobalWin32State.Arena, RequestCount, win32_http_io_response);
    
    HttpState->RequestCount = RequestCount;
    HttpState->NextRequestIndex = 0;
    HttpState->Requests = PushArray(&GlobalWin32State.Arena, RequestCount, win32_http_io_request);
    
    u32 Result = Win32InitializeHttpServer_(HttpState);
    if(Result == NO_ERROR)
    {
        if(Win32InitializeIoThreadPool(HttpState))
        {
            if(!Win32StartHttpServer(HttpState))
            {
                LogError("Failed to start server");
            }
        }
        else
        {
            Win32UninitializeIoCompletionContext(HttpState);
        }
    }
    else
    {
        LogError("Failed to initialize http server");
        Win32UnitializeHttpServer(HttpState);
    }
#endif
    
    if(Win32RegisterClassExA(&WindowClass))
    {
        GlobalWin32State.Window = Win32CreateWindowExA(0,
                                                       WindowClass.lpszClassName,
                                                       "kengine",
                                                       WS_OVERLAPPEDWINDOW,
                                                       CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                                       0, 0, Instance, 0);
        if(GlobalWin32State.Window)
        {
            Win32ShowWindow(GlobalWin32State.Window, SW_SHOW);
            HDC OpenGLDC = Win32GetDC(GlobalWin32State.Window);
            HGLRC OpenGLRC = Win32InitOpenGL(OpenGLDC);
            
            u32 PushBufferSize = Megabytes(64);
            void *PushBuffer = Win32VirtualAlloc(0, PushBufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            u32 CurrentSortMemorySize = Kilobytes(64);
            void *SortMemory = Win32VirtualAlloc(0, CurrentSortMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            u32 CurrentClipMemorySize = Kilobytes(64);
            void *ClipMemory = Win32VirtualAlloc(0, CurrentClipMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            LARGE_INTEGER LastCounter = Win32GetWallClock();
            
            app_input Input[2];
            ZeroArray(ArrayCount(Input), Input);
            app_input *NewInput = &Input[0];
            app_input *OldInput = &Input[1];
            
            // NOTE(kstandbridge): Otherwise Sleep will be ignored for requests less than 50? citation needed
            UINT MinSleepPeriod = 1;
            Win32timeBeginPeriod(MinSleepPeriod);
            
#if 0            
            s32 MonitorRefreshHz = 60;
            s32 Win32RefreshRate = Win32GetDeviceCaps(OpenGLDC, VREFRESH);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            
            f32 TargetSecondsPerFrame = 1.0f / MonitorRefreshHz;
#else
            f32 TargetSecondsPerFrame = 1.0f / 20.0f;
#endif
            RECT ClientRect;
            Win32GetClientRect(GlobalWin32State.Window, &ClientRect);
            
            v2i WindowDimensions = V2i((ClientRect.right - ClientRect.left),
                                       (ClientRect.bottom - ClientRect.top));
            
            GlobalWin32State.IsRunning = true;
            while(GlobalWin32State.IsRunning)
            {
                
#if KENGINE_INTERNAL
                b32 DllNeedsToBeReloaded = false;
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(GlobalWin32State.DllFullFilePath);
                if(Win32CompareFileTime(&NewDLLWriteTime, &GlobalWin32State.LastDLLWriteTime) != 0)
                {
                    WIN32_FILE_ATTRIBUTE_DATA Ignored;
                    if(!Win32GetFileAttributesExA(GlobalWin32State.LockFullFilePath, GetFileExInfoStandard, &Ignored))
                    {
                        DllNeedsToBeReloaded = true;
                    }
                }
#endif
                
                BEGIN_BLOCK("ProcessPendingMessages");
                NewInput->dtForFrame = TargetSecondsPerFrame;
                NewInput->MouseZ = 0;
                ZeroStruct(NewInput->FKeyPressed);
                Win32ProcessPendingMessages(NewInput);
                END_BLOCK();
                
                BEGIN_BLOCK("ProcessMouseInput");
                POINT MouseP;
                Win32GetCursorPos(&MouseP);
                Win32ScreenToClient(GlobalWin32State.Window, &MouseP);
                NewInput->MouseX = (f32)MouseP.x;
                NewInput->MouseY = (f32)((WindowDimensions.Y - 1) - MouseP.y);
                
                NewInput->ShiftDown = (Win32GetKeyState(VK_SHIFT) & (1 << 15));
                NewInput->AltDown = (Win32GetKeyState(VK_MENU) & (1 << 15));
                NewInput->ControlDown = (Win32GetKeyState(VK_CONTROL) & (1 << 15));
                
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
                                        Win32GetKeyState(ButtonVKs[ButtonIndex]) & (1 << 15));
                }
                END_BLOCK();
                
                HDC DeviceContext = Win32GetDC(GlobalWin32State.Window);
                
                render_commands Commands_ = BeginRenderCommands(PushBufferSize, PushBuffer, WindowDimensions.X, WindowDimensions.Y);
                render_commands *Commands = &Commands_;
                
#if KENGINE_INTERNAL
                BEGIN_BLOCK("AppUpdateFrame");
                if(GlobalWin32State.AppUpdateFrame)
                {
                    GlobalWin32State.AppUpdateFrame(&AppMemory, Commands, &GlobalWin32State.Arena, NewInput);
                }
                END_BLOCK();
                if(DllNeedsToBeReloaded)
                {
                    Win32CompleteAllWork(AppMemory.PerFrameWorkQueue);
                    Win32CompleteAllWork(AppMemory.BackgroundWorkQueue);
                    SetDebugEventRecording(false);
                }
                
                if(GlobalWin32State.DebugUpdateFrame)
                {
                    GlobalWin32State.DebugUpdateFrame(&AppMemory, Commands, &GlobalWin32State.Arena, NewInput);
                }
                
                if(DllNeedsToBeReloaded)
                {
                    if(GlobalWin32State.AppLibrary && !Win32FreeLibrary(GlobalWin32State.AppLibrary))
                    {
                        // TODO(kstandbridge): Error freeing app library
                        InvalidCodePath;
                    }
                    GlobalWin32State.AppLibrary = 0;
                    GlobalWin32State.AppUpdateFrame = 0;
                    GlobalWin32State.DebugUpdateFrame = 0;
                    GlobalWin32State.AppHandleHttpRequest = 0;
                    
                    if(Win32CopyFileA(GlobalWin32State.DllFullFilePath, GlobalWin32State.TempDllFullFilePath, false))
                    {
                        GlobalWin32State.AppLibrary = Win32LoadLibraryA(GlobalWin32State.TempDllFullFilePath);
                        if(GlobalWin32State.AppLibrary)
                        {
                            GlobalWin32State.AppUpdateFrame = (app_update_frame *)Win32GetProcAddressA(GlobalWin32State.AppLibrary, "AppUpdateFrame");
                            Assert(GlobalWin32State.AppUpdateFrame);
                            GlobalWin32State.DebugUpdateFrame = (debug_update_frame *)Win32GetProcAddressA(GlobalWin32State.AppLibrary, "DebugUpdateFrame");
                            Assert(GlobalWin32State.DebugUpdateFrame);
                            GlobalWin32State.AppHandleHttpRequest = (app_handle_http_request *)Win32GetProcAddressA(GlobalWin32State.AppLibrary, "AppHandleHttpRequest");
                            Assert(GlobalWin32State.AppHandleHttpRequest);
                            
                            GlobalWin32State.LastDLLWriteTime = NewDLLWriteTime;
                        }
                    }
                    else
                    {
                        // TODO(kstandbridge): Error copying temp dll
                        InvalidCodePath;
                    }
                    SetDebugEventRecording(true);
                }
                
#else
                AppUpdateFrame(&AppMemory, Commands, &GlobalWin32State.Arena, NewInput);
#endif
                
                BEGIN_BLOCK("ExpandRenderStorage");
                u32 NeededSortMemorySize = Commands->PushBufferElementCount * sizeof(sort_entry);
                if(CurrentSortMemorySize < NeededSortMemorySize)
                {
                    Win32VirtualFree(SortMemory, 0, MEM_RELEASE);
                    while(CurrentSortMemorySize < NeededSortMemorySize)
                    {
                        CurrentSortMemorySize *= 2;
                    }
                    SortMemory = Win32VirtualAlloc(0, CurrentSortMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                }
                
                // TODO(kstandbridge): Can we merge sort/clip and push buffer memory together?
                u32 NeededClipMemorySize = Commands->PushBufferElementCount * sizeof(sort_entry);
                if(CurrentClipMemorySize < NeededClipMemorySize)
                {
                    Win32VirtualFree(ClipMemory, 0, MEM_RELEASE);
                    while(CurrentClipMemorySize < NeededClipMemorySize)
                    {
                        CurrentClipMemorySize *= 2;
                    }
                    ClipMemory = Win32VirtualAlloc(0, CurrentClipMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                }
                END_BLOCK();
                
                BEGIN_BLOCK("Render");
                SortRenderCommands(Commands, SortMemory);
                LinearizeClipRects(Commands, ClipMemory);
                
                Win32OpenGLRenderCommands(Commands);
                Win32SwapBuffers(DeviceContext);
                
                EndRenderCommands(Commands);
                
                if(!Win32ReleaseDC(GlobalWin32State.Window, DeviceContext))
                {
                    InvalidCodePath;
                }
                END_BLOCK();
                Win32CompleteAllWork(AppMemory.PerFrameWorkQueue);
                
                app_input *Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
                
                BEGIN_BLOCK("FrameWait");
                f32 FrameSeconds = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock(), GlobalWin32State.PerfCountFrequency);
                
                if(FrameSeconds < TargetSecondsPerFrame)
                {
                    DWORD Miliseconds = (DWORD)(1000.0f * (TargetSecondsPerFrame - FrameSeconds));
                    if(Miliseconds > 0)
                    {
                        Win32Sleep(Miliseconds);
                    }
                    
                    FrameSeconds = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock(), GlobalWin32State.PerfCountFrequency);
                    
                    // NOTE(kstandbridge): FINE I'll make my own sleep function, with blackjack and hookers!
                    while(FrameSeconds < TargetSecondsPerFrame)
                    {
                        FrameSeconds = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock(), GlobalWin32State.PerfCountFrequency);
                        _mm_pause();
                    }
                }
                END_BLOCK();
                
                LARGE_INTEGER ThisCounter = Win32GetWallClock();                
                f32 MeasuredSecondsPerFrame = Win32GetSecondsElapsed(LastCounter, ThisCounter, GlobalWin32State.PerfCountFrequency);
                DEBUG_FRAME_END(MeasuredSecondsPerFrame);
                LastCounter = ThisCounter;
                
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
    
#if KENGINE_HTTP
    Win32StopHttpServer(HttpState);
    Win32UnitializeHttpServer(HttpState);
    Win32UninitializeIoCompletionContext(HttpState);
#endif
    
    Win32ExitProcess(0);
    
    InvalidCodePath;
}