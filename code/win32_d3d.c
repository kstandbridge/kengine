#include "win32_d3d.h"

#ifndef VERSION
#define VERSION 0
#endif // VERSION

#include "kengine_debug_shared.h"
#if KENGINE_INTERNAL
#include "kengine_debug.h"
global debug_event_table GlobalDebugEventTable_;
debug_event_table *GlobalDebugEventTable = &GlobalDebugEventTable_;
#include "kengine_debug.c"
#endif

#include "kengine_telemetry.c"
#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"

global app_memory GlobalAppMemory;

#include "win32_kengine_shared.c"

#include "VertexShader.h"
#include "PixelShader.h"

internal LRESULT CALLBACK
Win32WndProc_(win32_window_state *WindowState, HWND WindowHwnd, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CLOSE:
        {
            Win32PostQuitMessage(0);
        } break;
        
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if(!(LParam & 0x40000000))
            {
                u8 Char = (u8)WParam;
                LogDebug("KeyDown: %u");
            }
        } break;
        
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            u8 Char = (u8)WParam;
            LogDebug("KeyUp: %u");
        } break;
        
        case WM_CHAR:
        {
            u8 Char = (u8)WParam;
            LogDebug("Char: %u");
        } break;
        
        case WM_MOUSEMOVE:
        {
            POINTS Point = MAKEPOINTS(LParam);
            WindowState->MouseP = V2((f32)Point.x, (f32)Point.y);
            u8 TitleBuffer[256];
            
            if((Point.x >= 0) &&
               (Point.x < WindowState->Width) &&
               (Point.y >= 0) &&
               (Point.y < WindowState->Height))
            {
                string Title = FormatStringToBuffer(TitleBuffer, sizeof(TitleBuffer),
                                                    "Mouse Position inside window: (%d, %d)", Point.x, Point.y);
                Title.Data[Title.Size] = '\0';
                Win32SetWindowTextA(WindowHwnd, (LPCSTR)Title.Data);
                if(!WindowState->MouseInWindow)
                {
                    Win32SetCapture(WindowHwnd);
                    WindowState->MouseInWindow = true;
                    LogDebug("Mouse entered window");
                }
            }
            else
            {
                if(WParam & (MK_LBUTTON | MK_RBUTTON))
                {
                    string Title = FormatStringToBuffer(TitleBuffer, sizeof(TitleBuffer),
                                                        "Mouse Position outside window: (%d, %d)", Point.x, Point.y);
                    Title.Data[Title.Size] = '\0';
                    Win32SetWindowTextA(WindowHwnd, (LPCSTR)Title.Data);
                }
                else
                {
                    Win32ReleaseCapture();
                    WindowState->MouseInWindow = false;
                    LogDebug("Left window");
                }
            }
            
        } break;
        
        case WM_LBUTTONDOWN:
        {
            POINTS Point = MAKEPOINTS(LParam);
            LogDebug("MouseLeftDownAt: %d, %d", Point.x, Point.y);
        } break;
        
        case WM_LBUTTONUP:
        {
            POINTS Point = MAKEPOINTS(LParam);
            LogDebug("MouseLeftUpAt: %d, %d", Point.x, Point.y);
        } break;
        
        case WM_RBUTTONDOWN:
        {
            POINTS Point = MAKEPOINTS(LParam);
            LogDebug("MouseRightDownAt: %d, %d", Point.x, Point.y);
        } break;
        
        case WM_RBUTTONUP:
        {
            POINTS Point = MAKEPOINTS(LParam);
            LogDebug("MouseRightUpAt: %d, %d", Point.x, Point.y);
        } break;
        
        case WM_MOUSEWHEEL:
        {
            POINTS Point = MAKEPOINTS(LParam);
            s32 WheelDelta = GET_WHEEL_DELTA_WPARAM(WParam);
            if(WheelDelta > 0)
            {
                LogDebug("MouseWheelUpAt: %d, %d Delta: %d", Point.x, Point.y, WheelDelta);
            }
            else if(WheelDelta < 0)
            {
                LogDebug("MouseWheelDownAt: %d, %d Delta: %d", Point.x, Point.y, WheelDelta);
            }
        } break;
        
        case WM_KILLFOCUS:
        {
            LogDebug("Window lost focus");
        } break;
        
        default:
        {
            Result = Win32DefWindowProcA(WindowHwnd, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal LRESULT CALLBACK
Win32WndProc(HWND WindowHwnd, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result;
    
    win32_window_state *WindowState = (win32_window_state *)Win32GetWindowLongPtrA(WindowHwnd, GWLP_USERDATA);
    
    if(WindowState)
    {
        Result = Win32WndProc_(WindowState, WindowHwnd, Message, WParam, LParam);
    }
    else
    {
        Result = Win32DefWindowProcA(WindowHwnd, Message, WParam, LParam);
    }
    
    return Result;
}

internal LRESULT CALLBACK
Win32WndProcSetup(HWND WindowHwnd, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result;
    
    win32_window_state *WindowState = 0;
    if(Message == WM_NCCREATE)
    {
        CREATESTRUCTA *Create = (CREATESTRUCTA *)LParam;
        WindowState = (win32_window_state *)Create->lpCreateParams;
        Win32SetWindowLongPtrA(WindowHwnd, GWLP_USERDATA, (LONG_PTR)WindowState);
        Win32SetWindowLongPtrA(WindowHwnd, GWLP_WNDPROC, (LONG_PTR)Win32WndProc);
        
        Result = Win32WndProc_(WindowState, WindowHwnd, Message, WParam, LParam);
    }
    else
    {
        Result = Win32DefWindowProcA(WindowHwnd, Message, WParam, LParam);
    }
    
    return Result;
}

inline b32
Win32WindowStateInit(win32_window_state *WindowState, HINSTANCE Instance, char *ClassName)
{
    b32 Result = false;
    
    WindowState->Instance = Instance;
    WindowState->ClassName = ClassName;
    
    LogDebug("Registering window class");
    WNDCLASSEXA WindowClass;
    ZeroStruct(WindowClass);
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_OWNDC;
    WindowClass.lpfnWndProc = Win32WndProcSetup;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = WindowState->Instance;
    WindowClass.hIcon = Win32LoadIconA(WindowState->Instance, MAKEINTRESOURCE(IDI_ICON));
    WindowClass.hCursor = 0;
    WindowClass.hbrBackground = 0;
    WindowClass.lpszMenuName = 0;
    WindowClass.lpszClassName = WindowState->ClassName;
    WindowClass.hIconSm = WindowClass.hIcon;
    if(Win32RegisterClassExA(&WindowClass))
    {
        Result = true;
    }
    else
    {
        Win32LogError("Failed to register window class");
    }
    
    return Result;
}

inline b32
Win32CreateWindow(win32_window_state *WindowState, s32 Width, s32 Height, char *Title)
{
    b32 Result = false;
    
    DWORD Style = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
    
    RECT Rect;
    Rect.left = 100;
    Rect.right = Width + Rect.left;
    Rect.top = 100;
    Rect.bottom = Height + Rect.top;
    
    if(!Win32AdjustWindowRect(&Rect, Style, FALSE))
    {
        Win32LogError("Failed to adjust window rect");
    }
    
    WindowState->Hwnd = Win32CreateWindowExA(0, WindowState->ClassName, Title, Style,
                                             CW_USEDEFAULT, CW_USEDEFAULT, 
                                             Rect.right - Rect.left, Rect.bottom - Rect.top, 
                                             0, 0, WindowState->Instance, WindowState);
    WindowState->Width = Width;
    WindowState->Height = Height;
    
    if(WindowState->Hwnd)
    {
        Win32ShowWindow(WindowState->Hwnd, SW_SHOWDEFAULT);
        Result = true;
    }
    else
    {
        Win32LogError("Failed to create window");
    }
    
    return Result;
}

internal b32
Win32InitDirect3D(win32_window_state *WindowState)
{
    LogDebug("Creating D3D11 device and swap chain");
    DXGI_SWAP_CHAIN_DESC SwapChainDesc;
    ZeroStruct(SwapChainDesc);
    SwapChainDesc.BufferDesc.Width = 0;
    SwapChainDesc.BufferDesc.Height = 0;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
    SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = 1;
    SwapChainDesc.OutputWindow = WindowState->Hwnd;
    SwapChainDesc.Windowed = TRUE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    SwapChainDesc.Flags = 0;
    
    HRESULT HResult;
    
#if KENGINE_INTERNAL
    u32 Flags = D3D11_CREATE_DEVICE_DEBUG;
#else
    u32 Flags = 0;
#endif
    
    if(FAILED(HResult = Win32D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, Flags, 
                                                           0, 0, D3D11_SDK_VERSION, &SwapChainDesc, 
                                                           &WindowState->SwapChain, &WindowState->Device, 0, &WindowState->DeviceContext)))
    {
        Win32LogError_(HResult, "Failed to create device and swap chain");
    }
    else
    {    
        LogDebug("Creating D3D11 render target view");
        ID3D11Resource *BackBuffer;
        if(FAILED(HResult = IDXGISwapChain_GetBuffer(WindowState->SwapChain, 0, &IID_ID3D11Resource, &BackBuffer)))
        {
            Win32LogError_(HResult, "Failed to get buffer from swap chain");
        }
        else
        {
            if(SUCCEEDED(HResult = ID3D11Device_CreateRenderTargetView(WindowState->Device, BackBuffer, 0, &WindowState->RenderTargetView)))
            {
                LogDebug("Creating D3D11 depth stencil state");
                D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
                ZeroStruct(DepthStencilDesc);
                DepthStencilDesc.DepthEnable = TRUE;
                DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
                
                ID3D11DepthStencilState *DepthStencilState;
                ID3D11Device_CreateDepthStencilState(WindowState->Device, &DepthStencilDesc, &DepthStencilState);
                
                LogDebug("Setting D3D11 depth stencil state");
                ID3D11DeviceContext_OMSetDepthStencilState(WindowState->DeviceContext, DepthStencilState, 1);
                
                LogDebug("Creating D3D11 depth stencil texture");
                D3D11_TEXTURE2D_DESC DepthDesc;
                ZeroStruct(DepthDesc);
                DepthDesc.Width = 800u;
                DepthDesc.Height = 600u;
                DepthDesc.MipLevels = 1u;
                DepthDesc.ArraySize = 1u;
                DepthDesc.Format = DXGI_FORMAT_D32_FLOAT;
                DepthDesc.SampleDesc.Count = 1u;
                DepthDesc.SampleDesc.Quality = 0u;
                DepthDesc.Usage = D3D11_USAGE_DEFAULT;
                DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                
                ID3D11Texture2D *DepthStencil;
                ID3D11Device_CreateTexture2D(WindowState->Device, &DepthDesc, 0, &DepthStencil);
                
                LogDebug("Creating D3D11 depth stencil view");
                D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc;
                ZeroStruct(DepthStencilViewDesc);
                DepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
                DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                DepthStencilViewDesc.Texture2D.MipSlice = 0u;
                ID3D11Device_CreateDepthStencilView(WindowState->Device, (ID3D11Resource *)DepthStencil, 
                                                    &DepthStencilViewDesc, &WindowState->DepthStencilView);
                
                LogDebug("Binding D3D11 depth stencil view to OM");
                ID3D11DeviceContext_OMSetRenderTargets(WindowState->DeviceContext, 1u, &WindowState->RenderTargetView, WindowState->DepthStencilView);
            }
            else
            {
                Win32LogError_(HResult, "Failed to create render target view");
            }
            ID3D11Resource_Release(BackBuffer);
        }
    }
    
    b32 Result = (HResult == 0);
    return Result;
}

internal void
Win32CleanupDirect3D(win32_window_state *WindowState)
{
    if(WindowState->RenderTargetView)
    {
        LogDebug("Cleaning up D3D11 render target view");
        ID3D11RenderTargetView_Release(WindowState->RenderTargetView);
    }
    
    if(WindowState->DeviceContext)
    {
        LogDebug("Cleaning up D3D11 device context");
        ID3D11DeviceContext_Release(WindowState->DeviceContext);
    }
    
    if(WindowState->SwapChain)
    {
        LogDebug("Cleaning up D3D11 swap chain");
        IDXGISwapChain_Release(WindowState->SwapChain);
    }
    
    if(WindowState->Device)
    {
        LogDebug("Cleaning up D3D11 device");
        ID3D11Device_Release(WindowState->Device);
    }
}

internal void
Win32EndFrame(win32_window_state *WindowState)
{
    HRESULT HResult;
    if(FAILED(HResult = IDXGISwapChain_Present(WindowState->SwapChain, 1, 0)))
    {
        if(HResult == DXGI_ERROR_DEVICE_REMOVED)
        {
            HResult = ID3D11Device_GetDeviceRemovedReason(WindowState->Device);
        }
        Win32LogError_(HResult, "Failed to present frame");
    }
}

internal void
Win32ClearBuffer(win32_window_state *WindowState, v4 Color)
{
    f32 Color_[4] = { Color.R, Color.G, Color.B, Color.A };
    
    ID3D11DeviceContext_ClearRenderTargetView(WindowState->DeviceContext,
                                              WindowState->RenderTargetView,
                                              Color_);
    ID3D11DeviceContext_ClearDepthStencilView(WindowState->DeviceContext,
                                              WindowState->DepthStencilView,
                                              D3D11_CLEAR_DEPTH, 1.0f, 0u);
}

typedef struct vertex
{
    v3 Position;
} vertex;

typedef struct m4x4
{
    f32 E[4][4];
} m4x4;

inline m4x4
M4x4RotationZ(f32 Angle)
{
    f32 CosAngle = Cos(Angle);
    f32 SinAngle = Sin(Angle);
    
    m4x4 Result;
    
    Result.E[0][0] = CosAngle;
    Result.E[0][1] = SinAngle;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = -SinAngle;
    Result.E[1][1] = CosAngle;
    Result.E[1][2] = 0.0f;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = 0.0f;
    Result.E[2][2] = 1.0f;
    Result.E[2][3] = 0.0f;
    
    Result.E[3][0] = 0.0f;
    Result.E[3][1] = 0.0f;
    Result.E[3][2] = 0.0f;
    Result.E[3][3] = 1.0f;
    
    return Result;
}

inline m4x4
M4x4RotationX(f32 Angle)
{
    f32 SinAngle = Sin(Angle);
    f32 CosAngle = Cos(Angle);
    
    m4x4 Result;
    
    Result.E[0][0] = 1.0f;
    Result.E[0][1] = 0.0f;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = 0.0f;
    Result.E[1][1] = CosAngle;
    Result.E[1][2] = SinAngle;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = -SinAngle;
    Result.E[2][2] = CosAngle;
    Result.E[2][3] = 0.0f;
    
    Result.E[3][0] = 0.0f;
    Result.E[3][1] = 0.0f;
    Result.E[3][2] = 0.0f;
    Result.E[3][3] = 1.0f;
    
    return Result;
}

inline m4x4
M4x4Multiply(m4x4 M1, m4x4 M2)
{
#if 0
    m4x4 Result;
    ZeroStruct(Result);
    
    for(s32 Row = 0; Row <= 3; ++Row) // NOTE(kstandbridge): Rows of A
    {
        for(s32 Column = 0; Column <= 3; ++Column) // NOTE(kstandbridge): Column of B
        {
            for(s32 Index = 0; Index <= 3; ++Index) // NOTE(kstandbridge): Columns of A, Rows of B
            {
                Result.E[Column][Row] += A.E[Row][Index]*B.E[Index][Column];
            }
        }
    }
    
    return Result;
#else
    m4x4 Result;
    // Cache the invariants in registers
    f32 x = M1.E[0][0];
    f32 y = M1.E[0][1];
    f32 z = M1.E[0][2];
    f32 w = M1.E[0][3];
    // Perform the operation on the first row
    Result.E[0][0] = (M2.E[0][0] * x) + (M2.E[1][0] * y) + (M2.E[2][0] * z) + (M2.E[3][0] * w);
    Result.E[0][1] = (M2.E[0][1] * x) + (M2.E[1][1] * y) + (M2.E[2][1] * z) + (M2.E[3][1] * w);
    Result.E[0][2] = (M2.E[0][2] * x) + (M2.E[1][2] * y) + (M2.E[2][2] * z) + (M2.E[3][2] * w);
    Result.E[0][3] = (M2.E[0][3] * x) + (M2.E[1][3] * y) + (M2.E[2][3] * z) + (M2.E[3][3] * w);
    // Repeat for all the other rows
    x = M1.E[1][0];
    y = M1.E[1][1];
    z = M1.E[1][2];
    w = M1.E[1][3];
    Result.E[1][0] = (M2.E[0][0] * x) + (M2.E[1][0] * y) + (M2.E[2][0] * z) + (M2.E[3][0] * w);
    Result.E[1][1] = (M2.E[0][1] * x) + (M2.E[1][1] * y) + (M2.E[2][1] * z) + (M2.E[3][1] * w);
    Result.E[1][2] = (M2.E[0][2] * x) + (M2.E[1][2] * y) + (M2.E[2][2] * z) + (M2.E[3][2] * w);
    Result.E[1][3] = (M2.E[0][3] * x) + (M2.E[1][3] * y) + (M2.E[2][3] * z) + (M2.E[3][3] * w);
    x = M1.E[2][0];
    y = M1.E[2][1];
    z = M1.E[2][2];
    w = M1.E[2][3];
    Result.E[2][0] = (M2.E[0][0] * x) + (M2.E[1][0] * y) + (M2.E[2][0] * z) + (M2.E[3][0] * w);
    Result.E[2][1] = (M2.E[0][1] * x) + (M2.E[1][1] * y) + (M2.E[2][1] * z) + (M2.E[3][1] * w);
    Result.E[2][2] = (M2.E[0][2] * x) + (M2.E[1][2] * y) + (M2.E[2][2] * z) + (M2.E[3][2] * w);
    Result.E[2][3] = (M2.E[0][3] * x) + (M2.E[1][3] * y) + (M2.E[2][3] * z) + (M2.E[3][3] * w);
    x = M1.E[3][0];
    y = M1.E[3][1];
    z = M1.E[3][2];
    w = M1.E[3][3];
    Result.E[3][0] = (M2.E[0][0] * x) + (M2.E[1][0] * y) + (M2.E[2][0] * z) + (M2.E[3][0] * w);
    Result.E[3][1] = (M2.E[0][1] * x) + (M2.E[1][1] * y) + (M2.E[2][1] * z) + (M2.E[3][1] * w);
    Result.E[3][2] = (M2.E[0][2] * x) + (M2.E[1][2] * y) + (M2.E[2][2] * z) + (M2.E[3][2] * w);
    Result.E[3][3] = (M2.E[0][3] * x) + (M2.E[1][3] * y) + (M2.E[2][3] * z) + (M2.E[3][3] * w);
    return Result;
#endif
    
}
inline m4x4
M4x4Scale(v3 A)
{
    m4x4 Result = 
    { 
        A.X,  0,   0,   0,
        0,  A.Y,   0,   0,
        0,    0, A.Z,   0,
        0,    0,   0,   1
    };
    
    return Result;
}

inline m4x4
M4x4Translation(v3 A)
{
#if 0
    m4x4 Result = 
    { 
        1, 0, 0, A.X,
        0, 1, 0, A.Y,
        0, 0, 1, A.Z,
        0, 0, 0,   1
    };
#else
    m4x4 Result ;
    
    Result.E[0][0] = 1.0f;
    Result.E[0][1] = 0.0f;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = 0.0f;
    Result.E[1][1] = 1.0f;
    Result.E[1][2] = 0.0f;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = 0.0f;
    Result.E[2][2] = 1.0f;
    Result.E[2][3] = 0.0f;
    
    Result.E[3][0] = A.X;
    Result.E[3][1] = A.Y;
    Result.E[3][2] = A.Z;
    Result.E[3][3] = 1.0f;
#endif
    return Result;
}

inline m4x4
M4x4Identity()
{
    m4x4 Result = 
    { 
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    return Result;
}

inline m4x4
M4x4Transpose(m4x4 A)
{
#if 0
    m4x4 Result;
    
    for(s32 Row = 0; Row <= 3; ++Row)
    {
        for(s32 Column = 0; Column <= 3; ++Column)
        {
            Result.E[Row][Column] = A.E[Column][Row];
        }
    }
#else
    m4x4 Result = 
    {
        A.E[0][0], A.E[1][0], A.E[2][0], A.E[3][0],
        A.E[0][1], A.E[1][1], A.E[2][1], A.E[3][1],
        A.E[0][2], A.E[1][2], A.E[2][2], A.E[3][2],
        A.E[0][3], A.E[1][3], A.E[2][3], A.E[3][3],
    };
#endif
    
    return Result;
}

typedef struct constant_buffer
{
    m4x4 Transform;
} constant_buffer;

typedef struct constant_buffer_2
{
    v4 FaceColors[6];
} constant_buffer_2;

inline m4x4
M4x4PerspectiveLH(f32 ViewWidth, f32 ViewHeight, f32 NearZ, f32 FarZ)
{
    f32 TwoNearZ = NearZ + NearZ;
    f32 Range = FarZ / (FarZ - NearZ);
    
    m4x4 Result;
    
    Result.E[0][0] = TwoNearZ / ViewWidth;
    Result.E[0][1] = 0.0f;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = 0.0f;
    Result.E[1][1] = TwoNearZ / ViewHeight;
    Result.E[1][2] = 0.0f;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = 0.0f;
    Result.E[2][2] = Range;
    Result.E[2][3] = 1.0f;
    
    Result.E[3][0] = 0.0f;
    Result.E[3][1] = 0.0f;
    Result.E[3][2] = -Range * NearZ;
    Result.E[3][3] = 0.0f;
    
    return Result;
}

internal void
Win32DrawTestTriangle(win32_window_state *WindowState, f32 Angle, f32 X, f32 Z)
{
    ID3D11Device *Device = WindowState->Device;
    IDXGISwapChain *SwapChain = WindowState->SwapChain;
    ID3D11DeviceContext *DeviceContext = WindowState->DeviceContext;
    ID3D11RenderTargetView *RenderTargetView = WindowState->RenderTargetView;
    
    vertex Vertices[] =
    {
        { -1.0f,-1.0f,-1.0f },
        {  1.0f,-1.0f,-1.0f },
        { -1.0f, 1.0f,-1.0f },
        {  1.0f, 1.0f,-1.0f },
        { -1.0f,-1.0f, 1.0f },
        {  1.0f,-1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f },
        {  1.0f, 1.0f, 1.0f },
    };
    
    D3D11_BUFFER_DESC BufferDesc;
    ZeroStruct(BufferDesc);
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.CPUAccessFlags = 0;
    BufferDesc.MiscFlags = 0;
    BufferDesc.ByteWidth = sizeof(Vertices);
    BufferDesc.StructureByteStride = sizeof(vertex);
    
    D3D11_SUBRESOURCE_DATA SubresourceData;
    ZeroStruct(SubresourceData);
    SubresourceData.pSysMem = Vertices;
    
    ID3D11Buffer *VertexBuffer;
    HRESULT HResult;
    if(SUCCEEDED(HResult = ID3D11Device_CreateBuffer(Device, &BufferDesc, &SubresourceData, &VertexBuffer)))
    {
        u32 Stride = sizeof(vertex);
        u32 Offset = 0;
        
        u16 Indicies[] =
        {
            0,2,1, 2,3,1,
            1,3,5, 3,7,5,
            2,6,3, 3,6,7,
            4,5,7, 4,7,6,
            0,4,2, 2,4,6,
            0,1,4, 1,5,4
        };
        
        ID3D11Buffer *IndexBuffer;
        ZeroStruct(BufferDesc);
        BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
        BufferDesc.CPUAccessFlags = 0;
        BufferDesc.MiscFlags = 0;
        BufferDesc.ByteWidth = sizeof(Indicies);
        BufferDesc.StructureByteStride = sizeof(u16);
        
        ZeroStruct(SubresourceData);
        SubresourceData.pSysMem = Indicies;
        
        ID3D11DeviceContext_IASetVertexBuffers(DeviceContext, 0, 1, &VertexBuffer, &Stride, &Offset);
        ID3D11VertexShader *VertexShader;
        u32 VertexShaderBlobSize = sizeof(VertexShaderBlob);
        
        if(SUCCEEDED(HResult = ID3D11Device_CreateBuffer(Device, &BufferDesc, &SubresourceData, &IndexBuffer)))
        {
            ID3D11DeviceContext_IASetIndexBuffer(DeviceContext, IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
            
            if(SUCCEEDED(HResult = ID3D11Device_CreateVertexShader(Device, VertexShaderBlob, VertexShaderBlobSize, 0, &VertexShader)))
            {
                ID3D11DeviceContext_VSSetShader(DeviceContext, VertexShader, 0, 0);
                
                ID3D11InputLayout *InputLayout;
                D3D11_INPUT_ELEMENT_DESC InputElementDesc[] =
                {
                    { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
                };
                if(SUCCEEDED(HResult = ID3D11Device_CreateInputLayout(Device, InputElementDesc, ArrayCount(InputElementDesc),
                                                                      VertexShaderBlob, VertexShaderBlobSize, &InputLayout)))
                {
                    ID3D11DeviceContext_IASetInputLayout(DeviceContext, InputLayout);
                    
                    m4x4 Transform = M4x4Identity();
                    Transform = M4x4Multiply(Transform, M4x4RotationZ(Angle));
                    Transform = M4x4Multiply(Transform, M4x4RotationX(Angle));
                    Transform = M4x4Multiply(Transform, M4x4Translation(V3(X, 0.0f, Z + 4.0f)));
                    Transform = M4x4Multiply(Transform, M4x4PerspectiveLH(1.0f, 3.0f / 4.0f, 0.5f, 10.0f));
                    
                    Transform = M4x4Transpose(Transform);
                    
                    constant_buffer ConstantBufferData = 
                    {
                        Transform
                    };
                    
                    ZeroStruct(BufferDesc);
                    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
                    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    BufferDesc.MiscFlags = 0;
                    BufferDesc.ByteWidth = sizeof(ConstantBufferData);
                    BufferDesc.StructureByteStride = 0;
                    
                    ZeroStruct(SubresourceData);
                    SubresourceData.pSysMem = &ConstantBufferData;
                    
                    ID3D11Buffer *ConstantBuffer;
                    ID3D11Device_CreateBuffer(Device, &BufferDesc, &SubresourceData, &ConstantBuffer);
                    ID3D11DeviceContext_VSSetConstantBuffers(DeviceContext, 0, 1, &ConstantBuffer);
                    
                    
                    constant_buffer_2 ConstantBuffer2Data =
                    {
                        V4(1.0f,0.0f,1.0f,1.0f),
                        V4(1.0f,0.0f,0.0f,1.0f),
                        V4(0.0f,1.0f,0.0f,1.0f),
                        V4(0.0f,0.0f,1.0f,1.0f),
                        V4(1.0f,1.0f,0.0f,1.0f),
                        V4(0.0f,1.0f,0.0f,1.0f),
                    };
                    BufferDesc.ByteWidth = sizeof(ConstantBuffer2Data);
                    SubresourceData.pSysMem = &ConstantBuffer2Data;
                    
                    ID3D11Buffer *ConstantBuffer2;
                    ID3D11Device_CreateBuffer(Device, &BufferDesc, &SubresourceData, &ConstantBuffer2);
                    ID3D11DeviceContext_PSSetConstantBuffers(DeviceContext, 0, 1, &ConstantBuffer2);
                    
                    ID3D11PixelShader *PixelShader;
                    u32 PixelShaderBlobSize = sizeof(PixelShaderBlob);
                    if(SUCCEEDED(HResult = ID3D11Device_CreatePixelShader(Device, PixelShaderBlob, PixelShaderBlobSize, 0, &PixelShader)))
                    {            
                        ID3D11DeviceContext_PSSetShader(DeviceContext, PixelShader, 0, 0);
                        
                        
                        ID3D11DeviceContext_IASetPrimitiveTopology(DeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                        
                        D3D11_VIEWPORT ViewPort;
                        ViewPort.Width = 800;
                        ViewPort.Height = 600;
                        ViewPort.MinDepth = 0;
                        ViewPort.MaxDepth = 1;
                        ViewPort.TopLeftX = 0;
                        ViewPort.TopLeftY = 0;
                        ID3D11DeviceContext_RSSetViewports(DeviceContext, 1, &ViewPort);
                        
                        ID3D11DeviceContext_DrawIndexed(DeviceContext, ArrayCount(Indicies), 0, 0);
                        
                        ID3D11PixelShader_Release(PixelShader);
                    }
                    else
                    {
                        Win32LogError_(HResult, "Failed to create pixel shader");
                    }
                }
                else
                {
                    Win32LogError_(HResult, "Failed to create input layout");
                }
                
                ID3D11VertexShader_Release(VertexShader);
            }
            else
            {
                Win32LogError_(HResult, "Failed to create vertex shader");
            }
            
        }
        else
        {
            Win32LogError_(HResult, "Failed to create index buffer");
        }
        
        ID3D11Buffer_Release(VertexBuffer);
    }
    else
    {
        Win32LogError_(HResult, "Failed to create vertex buffer");
    }
}

internal s32
Win32ProcessPendingMessages(win32_window_state *WindowState)
{
    s32 Result = 0;
    
    MSG Msg;
    while(Win32PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE))
    {
        if(Msg.message == WM_QUIT)
        {
            Result = Msg.wParam;
            WindowState->IsRunning = false;
            break;
        }
        else
        {
            Win32TranslateMessage(&Msg);
            Win32DispatchMessageA(&Msg);
        }
    }
    
    return Result;
}

s32 CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, s32 CmdShow)
{
    s32 Result = 0;
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalWin32State.PerfCountFrequency = (s64)PerfCountFrequencyResult.QuadPart;
    GlobalAppMemory.PlatformAPI.MakeWorkQueue = Win32MakeWorkQueue;
    GlobalAppMemory.PlatformAPI.AddWorkEntry = Win32AddWorkEntry;
    GlobalAppMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
    
#if KENGINE_CONSOLE
    GlobalAppMemory.PlatformAPI.InitConsoleCommandLoop = Win32InitConsoleCommandLoop;
#endif
    
    GlobalAppMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
    GlobalAppMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;
    GlobalAppMemory.PlatformAPI.GetMemoryStats = Win32GetMemoryStats;
    
#if 0    
    GlobalAppMemory.PlatformAPI.GetGlyphForCodePoint = Win32GetGlyphForCodePoint;
    GlobalAppMemory.PlatformAPI.GetHorizontalAdvance = Win32GetHorizontalAdvance;
    GlobalAppMemory.PlatformAPI.GetVerticleAdvance = Win32GetVerticleAdvance;
    
    GlobalAppMemory.PlatformAPI.DeleteHttpCache = Win32DeleteHttpCache;
    GlobalAppMemory.PlatformAPI.EndHttpClient = Win32EndHttpClient;
    GlobalAppMemory.PlatformAPI.BeginHttpClientWithCreds = Win32BeginHttpClientWithCreds;
    GlobalAppMemory.PlatformAPI.BeginHttpClient = Win32BeginHttpClient;
    GlobalAppMemory.PlatformAPI.SetHttpClientTimeout = Win32SetHttpClientTimeout;
    GlobalAppMemory.PlatformAPI.EndHttpRequest = Win32EndHttpRequest;
    GlobalAppMemory.PlatformAPI.BeginHttpRequest = Win32BeginHttpRequest;
    GlobalAppMemory.PlatformAPI.SetHttpRequestHeaders = Win32SetHttpRequestHeaders;
    GlobalAppMemory.PlatformAPI.SendHttpRequestFromFile = Win32SendHttpRequestFromFile;
    GlobalAppMemory.PlatformAPI.SendHttpRequest = Win32SendHttpRequest;
    GlobalAppMemory.PlatformAPI.GetHttpResponseToFile = Win32GetHttpResponseToFile;
    GlobalAppMemory.PlatformAPI.GetHttpResponse = Win32GetHttpResponse;
#endif
    
    GlobalAppMemory.PlatformAPI.ReadEntireFile = Win32ReadEntireFile;
    GlobalAppMemory.PlatformAPI.WriteTextToFile = Win32WriteTextToFile;
    GlobalAppMemory.PlatformAPI.ZipDirectory = Win32ZipDirectory;
    GlobalAppMemory.PlatformAPI.UnzipToDirectory = Win32UnzipToDirectory;
    GlobalAppMemory.PlatformAPI.FileExists = Win32FileExists;
    GlobalAppMemory.PlatformAPI.PermanentDeleteFile = Win32PermanentDeleteFile;
    GlobalAppMemory.PlatformAPI.DirectoryExists = Win32DirectoryExists;
    GlobalAppMemory.PlatformAPI.CreateDirectory = Win32CreateDirectory;
    GlobalAppMemory.PlatformAPI.RequestCloseProcess = Win32RequestCloseProcess;
    GlobalAppMemory.PlatformAPI.KillProcess = Win32KillProcess;
    GlobalAppMemory.PlatformAPI.CheckForProcess = Win32CheckForProcess;
    GlobalAppMemory.PlatformAPI.ExecuteProcess = Win32ExecuteProcess;
    
#if 0    
    GlobalAppMemory.PlatformAPI.GetHostname = Win32GetHostname;
    GlobalAppMemory.PlatformAPI.GetHomeDirectory = Win32GetHomeDirectory;
    GlobalAppMemory.PlatformAPI.GetAppConfigDirectory = Win32GetAppConfigDirectory;
    GlobalAppMemory.PlatformAPI.GetUsername = Win32GetUsername;
    GlobalAppMemory.PlatformAPI.GetProcessId = Win32GetProcessId;
#endif
    
    GlobalAppMemory.PlatformAPI.GetSystemTimestamp = Win32GetSystemTimestamp;
    GlobalAppMemory.PlatformAPI.GetDateTimeFromTimestamp = Win32GetDateTimeFromTimestamp;
    GlobalAppMemory.PlatformAPI.ConsoleOut = Win32ConsoleOut;
    
#if KENGINE_INTERNAL
    GlobalAppMemory.DebugEventTable = GlobalDebugEventTable;
#endif
    
    Platform = GlobalAppMemory.PlatformAPI;
    
    win32_window_state WindowState_;
    ZeroStruct(WindowState_);
    win32_window_state *WindowState = &WindowState_;
    
    if(Win32WindowStateInit(WindowState, Instance, "KengineWindowClass") &&
       Win32CreateWindow(WindowState, 800, 600, "kengine") &&
       Win32InitDirect3D(WindowState))
    {
        WindowState->IsRunning = true;
        
        LARGE_INTEGER StartCounter = Win32GetWallClock();
        
        while(WindowState->IsRunning)
        {
            Result = Win32ProcessPendingMessages(WindowState);
            
            f32 SecondsElapsed = Win32GetSecondsElapsed(StartCounter, Win32GetWallClock(), GlobalWin32State.PerfCountFrequency);
            f32 Color = Sin(SecondsElapsed) / 2.0f + 0.5f;
            
            Win32ClearBuffer(WindowState, V4(Color, Color, 1.0f, 1.0f));
            
            Win32DrawTestTriangle(WindowState, -SecondsElapsed,  0.0f, 0.0f);
            
            Win32DrawTestTriangle(WindowState, SecondsElapsed, 
                                  WindowState->MouseP.X / 400.0f - 1.0f, 
                                  -WindowState->MouseP.Y / 300.0f + 1.0f);
            
            Win32EndFrame(WindowState);
        }
    }
    
    Win32CleanupDirect3D(WindowState);
    Win32DestroyWindow(WindowState->Hwnd);
    
    return Result;
}