#include "kengine_platform.h"
#include "kengine_memory.h"
#include "kengine_generated.h"
#include "win32_kengine_types.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"


#ifndef VERSION
#define VERSION 0
#endif // VERSION

#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"

#include "kengine_debug_shared.h"
#if KENGINE_INTERNAL
#include "kengine_debug.h"
global debug_event_table GlobalDebugEventTable_;
debug_event_table *GlobalDebugEventTable = &GlobalDebugEventTable_;
#include "kengine_debug.c"
#endif

global app_memory GlobalAppMemory;

#include "kengine_telemetry.c"

#include "win32_kengine_shared.c"

global b32 GlobalWindowResize = false;


internal LRESULT __stdcall
Win32WndProc(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_KEYDOWN:
        {
            if(WParam == VK_ESCAPE)
            {
                Win32DestroyWindow(Window);
            }
        } break;
        
        case WM_DESTROY:
        {
            Win32PostQuitMessage(0);
        } break;
        
        case WM_SIZE:
        {
            GlobalWindowResize = true;
        } break;
        
        default:
        {
            Result = Win32DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    
    return Result;
}

#if KENGINE_CONSOLE
s32 __stdcall
mainCRTStartup()
#else
void __stdcall 
WinMainCRTStartup()
#endif
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    LARGE_INTEGER PerfCountFrequencyResult;
    Win32QueryPerformanceFrequency(&PerfCountFrequencyResult);
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
    
    LogInfo("Starting up");
    
    HINSTANCE Instance = Win32GetModuleHandleA(0);
    
    WNDCLASSEXA WindowClass;
    ZeroStruct(WindowClass);
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = &Win32WndProc;
    WindowClass.hInstance = Instance;
    WindowClass.hIcon = Win32LoadIconA(0, IDI_APPLICATION);
    WindowClass.hCursor = Win32LoadCursorA(0, IDC_ARROW);
    WindowClass.lpszClassName = "KengineWindowClass";
    WindowClass.hIconSm = WindowClass.hIcon;
    
    LogDebug("Registering window class");
    if(Win32RegisterClassExA(&WindowClass))
    {
        RECT InitialRect;
        InitialRect.left = 0;
        InitialRect.top = 0;
        InitialRect.right = 1024;
        InitialRect.bottom = 768;
        Win32AdjustWindowRectEx(&InitialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
        LONG InitialWidth = InitialRect.right - InitialRect.left;
        LONG InitialHeight = InitialRect.bottom - InitialRect.top;
        LogDebug("Setting initial window dimenstions to %u, %u", InitialWidth, InitialHeight);
        
        LogDebug("Creating window");
        HWND Window;
        
        if(SUCCEEDED(Window = Win32CreateWindowExA(WS_EX_OVERLAPPEDWINDOW, WindowClass.lpszClassName, "kengine",
                                                   WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, InitialWidth, InitialHeight,
                                                   0, 0, Instance, 0)))
        {
            HRESULT HResult;
            ID3D11Device1 *D3D11Device = 0;
            ID3D11DeviceContext1 *D3D11DeviceContext = 0;
            LogDebug("Creating D3D11 device and context");
            {
                ID3D11Device *BaseDevice;
                ID3D11DeviceContext *BaseDeviceContext;
                D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
                u32 CreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if KENGINE_INTERNAL
                CreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
                if(SUCCEEDED(HResult = Win32D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, CreationFlags, &FeatureLevel, 1,
                                                              D3D11_SDK_VERSION, &BaseDevice, 0, &BaseDeviceContext)))
                {
                    LogDebug("Getting 1.1 interface of D3D11 Device and Context");
                    if(SUCCEEDED(HResult = ID3D11Device1_QueryInterface(BaseDevice, &IID_ID3D11Device1, &D3D11Device)))
                    {
                        if(FAILED(HResult = ID3D11DeviceContext1_QueryInterface(BaseDeviceContext, &IID_ID3D11DeviceContext1, &D3D11DeviceContext)))
                        {
                            LogDebug("Failed to get interface of D3D11 device context");
                        }
                    }
                    else
                    {
                        LogDebug("Failed to get interface of D3D11 device");
                        
                    }
                    ID3D11Device1_Release(BaseDevice);
                    ID3D11Device1_Release(BaseDeviceContext);
                }
                else
                {
                    Win32LogError_(HResult, "Failed to create base device and context");
                    
                }
                
            }
            
#if KENGINE_INTERNAL
            LogDebug("Setting up debug layer to break on D3D11 errors");
            ID3D11Debug *D3D11Debug = 0;
            ID3D11Device1_QueryInterface(D3D11Device, &IID_ID3D11Debug, &D3D11Debug);
            if(D3D11Debug)
            {
                ID3D11InfoQueue *D3D11InfoQueue = 0;
                if(SUCCEEDED(HResult = ID3D11Debug_QueryInterface(D3D11Debug, &IID_ID3D11InfoQueue, &D3D11InfoQueue)))
                {
                    ID3D11InfoQueue_SetBreakOnSeverity(D3D11InfoQueue, D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                    ID3D11InfoQueue_SetBreakOnSeverity(D3D11InfoQueue, D3D11_MESSAGE_SEVERITY_ERROR, true);
                    ID3D11InfoQueue_Release(D3D11InfoQueue);
                }
                else
                {
                    Win32LogError_(HResult, "Failed to query info queue");
                }
                ID3D11Debug_Release(D3D11Debug);
            }
            else
            {
                LogError("Failed to create D3D11 debug layer");
            }
#endif
            
            IDXGISwapChain1 *D3D11SwapChain = 0;
            LogDebug("Creating D3D11 swap chain");
            {
                IDXGIFactory2 *DXGIFactory = 0;
                LogDebug("Getting DXGI factory");
                {
                    IDXGIDevice1 *DXGIDevice;
                    if(SUCCEEDED(HResult = ID3D11Device1_QueryInterface(D3D11Device, &IID_IDXGIDevice1, &DXGIDevice)))
                    {
                        IDXGIAdapter *DXGIAdapter;
                        if(SUCCEEDED(HResult = IDXGIDevice1_GetAdapter(DXGIDevice, &DXGIAdapter)))
                        {
                            DXGI_ADAPTER_DESC AdapterDesc;
                            IDXGIAdapter_GetDesc(DXGIAdapter, &AdapterDesc);
                            
                            LogInfo("Graphics Device: %ls", AdapterDesc.Description);
                            
                            if(FAILED(HResult = IDXGIAdapter_GetParent(DXGIAdapter, &IID_IDXGIFactory2, &DXGIFactory)))
                            {
                                Win32LogError_(HResult, "Failed to get DXGI factory");
                            }
                            IDXGIAdapter_Release(DXGIAdapter);
                        }
                        else
                        {
                            Win32LogError_(HResult, "Failed to get DXGI adapter");
                        }
                        IDXGIDevice1_Release(DXGIDevice);
                    }
                    else
                    {
                        Win32LogError_(HResult, "Failed to get DXGI device");
                    }
                }
                
                if(DXGIFactory)
                {                
                    DXGI_SWAP_CHAIN_DESC1 D3D11SwapChainDesc;
                    ZeroStruct(D3D11SwapChainDesc);
                    D3D11SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
                    D3D11SwapChainDesc.SampleDesc.Count = 1;
                    D3D11SwapChainDesc.SampleDesc.Quality = 0;
                    D3D11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                    D3D11SwapChainDesc.BufferCount = 2;
                    D3D11SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
                    D3D11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                    D3D11SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                    D3D11SwapChainDesc.Flags = 0;
                    
                    // NOTE(kstandbridge): Use Window width and height
                    D3D11SwapChainDesc.Width = 0; 
                    D3D11SwapChainDesc.Height = 0;
                    
                    if(FAILED(HResult = IDXGIFactory2_CreateSwapChainForHwnd(DXGIFactory, (IUnknown *)D3D11Device, Window, 
                                                                             &D3D11SwapChainDesc, 0, 0, &D3D11SwapChain)))
                    {
                        Win32LogError_(HResult, "Failed to create D3D11 swap chain for window handle");
                    }
                }
            }
            
            if(D3D11SwapChain)
            {
                LogDebug("Creating D3D11 framebuffer rendet target");
                ID3D11RenderTargetView *D3D11RenderTargetView = 0;
                {
                    ID3D11Texture2D *D3D11FrameBuffer;
                    if(SUCCEEDED(HResult = IDXGISwapChain1_GetBuffer(D3D11SwapChain, 0, &IID_ID3D11Texture2D, &D3D11FrameBuffer)))
                    {
                        if(FAILED(HResult = ID3D11Device1_CreateRenderTargetView(D3D11Device, (ID3D11Resource *)D3D11FrameBuffer, 0, &D3D11RenderTargetView)))
                        {
                            Win32LogError_(HResult, "Failed to create D3D11 render target view");
                        }
                    }
                    else
                    {
                        Win32LogError_(HResult, "Failed to get D3D11 framebuffer");
                    }
                    ID3D11Texture2D_Release(D3D11FrameBuffer);
                }
                
                ID3D11VertexShader *D3D11VertexShader = 0;
                ID3D11InputLayout *D3D11InputLayout = 0;
                LogDebug("Creating D3D11 vertex shader");
                {
                    ID3DBlob *VertexShaderBlob;
                    ID3DBlob *ShaderCompileErrorsBlob;
                    if(SUCCEEDED(HResult = Win32D3DCompileFromFile(L"..\\lib\\kengine\\code\\shaders.hlsl", 0, 0, "vs_main", "vs_5_0", 0, 0, 
                                                                   &VertexShaderBlob, &ShaderCompileErrorsBlob)))
                    {
                        if(SUCCEEDED(HResult = ID3D11Device1_CreateVertexShader(D3D11Device,
                                                                                ID3D10Blob_GetBufferPointer(VertexShaderBlob),
                                                                                ID3D10Blob_GetBufferSize(VertexShaderBlob),
                                                                                0, &D3D11VertexShader)))
                        {
                            LogDebug("Creating D3D11 input layout");
                            
                            D3D11_INPUT_ELEMENT_DESC InputElementDesc[2];
                            
                            InputElementDesc[0].SemanticName = "POS";
                            InputElementDesc[0].SemanticIndex = 0;
                            InputElementDesc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
                            InputElementDesc[0].InputSlot = 0;
                            InputElementDesc[0].AlignedByteOffset = 0;
                            InputElementDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                            InputElementDesc[0].InstanceDataStepRate = 0;
                            
                            InputElementDesc[1].SemanticName = "COL";
                            InputElementDesc[1].SemanticIndex = 0;
                            InputElementDesc[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                            InputElementDesc[1].InputSlot = 0;
                            InputElementDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                            InputElementDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                            InputElementDesc[1].InstanceDataStepRate = 0;
                            
                            if(FAILED(HResult = ID3D11Device1_CreateInputLayout(D3D11Device, InputElementDesc, ArrayCount(InputElementDesc),
                                                                                ID3D10Blob_GetBufferPointer(VertexShaderBlob),
                                                                                ID3D10Blob_GetBufferSize(VertexShaderBlob),
                                                                                &D3D11InputLayout)))
                            {
                                Win32LogError_(HResult, "Failed to create D3D11 input layout");
                            }
                        }
                        else
                        {
                            Win32LogError_(HResult, "Failed to create D3D11 vertex shader");
                        }
                    }
                    else
                    {
                        if(HResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                        {
                            LogError("Failed to compile D3D11 vertex shader, file not found");
                        }
                        else
                        {
                            string ErrorMessage = String_(ID3D10Blob_GetBufferSize(ShaderCompileErrorsBlob), 
                                                          ID3D10Blob_GetBufferPointer(ShaderCompileErrorsBlob));
                            LogError("Failed to compile D3D11 vertex shader, %S", ErrorMessage);
                            ID3D10Blob_Release(ShaderCompileErrorsBlob);
                        }
                    }
                    ID3D10Blob_Release(VertexShaderBlob);
                }
                
                ID3D11PixelShader *D3D11PixelShader = 0;
                LogDebug("Creating D3D11 pixel shader");
                {
                    ID3DBlob *PixelShaderBlob;
                    ID3DBlob *ShaderCompileErrorsBlob;
                    if(SUCCEEDED(HResult = Win32D3DCompileFromFile(L"..\\lib\\kengine\\code\\shaders.hlsl", 0, 0, "ps_main", "ps_5_0", 0, 0, 
                                                                   &PixelShaderBlob, &ShaderCompileErrorsBlob)))
                    {
                        if(FAILED(HResult = ID3D11Device1_CreatePixelShader(D3D11Device,
                                                                            ID3D10Blob_GetBufferPointer(PixelShaderBlob),
                                                                            ID3D10Blob_GetBufferSize(PixelShaderBlob),
                                                                            0, &D3D11PixelShader)))
                        {
                            Win32LogError_(HResult, "Failed to create D3D11 pixel shader");
                        }
                        ID3D10Blob_Release(PixelShaderBlob);
                    }
                    else
                    {
                        if(HResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                        {
                            LogError("Failed to compile D3D11 pixel shader, file not found");
                        }
                        else
                        {
                            string ErrorMessage = String_(ID3D10Blob_GetBufferSize(ShaderCompileErrorsBlob), 
                                                          ID3D10Blob_GetBufferPointer(ShaderCompileErrorsBlob));
                            LogError("Failed to compile D3D11 pixel shader, %S", ErrorMessage);
                            ID3D10Blob_Release(ShaderCompileErrorsBlob);
                        }
                    }
                }
                
                if(D3D11VertexShader &&
                   D3D11InputLayout &&
                   D3D11PixelShader)
                {
                    
                    ID3D11Buffer *VertexBuffer;
                    u32 VertexCount = 0;
                    u32 Stride;
                    u32 Offset;
                    {
                        // NOTE(kstandbridge): x, y, r, g, b, a
                        f32 VertexData[] = 
                        {
                            0.0f,  0.5f, 0.f, 1.f, 0.f, 1.f,
                            0.5f, -0.5f, 1.f, 0.f, 0.f, 1.f,
                            -0.5f, -0.5f, 0.f, 0.f, 1.f, 1.f
                        };
                        Stride = 6 * sizeof(f32);
                        VertexCount = sizeof(VertexData) / Stride;
                        Offset = 0;
                        
                        D3D11_BUFFER_DESC VertexBufferDesc;
                        ZeroStruct(VertexBufferDesc);
                        VertexBufferDesc.ByteWidth = sizeof(VertexData);
                        VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
                        VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                        
                        D3D11_SUBRESOURCE_DATA VertexSubresourceData;
                        ZeroStruct(VertexSubresourceData);
                        VertexSubresourceData.pSysMem = VertexData;
                        
                        if(FAILED(HResult = ID3D11Device1_CreateBuffer(D3D11Device, &VertexBufferDesc, &VertexSubresourceData, &VertexBuffer)))
                        {
                            Win32LogError_(HResult, "Failed to create D3D11 buffer");
                        }
                        else
                        {
                            b32 IsRunning = true;
                            while(IsRunning)
                            {
                                MSG Message;
                                ZeroStruct(Message);
                                while(Win32PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                                {
                                    if(Message.message == WM_QUIT)
                                    {
                                        IsRunning = false;
                                    }
                                    
                                    Win32TranslateMessage(&Message);
                                    Win32DispatchMessageA(&Message);
                                }
                                
                                if(GlobalWindowResize)
                                {
                                    ID3D11DeviceContext1_OMSetRenderTargets(D3D11DeviceContext, 0, 0, 0);
                                    
                                    ID3D11RenderTargetView_Release(D3D11RenderTargetView);
                                    
                                    if(SUCCEEDED(HResult = IDXGISwapChain1_ResizeBuffers(D3D11SwapChain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0)))
                                    {
                                        ID3D11Texture2D *D3D11FrameBuffer;
                                        if(SUCCEEDED(HResult = IDXGISwapChain1_GetBuffer(D3D11SwapChain, 0, &IID_ID3D11Texture2D, &D3D11FrameBuffer)))
                                        {
                                            if(FAILED(HResult = ID3D11Device1_CreateRenderTargetView(D3D11Device, (ID3D11Resource *)D3D11FrameBuffer, 
                                                                                                     0, &D3D11RenderTargetView)))
                                            {
                                                Win32LogError_(HResult, "Failed to create D3D11 render target view");
                                                IsRunning = false;
                                            }
                                        }
                                        else
                                        {
                                            Win32LogError_(HResult, "Failed to get D3D11 buffer");
                                            IsRunning = false;
                                        }
                                        
                                        ID3D11Texture2D_Release(D3D11FrameBuffer);
                                    }
                                    else
                                    {
                                        Win32LogError_(HResult, "Failed to resize D3D11 buffers");
                                        IsRunning = false;
                                    }
                                    
                                    GlobalWindowResize = false;
                                }
                                
                                f32 BackgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
                                ID3D11DeviceContext1_ClearRenderTargetView(D3D11DeviceContext, D3D11RenderTargetView, BackgroundColor);
                                
                                RECT WindowRect;
                                Win32GetClientRect(Window, &WindowRect);
                                D3D11_VIEWPORT ViewPort;
                                ZeroStruct(ViewPort);
                                ViewPort.TopLeftX = 0.0f;
                                ViewPort.TopLeftY = 0.0f;
                                ViewPort.Width = WindowRect.right - WindowRect.left;
                                ViewPort.Height = WindowRect.bottom - WindowRect.top;
                                ViewPort.MinDepth = 0.0f;
                                ViewPort.MaxDepth = 1.0f;
                                ID3D11DeviceContext1_RSSetViewports(D3D11DeviceContext, 1, &ViewPort);
                                
                                ID3D11DeviceContext1_OMSetRenderTargets(D3D11DeviceContext, 1, &D3D11RenderTargetView, 0);
                                
                                ID3D11DeviceContext1_IASetPrimitiveTopology(D3D11DeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                                ID3D11DeviceContext1_IASetInputLayout(D3D11DeviceContext, D3D11InputLayout);
                                
                                ID3D11DeviceContext1_VSSetShader(D3D11DeviceContext, D3D11VertexShader, 0, 0);
                                ID3D11DeviceContext1_PSSetShader(D3D11DeviceContext, D3D11PixelShader, 0, 0);
                                
                                ID3D11DeviceContext1_IASetVertexBuffers(D3D11DeviceContext, 0, 1, &VertexBuffer, &Stride, &Offset);
                                
                                ID3D11DeviceContext1_Draw(D3D11DeviceContext, VertexCount, 0);
                                
                                IDXGISwapChain1_Present(D3D11SwapChain, 1, 0);
                            }
                        }
                    }
                    
                    
                }
                
            }
        }
        else
        {
            Win32LogError("Failed to create window");
        }
    }
    else
    {
        Win32LogError("Failed to register window class %s", WindowClass.lpszClassName);
    }
    
    Win32ExitProcess(0);
    
    InvalidCodePath;
}