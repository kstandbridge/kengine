#include "kengine_platform.h"
#include "kengine_memory.h"
#include "kengine_generated.h"
#include "win32_kengine_types.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

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

typedef struct glyph_info
{
    u8 *Data;
    
    u32 CodePoint;
    
    s32 Width;
    s32 Height;
    s32 XOffset;
    s32 YOffset;
    
    v4 UV;
    
} glyph_info;

#if KENGINE_CONSOLE
s32 __stdcall
mainCRTStartup()
#else
s32 CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
#endif
{
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
    
    LogInfo("Starting up");
    
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
                    D3D11SwapChainDesc.Width = 0;
                    D3D11SwapChainDesc.Height = 0;
                    D3D11SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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
                IDXGIFactory2_Release(DXGIFactory);
            }
            
            
            LogDebug("Creating D3D11 framebuffer render target");
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
                        
                        D3D11_INPUT_ELEMENT_DESC InputElementDesc[5];
                        
                        InputElementDesc[0].SemanticName = "POS";
                        InputElementDesc[0].SemanticIndex = 0;
                        InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
                        InputElementDesc[0].InputSlot = 0;
                        InputElementDesc[0].AlignedByteOffset = 0;
                        InputElementDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                        InputElementDesc[0].InstanceDataStepRate = 0;
                        
                        InputElementDesc[1].SemanticName = "TEX";
                        InputElementDesc[1].SemanticIndex = 0;
                        InputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
                        InputElementDesc[1].InputSlot = 0;
                        InputElementDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                        InputElementDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                        InputElementDesc[1].InstanceDataStepRate = 0;
                        
                        InputElementDesc[2].SemanticName = "COLOR_INSTANCE";
                        InputElementDesc[2].SemanticIndex = 0;
                        InputElementDesc[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                        InputElementDesc[2].InputSlot = 1;
                        InputElementDesc[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                        InputElementDesc[2].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                        InputElementDesc[2].InstanceDataStepRate = 1;
                        
                        InputElementDesc[3].SemanticName = "TEX_INSTANCE";
                        InputElementDesc[3].SemanticIndex = 0;
                        InputElementDesc[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                        InputElementDesc[3].InputSlot = 1;
                        InputElementDesc[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                        InputElementDesc[3].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                        InputElementDesc[3].InstanceDataStepRate = 1;
                        
                        InputElementDesc[4].SemanticName = "SIZE_INSTANCE";
                        InputElementDesc[4].SemanticIndex = 0;
                        InputElementDesc[4].Format = DXGI_FORMAT_R32G32_FLOAT;
                        InputElementDesc[4].InputSlot = 1;
                        InputElementDesc[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                        InputElementDesc[4].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                        InputElementDesc[4].InstanceDataStepRate = 1;
                        
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
            
            LogDebug("Creating D3D11 instance VertexBuffer");
            ID3D11Buffer *InstanceVertexBuffer;
            u32 InstanceStride;
            {
                
#define MAX_GLYPH_COUNT 16384
#define SIZE_OF_GLYPH_INSTANCE_IN_BYTES (sizeof(float)*10)
#define GLYPH_INSTANCE_DATA_TOTAL_SIZE_IN_BYTES MAX_GLYPH_COUNT*SIZE_OF_GLYPH_INSTANCE_IN_BYTES
                
                
                D3D11_BUFFER_DESC VertexBufferDesc;
                ZeroStruct(VertexBufferDesc);
                VertexBufferDesc.ByteWidth = (UINT)GLYPH_INSTANCE_DATA_TOTAL_SIZE_IN_BYTES;
                VertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
                VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                VertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                
                InstanceStride = SIZE_OF_GLYPH_INSTANCE_IN_BYTES;
                
                ID3D11Device1_CreateBuffer(D3D11Device, &VertexBufferDesc, 0, &InstanceVertexBuffer);
            }
            
            LogDebug("Creating D3D11 vertex buffer");
            ID3D11Buffer *VertexBuffer;
            u32 VertexCount = 0;
            u32 Stride;
            // NOTE(kstandbridge): x, y, z, u, v
            f32 VertexData[] = 
            {
                -0.5f,  0.5f, 0.f, 0.f, 0.f,
		        0.5f, -0.5f, 0.f, 1.f, 1.f,
		        -0.5f, -0.5f, 0.f, 0.f, 1.f,
		        -0.5f,  0.5f, 0.f, 0.f, 0.f,
		        0.5f,  0.5f, 0.f, 1.f, 0.f,
		        0.5f, -0.5f, 0.f, 1.f, 1.f
            };
            Stride = 5 * sizeof(f32);
            VertexCount = sizeof(VertexData) / Stride;
            
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
                
                LogDebug("Creating D3D11 sampler state");
                D3D11_SAMPLER_DESC SamplerDesc;
                ZeroStruct(SamplerDesc);
                SamplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                SamplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
                SamplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
                SamplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
                SamplerDesc.BorderColor[0] = 1.0f;
                SamplerDesc.BorderColor[1] = 1.0f;
                SamplerDesc.BorderColor[2] = 1.0f;
                SamplerDesc.BorderColor[3] = 1.0f;
                SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
                
                ID3D11SamplerState *SamplerState;
                ID3D11Device1_CreateSamplerState(D3D11Device, &SamplerDesc, &SamplerState);
                
                ID3D11BlendState *D3D11BlendState;
                {                
                    LogDebug("Creating D3D11 blender state");
                    D3D11_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc;
                    ZeroStruct(RenderTargetBlendDesc);
                    RenderTargetBlendDesc.BlendEnable = true;
                    RenderTargetBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
                    RenderTargetBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                    RenderTargetBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
                    RenderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
                    RenderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
                    RenderTargetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
                    RenderTargetBlendDesc.RenderTargetWriteMask = 0x0f;
                    
                    D3D11_BLEND_DESC BlendDesc;
                    ZeroStruct(BlendDesc);
                    BlendDesc.AlphaToCoverageEnable = false;
                    BlendDesc.IndependentBlendEnable = false;
                    BlendDesc.RenderTarget[0] = RenderTargetBlendDesc;
                    
                    ID3D11Device1_CreateBlendState(D3D11Device, &BlendDesc, &D3D11BlendState);
                    
                    f32 BlendFactor[4];
                    BlendFactor[0] = 1.0f;
                    BlendFactor[1] = 1.0f;
                    BlendFactor[2] = 1.0f;
                    BlendFactor[3] = 1.0f;
                    u32 SampleMask = 0xFFFFFFFF;
                    
                    ID3D11DeviceContext1_OMSetBlendState(D3D11DeviceContext, D3D11BlendState, BlendFactor, SampleMask);
                }
                
                ID3D11RasterizerState1 *D3D11RasterizerState;
                
                D3D11_RASTERIZER_DESC1 RasterizserDesc;
                RasterizserDesc.FillMode = D3D11_FILL_SOLID;
                RasterizserDesc.CullMode = D3D11_CULL_FRONT;
                RasterizserDesc.FrontCounterClockwise = true;
                RasterizserDesc.DepthBias = false;
                RasterizserDesc.DepthBiasClamp = 0;
                RasterizserDesc.SlopeScaledDepthBias = 0.0f;
                RasterizserDesc.DepthClipEnable = true;
                RasterizserDesc.ScissorEnable = false;
                RasterizserDesc.MultisampleEnable = false;
                RasterizserDesc.AntialiasedLineEnable = false;
                RasterizserDesc.ForcedSampleCount = 0;
                ID3D11Device1_CreateRasterizerState1(D3D11Device, &RasterizserDesc, &D3D11RasterizerState);
                ID3D11DeviceContext1_RSSetState(D3D11DeviceContext, (ID3D11RasterizerState *)D3D11RasterizerState);
                
#if 0
                LogDebug("Loading image");
                s32 TotalWidth;
                s32 TotalHeight;
                s32 TextureChannelCount;
                s32 TextureForceChannelCount = 4;
                u8 *TextureBytes = stbi_load("..\\data\\Texture.png", &TotalWidth, &TotalHeight,
                                             &TextureChannelCount, TextureForceChannelCount);
                Assert(TextureBytes);
                s32 TextureBytesPerRow = 4 * TotalWidth;
#else
                memory_arena MemoryArena;
                ZeroStruct(MemoryArena);
                string FontData = Win32ReadEntireFile(&MemoryArena, String("C:\\Windows\\Fonts\\segoeui.ttf"));
                
                stbtt_fontinfo FontInfo;
                ZeroStruct(FontInfo);
                stbtt_InitFont(&FontInfo, FontData.Data, 0);
                
                f32 MaxFontHeightInPixels = 32;
                f32 Scale = stbtt_ScaleForPixelHeight(&FontInfo, MaxFontHeightInPixels);
                s32 Padding = (s32)(MaxFontHeightInPixels / 3.0f);
                u8 OnEdgeValue = (u8)(0.8f*255);
                f32 PixelDistanceScale = (f32)OnEdgeValue/(f32)(Padding);
                
#if 1
                u32 FirstChar = 0;
                u32 LastChar = 256;
#else
                u32 FirstChar = 0x0400;
                u32 LastChar = FirstChar + 255;
#endif
                
                s32 MaxWidth = 0;
                s32 MaxHeight = 0;
                s32 TotalWidth = 0;
                s32 TotalHeight = 0;
                u32 ColumnAt = 0;
                u32 RowCount = 1;
                
                glyph_info GlyphInfos[256];
                ZeroArray(ArrayCount(GlyphInfos), GlyphInfos);
                
                glyph_info *GlyphInfo = GlyphInfos;
                
                for(u32 CodePoint = FirstChar;
                    CodePoint < LastChar;
                    ++CodePoint)
                {                
                    GlyphInfo->Data = stbtt_GetCodepointSDF(&FontInfo, Scale, CodePoint, Padding, OnEdgeValue, PixelDistanceScale, 
                                                            &GlyphInfo->Width, &GlyphInfo->Height, 
                                                            &GlyphInfo->XOffset, &GlyphInfo->YOffset);
                    
                    GlyphInfo->CodePoint = CodePoint;
                    
                    if(GlyphInfo->Data)
                    {
                        TotalWidth += GlyphInfo->Width;
                        ++ColumnAt;
                        
                        if(GlyphInfo->Height > MaxHeight)
                        {
                            MaxHeight = GlyphInfo->Height;
                        }
                    }
                    
                    if((ColumnAt % 16) == 0)
                    {
                        ++RowCount;
                        ColumnAt = 0;
                        if(TotalWidth > MaxWidth)
                        {
                            MaxWidth = TotalWidth;
                        }
                        TotalWidth = 0;
                    }
                    
                    ++GlyphInfo;
                }
                
                TotalWidth = MaxWidth;
                TotalHeight = MaxHeight*RowCount;
                
                umm TextureSize = TotalWidth*TotalHeight*sizeof(u32);
                u32 *TextureBytes = PushSize(&MemoryArena, TextureSize);
                s32 TextureBytesPerRow = 4 * TotalWidth;
                
                u32 AtX = 0;
                u32 AtY = 0;
                
                ColumnAt = 0;
                
                for(u32 Index = 0;
                    Index < ArrayCount(GlyphInfos);
                    ++Index)
                {
                    GlyphInfo = GlyphInfos + Index;
                    
                    GlyphInfo->UV = V4((f32)AtX / (f32)TotalWidth, (f32)AtY / (f32)TotalHeight,
                                       ((f32)AtX + (f32)GlyphInfo->Width) / (f32)TotalWidth, 
                                       ((f32)AtY + (f32)GlyphInfo->Height) / (f32)TotalHeight);
                    
                    for(s32 Y = 0;
                        Y < GlyphInfo->Height;
                        ++Y)
                    {
                        for(s32 X = 0;
                            X < GlyphInfo->Width;
                            ++X)
                        {
                            u32 Alpha = (u32)GlyphInfo->Data[(Y*GlyphInfo->Width) + X];
                            TextureBytes[(Y + AtY)*TotalWidth + (X + AtX)] = 0x00000000 | (u32)((Alpha) << 24);
                        }
                    }
                    
                    AtX += GlyphInfo->Width;
                    
                    ++ColumnAt;
                    
                    if((ColumnAt % 16) == 0)
                    {
                        AtY += MaxHeight;
                        AtX = 0;
                    }
                    
                    stbtt_FreeSDF(GlyphInfo->Data, 0);
                }
                
#endif
                
                LogDebug("Creating D3D11 texture");
                D3D11_TEXTURE2D_DESC TextureDesc;
                ZeroStruct(TextureDesc);
                TextureDesc.Width              = TotalWidth;
                TextureDesc.Height             = TotalHeight;
                TextureDesc.MipLevels          = 1;
                TextureDesc.ArraySize          = 1;
                TextureDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                TextureDesc.SampleDesc.Count   = 1;
                TextureDesc.Usage              = D3D11_USAGE_IMMUTABLE;
                TextureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
                
                D3D11_SUBRESOURCE_DATA TextureSubresourceData;
                ZeroStruct(TextureSubresourceData);
                TextureSubresourceData.pSysMem = TextureBytes;
                TextureSubresourceData.SysMemPitch = TextureBytesPerRow;
                
                ID3D11Texture2D *Texture;
                ID3D11Device1_CreateTexture2D(D3D11Device, &TextureDesc, &TextureSubresourceData, &Texture);
                
                ID3D11ShaderResourceView *TextureView;
                ID3D11Device1_CreateShaderResourceView(D3D11Device, (ID3D11Resource *)Texture, 0, &TextureView);
                
                // NOTE(kstandbridge): Constant buffer
                typedef struct constants
                {
                    v4 Color;
                    f32 Matrix[16];
                } constants;
                
                ID3D11Buffer *D3D11ConstantBuffer;
                LogDebug("Creating D3D11 constant buffer");
                {
                    D3D11_BUFFER_DESC ConstantBufferDesc;
                    ZeroStruct(ConstantBufferDesc);
                    // NOTE(kstandbridge): Must be 16 byte aligned
                    ConstantBufferDesc.ByteWidth = sizeof(constants) + 0xf & 0xfffffff0;
                    ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
                    ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                    ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    
                    if(FAILED(HResult = ID3D11Device1_CreateBuffer(D3D11Device, &ConstantBufferDesc, 0, &D3D11ConstantBuffer)))
                    {
                        Win32LogError_(HResult, "Failed to create D3D11 constant buffer");
                    }
                }
                
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
                    
                    // NOTE(kstandbridge): Constant buffer mapping
                    D3D11_MAPPED_SUBRESOURCE MappedSubresource;
                    ID3D11DeviceContext1_Map(D3D11DeviceContext, (ID3D11Resource *)D3D11ConstantBuffer, 
                                             0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
                    constants *Destination = (constants *)MappedSubresource.pData;
                    
                    f32 A = 2.0f / ViewPort.Width;
                    f32 B = 2.0f / ViewPort.Height;
                    f32 NearClip = 0.1f;
                    f32 FarClip = 1000.0f;
                    
                    f32 OffsetX = 0;
                    f32 OffsetY = 0;
                    
#if 1               
                    constants Constants = { 
                        V4(1.0f, 0.0f, 0.0f, 1.0f), 
                        {
                            A, 0, 0, 0,
                            0, B, 0, 0,
                            0, 0, 1.0f/(FarClip - NearClip), 0,
                            OffsetX, OffsetY, NearClip/(NearClip - FarClip), 1
                        } 
                    };
#else
                    constants Constants = { 
                        V4(1.0f, 0.0f, 0.0f, 1.0f), 
                        {
                            A, 0, 0, 0,
                            0, B, 0, 0,
                            0, 0, 1, 0,
                            -1, -1, 0, 1
                        } 
                    };
#endif
                    
                    *Destination = Constants;
                    
                    ID3D11DeviceContext1_Unmap(D3D11DeviceContext, (ID3D11Resource *)D3D11ConstantBuffer, 0);
                    
                    // NOTE(kstandbridge): COLOR_INSTANCE mapping?
                    {
                        D3D11_MAPPED_SUBRESOURCE MappedSubResource;
                        ZeroStruct(MappedSubResource);
                        ID3D11DeviceContext1_Map(D3D11DeviceContext, (ID3D11Resource *)InstanceVertexBuffer, 
                                                 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);
                        u8 *DestData = (u8 *)MappedSubResource.pData;
                        
                        f32 SourceData[10];
                        SourceData[0] = 1.0f;
                        SourceData[1] = 1.0f;
                        SourceData[2] = 1.0f;
                        SourceData[3] = 1.0f;
                        
                        glyph_info *Glyph = GlyphInfos + 'K';
                        Assert(Glyph->CodePoint == 'K');
                        SourceData[4] = Glyph->UV.R;
                        SourceData[5] = Glyph->UV.G;
                        SourceData[6] = Glyph->UV.B;
                        SourceData[7] = Glyph->UV.A;
                        
                        SourceData[8] = Glyph->Width*0.6f;
                        SourceData[9] = Glyph->Height*0.6f;
                        
                        memcpy(DestData, SourceData, SIZE_OF_GLYPH_INSTANCE_IN_BYTES);
                        
                        ID3D11DeviceContext1_Unmap(D3D11DeviceContext, (ID3D11Resource *)InstanceVertexBuffer, 0);
                    }
                    
                    
                    ID3D11DeviceContext1_OMSetRenderTargets(D3D11DeviceContext, 1, &D3D11RenderTargetView, 0);
                    
                    ID3D11DeviceContext1_IASetPrimitiveTopology(D3D11DeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    ID3D11DeviceContext1_IASetInputLayout(D3D11DeviceContext, D3D11InputLayout);
                    
                    ID3D11DeviceContext1_VSSetShader(D3D11DeviceContext, D3D11VertexShader, 0, 0);
                    ID3D11DeviceContext1_PSSetShader(D3D11DeviceContext, D3D11PixelShader, 0, 0);
                    
                    ID3D11DeviceContext1_PSSetShaderResources(D3D11DeviceContext, 0, 1, &TextureView);
                    ID3D11DeviceContext1_PSSetSamplers(D3D11DeviceContext, 0, 1, &SamplerState);
                    
                    ID3D11DeviceContext1_VSSetConstantBuffers(D3D11DeviceContext, 0, 1, &D3D11ConstantBuffer);
                    
                    ID3D11Buffer *VertexBuffers[2];
                    VertexBuffers[0] = VertexBuffer;
                    VertexBuffers[1] = InstanceVertexBuffer;
                    UINT Strides[2];
                    Strides[0] = Stride;
                    Strides[1] = InstanceStride;
                    UINT Offsets[2];
                    Offsets[0] = 0;
                    Offsets[1] = 0;
                    
                    ID3D11DeviceContext1_IASetVertexBuffers(D3D11DeviceContext, 0, 2, VertexBuffers, Strides, Offsets);
                    
                    ID3D11DeviceContext1_DrawInstanced(D3D11DeviceContext, VertexCount, 1, 0, 0);
                    
                    IDXGISwapChain1_Present(D3D11SwapChain, 1, 0);
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
    
    return 0;
}