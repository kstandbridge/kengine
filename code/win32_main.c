#include "kengine_platform.h"
#include "kengine_memory.h"
#include "kengine_generated.h"
#include "win32_kengine_types.h"
#include "win32_resource.h"
#include "win32_kengine_opengl.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "kengine_render_group.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"


#ifndef VERSION
#define VERSION 0
#endif // VERSION

#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"

#include "win32_vertex_shader_generated.h"
#include "win32_glyph_pixel_shader_generated.h"
#include "win32_sprite_pixel_shader_generated.h"
#include "win32_rect_pixel_shader_generated.h"

global app_memory GlobalAppMemory;

#include "kengine_debug_shared.h"
#if KENGINE_INTERNAL
#include "kengine_debug.h"
global debug_event_table GlobalDebugEventTable_;
debug_event_table *GlobalDebugEventTable = &GlobalDebugEventTable_;
#include "kengine_debug.c"
#endif

#if KENGINE_INTERNAL
#include "kengine_sort.c"
#include "kengine_telemetry.c"
#include "kengine_render_group.c"
#else
#include "kengine.h"
#include "kengine.c"
#endif
#include "win32_kengine_shared.c"
#if KENGINE_HTTP
#include "win32_kengine_http.c"
#endif

// TODO(kstandbridge): Put this in win32_state;
global s32 GlobalWindowWidth;
global s32 GlobalWindowHeight;

typedef struct vertex_instance
{
    v3 Offset;
    v2 Size;
    v4 Color;
    v4 SpriteUV;
} vertex_instance;

#define MAX_VERTEX_INSTANCES 2048
global vertex_instance VertexInstances[MAX_VERTEX_INSTANCES];
global u32 CurrentVertexInstanceIndex;

inline void
PushVertexInstance(v3 Offset, v2 Size, v4 Color, v4 SpriteUV)
{
    if(CurrentVertexInstanceIndex < MAX_VERTEX_INSTANCES)
    {
        vertex_instance *Vertex = VertexInstances + CurrentVertexInstanceIndex;
        CurrentVertexInstanceIndex++;
        
        Vertex->Offset = Offset;
        Vertex->Size = Size;
        Vertex->Color = Color;
        Vertex->SpriteUV = SpriteUV;
    }
    else
    {
        CurrentVertexInstanceIndex = 0;
        LogWarning("Go easy on the verticies!");
    }
}

v2
TransformV2(m4x4 Matrix, v2 Vector)
{
#if 0
    vec2d.transform = function(vec, mat){
        var data = mat.data;
        return new vec2d(
                         vec.x * data[0] + vec.y * data[1] + data[3],
                         vec.x * data[4] + vec.y * data[5] + data[7]);
    }
#endif
    
    v2 Result;
    
    Result.X = Vector.X * Matrix.E[0][0] + Vector.Y * Matrix.E[0][1] + Matrix.E[0][3];
    Result.Y = Vector.X * Matrix.E[1][0] + Vector.Y * Matrix.E[1][1] + Matrix.E[1][3];
    
    return Result;
    
}

internal m4x4
M4x4OrthographicOffCenterLH(float ViewLeft,
                            float ViewRight,
                            float ViewBottom,
                            float ViewTop,
                            float NearZ,
                            float FarZ)
{
    float ReciprocalWidth = 1.0f / (ViewRight - ViewLeft);
    float ReciprocalHeight = 1.0f / (ViewTop - ViewBottom);
    float fRange = 1.0f / (FarZ - NearZ);
    
    m4x4 Result;
    Result.E[0][0] = ReciprocalWidth + ReciprocalWidth;
    Result.E[0][1] = 0.0f;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = 0.0f;
    Result.E[1][1] = ReciprocalHeight + ReciprocalHeight;
    Result.E[1][2] = 0.0f;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = 0.0f;
    Result.E[2][2] = fRange;
    Result.E[2][3] = 0.0f;
    
    Result.E[3][0] = -(ViewLeft + ViewRight) * ReciprocalWidth;
    Result.E[3][1] = -(ViewTop + ViewBottom) * ReciprocalHeight;
    Result.E[3][2] = -fRange * NearZ;
    Result.E[3][3] = 1.0f;
    return Result;
}

internal m4x4
M4x4Orthographic(f32 ViewWidth, f32 ViewHeight, f32 NearZ, f32 FarZ)
{
    f32 fRange = 1.0f / (FarZ - NearZ);
    
    m4x4 Result;
    Result.E[0][0] =  2.0f / ViewWidth;
    Result.E[0][1] =  0.0f;
    Result.E[0][2] =  0.0f;
    Result.E[0][3] =  0.0f;
    
    Result.E[1][0] =  0.0f;
    Result.E[1][1] =  2.0f / ViewHeight;
    Result.E[1][2] =  0.0f;
    Result.E[1][3] =  0.0f;
    
    Result.E[2][0] =  0.0f;
    Result.E[2][1] =  0.0f;
    Result.E[2][2] =  fRange;
    Result.E[2][3] =  0.0f;
    
    Result.E[3][0] = -1.0f;
    Result.E[3][1] =  1.0f;
    Result.E[3][2] = -fRange * NearZ;
    Result.E[3][3] =  1.0f;
    
    return Result;
}

internal void
Foo(f32 Width, f32 Height, f32 X, f32 Y, v2 *Input) // 640, 480
{
    m4x4 Matrix = M4x4Identity();
    // Size
    
    Matrix = M4x4Multiply(Matrix, M4x4Scale(V3(Width*0.5f, Height*0.5f, 1.0f)));
    Matrix = M4x4Multiply(Matrix, M4x4Translation(V3(Width*0.5f + X, -Height*0.5f - Y, 1.0f)));
    
    // TODO(kstandbridge): cache this as only changes when window resizes
    Matrix = M4x4Multiply(Matrix, M4x4Orthographic(GlobalWindowWidth, GlobalWindowHeight, 0.1f, 10000.0f));
    
    Matrix = M4x4Transpose(Matrix);
    
    for(u32 Index = 0;
        Index < 6;
        ++Index)
    {
        Input[Index] = TransformV2(Matrix, Input[Index]);
    }
}

typedef struct constant
{
    m4x4 Transform;
} constant;

internal HRESULT
Win32RenderCreate()
{
    HRESULT HResult = 0;
    
    LogDebug("Creating renderer");
    
    D3D_FEATURE_LEVEL FeatureLevel;
    
    LogDebug("Creating device and context");
    {
        UINT Flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if KENGINE_INTERNAL
        Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        
        if(FAILED(HResult = Win32D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, Flags, 0, 0, D3D11_SDK_VERSION,
                                                   &GlobalWin32State.RenderDevice, &FeatureLevel, &GlobalWin32State.RenderContext)))
        {
            LogWarning("Failed to create D3D11 hardware device, attempting software mode");
            if(FAILED(HResult = Win32D3D11CreateDevice(0, D3D_DRIVER_TYPE_WARP, 0, Flags, 0, 0, D3D11_SDK_VERSION,
                                                       &GlobalWin32State.RenderDevice, &FeatureLevel, &GlobalWin32State.RenderContext)))
            {
                Win32LogError_(HResult, "Failed to create D3D11 software device");
            }
        }
        
        if(SUCCEEDED(HResult))
        {
            if(SUCCEEDED(HResult = ID3D11DeviceContext_QueryInterface(GlobalWin32State.RenderContext, &IID_ID3D11DeviceContext1, &GlobalWin32State.RenderContext1)))
            {
                LogDebug("Using ID3D11DeviceContext1");
                ID3D11DeviceContext_Release(GlobalWin32State.RenderContext);
                GlobalWin32State.RenderContext = (ID3D11DeviceContext *)GlobalWin32State.RenderContext1;
            }
            HResult = 0;
        }
        else
        {
            LogInfo("Weird edge case?");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating swap chain");
        IDXGIFactory *Factory;
        if(SUCCEEDED(HResult = Win32CreateDXGIFactory(&IID_IDXGIFactory, &Factory)))
        {
            DXGI_SWAP_CHAIN_DESC SwapChainDesc = 
            {
                .BufferDesc =
                {
                    .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                    .RefreshRate =
                    {
                        .Numerator = 60,
                        .Denominator = 1,
                    },
                },
                .SampleDesc =
                {
                    .Count = 1,
                    .Quality = 0
                },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .OutputWindow = GlobalWin32State.Window,
                .Windowed = TRUE,
            };
            
            SwapChainDesc.BufferCount = 2;
            SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            SwapChainDesc.Flags = 0;
            if(FAILED(HResult = 
                      IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)GlobalWin32State.RenderDevice, &SwapChainDesc, &GlobalWin32State.RenderSwapChain)))
            {
                LogDebug("Creating flip discard swap chain failed, attempting flip sequential");
                SwapChainDesc.BufferCount = 2;
                SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                if(FAILED(HResult =
                          IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)GlobalWin32State.RenderDevice, &SwapChainDesc, &GlobalWin32State.RenderSwapChain)))
                {
                    LogDebug("Creating flip sequential swap chain failed, attempting discard");
                    SwapChainDesc.BufferCount = 1;
                    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                    SwapChainDesc.Flags = 0;
                    if(SUCCEEDED(HResult =
                                 IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)GlobalWin32State.RenderDevice, &SwapChainDesc, &GlobalWin32State.RenderSwapChain)))
                    {
                        IDXGISwapChain2 *SwapChain2;
                        if(SUCCEEDED(HResult = 
                                     IDXGISwapChain_QueryInterface(GlobalWin32State.RenderSwapChain, &IID_IDXGISwapChain2, &SwapChain2)))
                        {
                            LogDebug("Using IDXGISwapChain2 for frame latency control");
                            GlobalWin32State.RenderFrameLatencyWait = IDXGISwapChain2_GetFrameLatencyWaitableObject(SwapChain2);
                            IDXGISwapChain2_Release(SwapChain2);
                        }
                    }
                }
            }
            
            if(SUCCEEDED(HResult))
            {
                if(FAILED(HResult = IDXGIFactory_MakeWindowAssociation(Factory, GlobalWin32State.Window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER)))
                {
                    Win32LogError_(HResult, "Make window association failed");
                }
            }
            
            IDXGIFactory_Release(Factory);
        }
        else
        {
            Win32LogError_(HResult, "Failed to create DXGI factory");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating rasterizer state");
        D3D11_RASTERIZER_DESC RasterizerDesc =
        {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_NONE,
            .FrontCounterClockwise = FALSE,
            .DepthBias = 0,
            .DepthBiasClamp = 0,
            .SlopeScaledDepthBias = 0.0f,
            .DepthClipEnable = TRUE,
            .ScissorEnable = FALSE,
            .MultisampleEnable = FALSE,
            .AntialiasedLineEnable = FALSE,
        };
        
        if(FAILED(HResult =
                  ID3D11Device_CreateRasterizerState(GlobalWin32State.RenderDevice, &RasterizerDesc, &GlobalWin32State.RenderRasterizerState)))
        {
            Win32LogError_(HResult, "Failed to create rasterizer state"); 
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating depth and stencil state");
        D3D11_DEPTH_STENCIL_DESC DepthStencilDesc =
        {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS_EQUAL,
            .StencilEnable = TRUE,
            .StencilReadMask = 0xFF,
            .StencilWriteMask = 0xFF,
            .FrontFace = 
            {
                .StencilFailOp = D3D11_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D11_STENCIL_OP_INCR,
                .StencilPassOp = D3D11_STENCIL_OP_KEEP,
                .StencilFunc = D3D11_COMPARISON_ALWAYS,
            },
            .BackFace =
            {
                .StencilFailOp = D3D11_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D11_STENCIL_OP_INCR,
                .StencilPassOp = D3D11_STENCIL_OP_KEEP,
                .StencilFunc = D3D11_COMPARISON_ALWAYS,
            }
        };
        
        if(FAILED(HResult =
                  ID3D11Device_CreateDepthStencilState(GlobalWin32State.RenderDevice, &DepthStencilDesc, &GlobalWin32State.RenderDepthStencilState)))
        {
            Win32LogError_(HResult, "Failed to create depth stencil state");
        }
    }
    
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating blend state");
        D3D11_BLEND_DESC BlendDesc =
        {
            .AlphaToCoverageEnable = FALSE,
            .IndependentBlendEnable = FALSE,
            .RenderTarget =
            {
                {
                    .BlendEnable = true,
                    
                    .SrcBlend = D3D11_BLEND_SRC_ALPHA,
                    .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                    .BlendOp = D3D11_BLEND_OP_ADD,
                    
                    .SrcBlendAlpha = D3D11_BLEND_ONE,
                    .DestBlendAlpha = D3D11_BLEND_ONE,
                    .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                    
                    .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
                },
            },
        };
        
        if(FAILED(HResult =
                  ID3D11Device_CreateBlendState(GlobalWin32State.RenderDevice, &BlendDesc, &GlobalWin32State.RenderBlendState)))
        {
            Win32LogError_(HResult, "Failed to create blend state");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating glyph vertex shader and input layout");
        D3D11_INPUT_ELEMENT_DESC InputElementDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "OFFSET", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "SPRITE_UV", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };
        
        size_t VertexShaderDataSize = sizeof(VertexShaderData);
        if(SUCCEEDED(HResult = 
                     ID3D11Device_CreateVertexShader(GlobalWin32State.RenderDevice, VertexShaderData, VertexShaderDataSize, 0, &GlobalWin32State.RenderVertexShader)))
        {
            if(FAILED(HResult =
                      ID3D11Device_CreateInputLayout(GlobalWin32State.RenderDevice, InputElementDesc, ArrayCount(InputElementDesc),
                                                     VertexShaderData, sizeof(VertexShaderData), &GlobalWin32State.RenderInputLayout)))
            {
                Win32LogError_(HResult, "Failed to create input layout");
            }
        }
        else
        {
            Win32LogError_(HResult, "Failed to create glyph vertex shader");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating glyph pixel shader");
        size_t GlyphPixelShaderDataSize = sizeof(GlyphPixelShaderData);
        if(FAILED(HResult =
                  ID3D11Device_CreatePixelShader(GlobalWin32State.RenderDevice, GlyphPixelShaderData, GlyphPixelShaderDataSize, 0, &GlobalWin32State.RenderGlyphPixelShader)))
        {
            Win32LogError_(HResult, "Failed to create glyph pixel shader");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating sprite pixel shader");
        size_t SpritePixelShaderDataSize = sizeof(SpritePixelShaderData);
        if(FAILED(HResult =
                  ID3D11Device_CreatePixelShader(GlobalWin32State.RenderDevice, SpritePixelShaderData, SpritePixelShaderDataSize, 0, &GlobalWin32State.RenderSpritePixelShader)))
        {
            Win32LogError_(HResult, "Failed to create sprite pixel shader");
        }
    }
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating rect pixel shader");
        size_t RectPixelShaderDataSize = sizeof(RectPixelShaderData);
        if(FAILED(HResult =
                  ID3D11Device_CreatePixelShader(GlobalWin32State.RenderDevice, RectPixelShaderData, RectPixelShaderDataSize, 0, &GlobalWin32State.RenderRectPixelShader)))
        {
            Win32LogError_(HResult, "Failed to create rect pixel shader");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        v4 Vertices[] =
        {
            
#if 1
            {  0.0f, -1.0f, 0.0f, 1.0f }, // Bottom Left?
            {  1.0f,  0.0f, 1.0f, 0.0f }, // Top Right?
            {  0.0f,  0.0f, 0.0f, 0.0f }, // Top Left?
            
            {  0.0f, -1.0f, 0.0f, 1.0f }, // Bottom Left
            {  1.0f, -1.0f, 1.0f, 1.0f }, // Bottom Right?
            {  1.0f,  0.0f, 1.0f, 0.0f }, // Top Right?
#else
            { -0.5f,  0.5f, 0.0f, 0.0f },
            {  0.5f, -0.5f, 1.0f, 1.0f },
            { -0.5f, -0.5f, 0.0f, 1.0f },
            { -0.5f,  0.5f, 0.0f, 0.0f },
            {  0.5f,  0.5f, 1.0f, 0.0f },
            {  0.5f, -0.5f, 1.0f, 1.0f },
#endif
        };
        
        LogDebug("Creating vertex buffer");
        D3D11_BUFFER_DESC BufferDesc =
        {
            .ByteWidth = sizeof(Vertices),
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        };
        
        D3D11_SUBRESOURCE_DATA SubresourceData =
        {
            .pSysMem = Vertices,
        };
        
        if(FAILED(HResult =
                  ID3D11Device_CreateBuffer(GlobalWin32State.RenderDevice, &BufferDesc, &SubresourceData, &GlobalWin32State.RenderVertexBuffer)))
        {
            Win32LogError_(HResult, "Failed to create vertex buffer");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating vertex instance buffer");
        D3D11_BUFFER_DESC BufferDesc =
        {
            .ByteWidth = sizeof(VertexInstances),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        
        if(FAILED(HResult =
                  ID3D11Device_CreateBuffer(GlobalWin32State.RenderDevice, &BufferDesc, 0, &GlobalWin32State.RenderInstanceBuffer)))
        {
            Win32LogError_(HResult, "Failed to create vertex instance buffer");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating constant buffer");
        D3D11_BUFFER_DESC BufferDesc =
        {
            .ByteWidth = sizeof(constant),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        
        if(FAILED(HResult =
                  ID3D11Device_CreateBuffer(GlobalWin32State.RenderDevice, &BufferDesc, 0, &GlobalWin32State.RenderConstantBuffer)))
        {
            Win32LogError_(HResult, "Failed to create constant buffer");
        }
    }
    
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating sampler state");
        D3D11_SAMPLER_DESC SamplerDesc =
        {
            .Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            .AddressU       = D3D11_TEXTURE_ADDRESS_BORDER,
            .AddressV       = D3D11_TEXTURE_ADDRESS_BORDER,
            .AddressW       = D3D11_TEXTURE_ADDRESS_BORDER,
            .ComparisonFunc = D3D11_COMPARISON_NEVER,
            .BorderColor    = { 1.0f, 1.0f, 1.0f, 1.0f },
        };
        
        if(FAILED(HResult =
                  ID3D11Device_CreateSamplerState(GlobalWin32State.RenderDevice, &SamplerDesc, &GlobalWin32State.RenderSamplerState)))
        {
            Win32LogError_(HResult, "Failed to create samper state");
        }
        
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Loading sprite texture");
        s32 TotalWidth;
        s32 TotalHeight;
        s32 TextureChannelCount;
        s32 TextureForceChannelCount = 4;
        u8 *TextureBytes = stbi_load("..\\data\\sprite_sheet.png", &TotalWidth, &TotalHeight,
                                     &TextureChannelCount, TextureForceChannelCount);
        Assert(TextureBytes);
        s32 TextureBytesPerRow = 4 * TotalWidth;
        
        D3D11_TEXTURE2D_DESC TextureDesc =
        {
            .Width = TotalWidth,
            .Height = TotalHeight,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
            .SampleDesc = 
            {
                .Count = 1,
            },
        };
        
        D3D11_SUBRESOURCE_DATA SubresourceData =
        {
            .pSysMem = TextureBytes,
            .SysMemPitch = TextureBytesPerRow
        };
        
        ID3D11Texture2D *Texture;
        if(SUCCEEDED(HResult = 
                     ID3D11Device_CreateTexture2D(GlobalWin32State.RenderDevice, &TextureDesc, &SubresourceData, &Texture)))
        {
            
            if(FAILED(HResult =
                      ID3D11Device_CreateShaderResourceView(GlobalWin32State.RenderDevice, (ID3D11Resource *)Texture, 0, &GlobalWin32State.RenderSpriteTextureView)))
            {
                Win32LogError_(HResult, "Failed to create sprite texture view");
            }
            
            ID3D11Texture2D_Release(Texture);
        }
        else
        {
            Win32LogError_(HResult, "Failed to create sprite texture");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Loading glyph texture");
        
        string FontData = Win32ReadEntireFile(&GlobalWin32State.Arena, String("C:\\Windows\\Fonts\\segoeui.ttf"));
        stbtt_InitFont(&GlobalWin32State.FontInfo, FontData.Data, 0);
        
        f32 MaxFontHeightInPixels = 32;
        GlobalWin32State.FontScale = stbtt_ScaleForPixelHeight(&GlobalWin32State.FontInfo, MaxFontHeightInPixels);
        stbtt_GetFontVMetrics(&GlobalWin32State.FontInfo, &GlobalWin32State.FontAscent, &GlobalWin32State.FontDescent, &GlobalWin32State.FontLineGap);
        
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
        
        glyph_info *GlyphInfo = GlobalWin32State.GlyphInfos;
        
        for(u32 CodePoint = FirstChar;
            CodePoint < LastChar;
            ++CodePoint)
        {                
            GlyphInfo->Data = stbtt_GetCodepointSDF(&GlobalWin32State.FontInfo, GlobalWin32State.FontScale, CodePoint, Padding, OnEdgeValue, PixelDistanceScale, 
                                                    &GlyphInfo->Width, &GlyphInfo->Height, 
                                                    &GlyphInfo->XOffset, &GlyphInfo->YOffset);
            stbtt_GetCodepointHMetrics(&GlobalWin32State.FontInfo, CodePoint, &GlyphInfo->AdvanceWidth, &GlyphInfo->LeftSideBearing);
            
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
        u32 *TextureBytes = PushSize(&GlobalWin32State.Arena, TextureSize);
        s32 TextureBytesPerRow = 4 * TotalWidth;
        
        u32 AtX = 0;
        u32 AtY = 0;
        
        ColumnAt = 0;
        
        for(u32 Index = 0;
            Index < ArrayCount(GlobalWin32State.GlyphInfos);
            ++Index)
        {
            GlyphInfo = GlobalWin32State.GlyphInfos + Index;
            
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
        
        D3D11_TEXTURE2D_DESC TextureDesc =
        {
            .Width = TotalWidth,
            .Height = TotalHeight,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
            .SampleDesc = 
            {
                .Count = 1,
            },
        };
        
        D3D11_SUBRESOURCE_DATA SubresourceData =
        {
            .pSysMem = TextureBytes,
            .SysMemPitch = TextureBytesPerRow
        };
        
        ID3D11Texture2D *Texture;
        if(SUCCEEDED(HResult = 
                     ID3D11Device_CreateTexture2D(GlobalWin32State.RenderDevice, &TextureDesc, &SubresourceData, &Texture)))
        {
            
            if(FAILED(HResult =
                      ID3D11Device_CreateShaderResourceView(GlobalWin32State.RenderDevice, (ID3D11Resource *)Texture, 0, &GlobalWin32State.RenderGlyphTextureView)))
            {
                Win32LogError_(HResult, "Failed to create glyph texture view");
            }
            
            ID3D11Texture2D_Release(Texture);
        }
        else
        {
            Win32LogError_(HResult, "Failed to create glyph texture");
        }
    }
    
    return HResult;
}

#define D3D11SafeRelease(Release, Obj) if (Obj) { Release##_Release(Obj); }

internal void
Win32RenderDestroy()
{
    LogDebug("Destroying renderer");
    
    if(GlobalWin32State.RenderContext)
    {
        ID3D11DeviceContext_ClearState(GlobalWin32State.RenderContext);
    }
    
    D3D11SafeRelease(ID3D11Buffer, GlobalWin32State.RenderConstantBuffer);
    D3D11SafeRelease(ID3D11Buffer, GlobalWin32State.RenderInstanceBuffer);
    D3D11SafeRelease(ID3D11Buffer, GlobalWin32State.RenderVertexBuffer);
    D3D11SafeRelease(ID3D11InputLayout, GlobalWin32State.RenderInputLayout);
    D3D11SafeRelease(ID3D11VertexShader, GlobalWin32State.RenderVertexShader);
    
    D3D11SafeRelease(ID3D11ShaderResourceView, GlobalWin32State.RenderGlyphTextureView);
    D3D11SafeRelease(ID3D11PixelShader, GlobalWin32State.RenderGlyphPixelShader);
    
    D3D11SafeRelease(ID3D11ShaderResourceView, GlobalWin32State.RenderSpriteTextureView);
    D3D11SafeRelease(ID3D11PixelShader, GlobalWin32State.RenderSpritePixelShader);
    
    D3D11SafeRelease(ID3D11PixelShader, GlobalWin32State.RenderRectPixelShader);
    
    D3D11SafeRelease(ID3D11RasterizerState, GlobalWin32State.RenderRasterizerState);
    D3D11SafeRelease(ID3D11DepthStencilState, GlobalWin32State.RenderDepthStencilState);
    D3D11SafeRelease(ID3D11BlendState, GlobalWin32State.RenderBlendState);
    D3D11SafeRelease(ID3D11DepthStencilView, GlobalWin32State.RenderDepthStencilView);
    D3D11SafeRelease(ID3D11RenderTargetView, GlobalWin32State.RenderTargetView);
    D3D11SafeRelease(ID3D11Device, GlobalWin32State.RenderDevice);
    D3D11SafeRelease(ID3D11DeviceContext, GlobalWin32State.RenderContext);
    D3D11SafeRelease(IDXGISwapChain, GlobalWin32State.RenderSwapChain);
    D3D11SafeRelease(ID3D11SamplerState, GlobalWin32State.RenderSamplerState);
    
    GlobalWin32State.RenderContext1 = 0;
    GlobalWin32State.RenderFrameLatencyWait = 0;
}

internal HRESULT
Win32RecreateDevice()
{
    HRESULT Result;
    
    LogDebug("Recreating renderer");
    Win32RenderDestroy();
    if(FAILED(Result = Win32RenderCreate()))
    {
        LogDebug("Recreating renderer");
    }
    
    return Result;
}
inline HRESULT
Win32FatalDeviceLostError()
{
    HRESULT Result = E_FAIL;
    Win32MessageBoxW(0, L"Cannot recreate D3D11 device, it is reset or removed!", L"Error", MB_ICONEXCLAMATION);
    return Result;
}

internal HRESULT
Win32RenderResize(s32 Width, s32 Height)
{
    HRESULT Result = 0;
    
    if((Width == 0) ||
       (Height == 0))
    {
        LogDebug("Skipping render resize due to invalid size %dx%d", Width, Height);
        Result = S_OK;
    }
    else
    {
        LogDebug("Resizing renderer to %dx%d", Width, Height);
        
        GlobalWindowWidth = Width;
        GlobalWindowHeight = Height;
        
        if(GlobalWin32State.RenderTargetView)
        {
            ID3D11DeviceContext_OMSetRenderTargets(GlobalWin32State.RenderContext, 0, 0, 0);
            ID3D11RenderTargetView_Release(GlobalWin32State.RenderTargetView);
            GlobalWin32State.RenderTargetView = 0;
        }
        
        if(GlobalWin32State.RenderDepthStencilView)
        {
            ID3D11DepthStencilView_Release(GlobalWin32State.RenderDepthStencilView);
            GlobalWin32State.RenderDepthStencilView = 0;
        }
        
        LogDebug("Resizing buffers");
        Result = IDXGISwapChain_ResizeBuffers(GlobalWin32State.RenderSwapChain, 0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
        if((Result == DXGI_ERROR_DEVICE_REMOVED) ||
           (Result == DXGI_ERROR_DEVICE_RESET) ||
           (Result == DXGI_ERROR_DRIVER_INTERNAL_ERROR))
        {
            if(FAILED(Win32RecreateDevice()))
            {
                Result = Win32FatalDeviceLostError();
            }
            else
            {
                Win32LogError_(Result, "Failed to resize buffers");
            }
        }
        else
        {
            Result = S_OK;
        }
        
        if(SUCCEEDED(Result))
        {
            LogDebug("Creating render target view");
            ID3D11Texture2D *WindowBuffer;
            if(SUCCEEDED(Result =
                         IDXGISwapChain_GetBuffer(GlobalWin32State.RenderSwapChain, 0, &IID_ID3D11Texture2D, &WindowBuffer)))
            {
                if(FAILED(Result =
                          ID3D11Device_CreateRenderTargetView(GlobalWin32State.RenderDevice, (ID3D11Resource *)WindowBuffer, 0, &GlobalWin32State.RenderTargetView)))
                {
                    Win32LogError_(Result, "Failed to create render target view");
                }
                ID3D11Texture2D_Release(WindowBuffer);
            }
            else
            {
                Win32LogError_(Result, "Failed to get window buffer");
            }
        }
        
        if(SUCCEEDED(Result))
        {
            LogDebug("Creating depth stencil view");
            D3D11_TEXTURE2D_DESC DepthStencilDesc =
            {
                .Width = Width,
                .Height = Height,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
                .SampleDesc =
                {
                    .Count = 1,
                    .Quality = 0,
                },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            };
            
            ID3D11Texture2D *DepthStencil;
            if(SUCCEEDED(Result = 
                         ID3D11Device_CreateTexture2D(GlobalWin32State.RenderDevice, &DepthStencilDesc, 0, &DepthStencil)))
            {
                if(FAILED(Result =
                          ID3D11Device_CreateDepthStencilView(GlobalWin32State.RenderDevice, (ID3D11Resource *)DepthStencil, 0,
                                                              &GlobalWin32State.RenderDepthStencilView)))
                {
                    Win32LogError_(Result, "Failed to create depth stencil view");
                }
                ID3D11Texture2D_Release(DepthStencil);
            }
            else
            {
                Win32LogError_(Result, "Failed to create depth stencil");
            }
        }
        
        if(SUCCEEDED(Result))
        {
            LogDebug("Setting view port");
            D3D11_VIEWPORT Viewport =
            {
                .TopLeftX = 0.f,
                .TopLeftY = 0.f,
                .Width = (f32)Width,
                .Height = (f32)Height,
                .MinDepth = 0.f,
                .MaxDepth = 1.f,
            };
            ID3D11DeviceContext_RSSetViewports(GlobalWin32State.RenderContext, 1, &Viewport);
        }
    }
    
    return Result;
}

internal HRESULT
Win32RenderPresent()
{
    HRESULT Result = S_OK;
    
    if(GlobalWin32State.RenderOccluded)
    {
        Result = IDXGISwapChain_Present(GlobalWin32State.RenderSwapChain, 0, DXGI_PRESENT_TEST);
        if((SUCCEEDED(Result)) &&
           (Result != DXGI_STATUS_OCCLUDED))
        {
            LogDebug("DXGI window is back to normal, resuming rendering");
            GlobalWin32State.RenderOccluded = false;
        }
    }
    
    if(!GlobalWin32State.RenderOccluded)
    {
        Result = IDXGISwapChain_Present(GlobalWin32State.RenderSwapChain, 1, 0);
    }
    
    if((Result == DXGI_ERROR_DEVICE_RESET) ||
       (Result == DXGI_ERROR_DEVICE_REMOVED))
    {
        LogDebug("Device reset or removed, recreating");
        if(FAILED(Win32RecreateDevice()))
        {
            Result = Win32FatalDeviceLostError();
        }
        else
        {
            RECT Rect;
            if(!Win32GetClientRect(GlobalWin32State.Window, &Rect))
            {
                Win32LogError("Failed to get window rect");
            }
            else
            {
                Win32RenderResize(Rect.right - Rect.left, Rect.bottom - Rect.top);
            }
        }
    }
    else if(Result == DXGI_STATUS_OCCLUDED)
    {
        LogDebug("DXGI window is occluded, skipping rendering");
        GlobalWin32State.RenderOccluded = true;
    }
    else if(FAILED(Result))
    {
        Win32LogError_(Result, "Swap chain present failed");
    }
    
    if(GlobalWin32State.RenderOccluded)
    {
        Sleep(10);
    }
    else
    {
        if(GlobalWin32State.RenderContext1)
        {
            ID3D11DeviceContext1_DiscardView(GlobalWin32State.RenderContext1, (ID3D11View *)GlobalWin32State.RenderTargetView);
        }
    }
    
    return Result;
}

internal void
Win32RenderFrame(render_group *RenderGroup)
{
    if(!GlobalWin32State.RenderOccluded)
    {
        if(GlobalWin32State.RenderFrameLatencyWait)
        {
            WaitForSingleObjectEx(GlobalWin32State.RenderFrameLatencyWait, INFINITE, TRUE);
        }
        
        // NOTE(kstandbridge): BeginDraw!
        {        
            ID3D11DeviceContext_OMSetRenderTargets(GlobalWin32State.RenderContext, 1, &GlobalWin32State.RenderTargetView, GlobalWin32State.RenderDepthStencilView);
            ID3D11DeviceContext_ClearDepthStencilView(GlobalWin32State.RenderContext, GlobalWin32State.RenderDepthStencilView, 
                                                      D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            ID3D11DeviceContext_OMSetDepthStencilState(GlobalWin32State.RenderContext, GlobalWin32State.RenderDepthStencilState, 1);
        }
        
        {        
            // NOTE(kstandbridge): Clear background
            f32 ClearColor[] = { 0.1f, 0.2f, 0.6f, 1.0f };
            ID3D11DeviceContext_ClearRenderTargetView(GlobalWin32State.RenderContext, GlobalWin32State.RenderTargetView, ClearColor);
        }
        
        // NOTE(kstandbridge): Map the constant buffer which has our orthographic matrix in
        // TODO(kstandbridge): only when its dirty!
        {            
            D3D11_MAPPED_SUBRESOURCE MappedSubresource;
            ID3D11DeviceContext_Map(GlobalWin32State.RenderContext, (ID3D11Resource *)GlobalWin32State.RenderConstantBuffer, 
                                    0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
            constant *Constants = (constant *)MappedSubresource.pData;
            Constants->Transform = M4x4Orthographic(GlobalWindowWidth, GlobalWindowHeight, 10.0f, 0.0f);
            //Constants->Transform = M4x4Orthographic(GlobalWindowWidth, GlobalWindowHeight, 0.0f, 1.0f);
            //Constants->Transform = M4x4OrthographicOffCenterLH(0, GlobalWindowWidth, GlobalWindowHeight, 0, 0.0f, 1.0f);
            ID3D11DeviceContext_Unmap(GlobalWin32State.RenderContext, (ID3D11Resource *)GlobalWin32State.RenderConstantBuffer, 0);
        }
        
        // NOTE(kstandbridge): Populate Rects
        {
            CurrentVertexInstanceIndex = 0;
            for(u32 CommandIndex = 0;
                CommandIndex < RenderGroup->CurrentCommand;
                ++CommandIndex)
            {
                render_command *Command = RenderGroup->Commands + CommandIndex;
                if(Command->Type == RenderCommand_Rect)
                {
                    PushVertexInstance(Command->Rect.Offset, Command->Rect.Size,
                                       Command->Rect.Color, V4(0.0f, 0.0f, 1.0f, 1.0f));
                }
            }
        }
        
        // NOTE(kstandbridge): Copy Rect data
        {            
            D3D11_MAPPED_SUBRESOURCE MappedSubresource;
            ID3D11DeviceContext_Map(GlobalWin32State.RenderContext, (ID3D11Resource *)GlobalWin32State.RenderInstanceBuffer, 
                                    0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
            vertex_instance *DataGPU = (vertex_instance *)MappedSubresource.pData;
            
            memcpy(DataGPU, VertexInstances, sizeof(vertex_instance) * CurrentVertexInstanceIndex);
            
            ID3D11DeviceContext_Unmap(GlobalWin32State.RenderContext, (ID3D11Resource *)GlobalWin32State.RenderInstanceBuffer, 0);
        }
        
        // NOTE(kstandbridge): Draw rects
        {        
            ID3D11DeviceContext_IASetInputLayout(GlobalWin32State.RenderContext, GlobalWin32State.RenderInputLayout);
            ID3D11DeviceContext_VSSetConstantBuffers(GlobalWin32State.RenderContext, 0, 1, &GlobalWin32State.RenderConstantBuffer);
            u32 Strides[] = { sizeof(v4), sizeof(vertex_instance) };
            u32 Offsets[] = { 0, 0 };
            
            ID3D11Buffer *VertexBuffers[] =
            {
                GlobalWin32State.RenderVertexBuffer,
                GlobalWin32State.RenderInstanceBuffer
            };
            
            ID3D11DeviceContext_IASetVertexBuffers(GlobalWin32State.RenderContext, 0, 2, VertexBuffers, Strides, Offsets);
            
            ID3D11DeviceContext_IASetPrimitiveTopology(GlobalWin32State.RenderContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            ID3D11DeviceContext_VSSetShader(GlobalWin32State.RenderContext, GlobalWin32State.RenderVertexShader, 0, 0);
            ID3D11DeviceContext_PSSetShader(GlobalWin32State.RenderContext, GlobalWin32State.RenderRectPixelShader, 0, 0);
            
            ID3D11DeviceContext_PSSetSamplers(GlobalWin32State.RenderContext, 0, 1, &GlobalWin32State.RenderSamplerState);
            
            ID3D11DeviceContext_RSSetState(GlobalWin32State.RenderContext, GlobalWin32State.RenderRasterizerState);
            ID3D11DeviceContext_OMSetBlendState(GlobalWin32State.RenderContext, GlobalWin32State.RenderBlendState, 0, 0xffffffff);
            
            ID3D11DeviceContext_DrawInstanced(GlobalWin32State.RenderContext, 6, CurrentVertexInstanceIndex, 0, 0);
        }
        
        // TODO(kstandbridge): Sprite commands
        
#if 0        
        // NOTE(kstandbridge): Populate Sprites
        {
            CurrentVertexInstanceIndex = 0;
            PushVertexInstance(V3(500.0f, 300.0f, 4.0f),
                               V2(100, 100),
                               V4(1.0f, 1.0f, 1.0f, 1.0f),
                               V4(0.126f, 0.354f, 0.141f, 0.374f));
        }
        
        // NOTE(kstandbridge): Copy sprite data
        {            
            D3D11_MAPPED_SUBRESOURCE MappedSubresource;
            ID3D11DeviceContext_Map(GlobalWin32State.RenderContext, (ID3D11Resource *)GlobalWin32State.RenderInstanceBuffer, 
                                    0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
            vertex_instance *DataGPU = (vertex_instance *)MappedSubresource.pData;
            
            memcpy(DataGPU, VertexInstances, sizeof(vertex_instance) * CurrentVertexInstanceIndex);
            
            ID3D11DeviceContext_Unmap(GlobalWin32State.RenderContext, (ID3D11Resource *)GlobalWin32State.RenderInstanceBuffer, 0);
        }
        
        
        // NOTE(kstandbridge): Draw sprite data
        {
            
            ID3D11DeviceContext_PSSetShaderResources(GlobalWin32State.RenderContext, 0, 1, &GlobalWin32State.RenderSpriteTextureView);
            
            ID3D11DeviceContext_PSSetShader(GlobalWin32State.RenderContext, GlobalWin32State.RenderSpritePixelShader, 0, 0);
            ID3D11DeviceContext_DrawInstanced(GlobalWin32State.RenderContext, 6, CurrentVertexInstanceIndex, 0, 0);
        }
#endif
        
        // NOTE(kstandbridge): Populate Glyphs
        {          
            CurrentVertexInstanceIndex = 0;
            for(u32 CommandIndex = 0;
                CommandIndex < RenderGroup->CurrentCommand;
                ++CommandIndex)
            {
                render_command *Command = RenderGroup->Commands + CommandIndex;
                if(Command->Type == RenderCommand_Text)
                {
                    f32 AtX = Command->Text.Offset.X;
                    f32 AtY = Command->Text.Offset.X +GlobalWin32State.FontScale*GlobalWin32State.FontAscent*Command->Text.Size.Y;
                    
                    for(umm Index = 0;
                        Index < Command->Text.Text.Size;
                        ++Index)
                    {
                        u32 CodePoint = Command->Text.Text.Data[Index];
                        
                        if(CodePoint == '\n')
                        {
                            AtY += GlobalWin32State.FontScale*GlobalWin32State.FontAscent*Command->Text.Size.Y;
                            AtX = 0.0f;
                        }
                        else
                        {
                            glyph_info *Info = GlobalWin32State.GlyphInfos + CodePoint;
                            
                            Assert(Info->CodePoint == CodePoint);
                            
                            PushVertexInstance(V3(AtX, AtY + Info->YOffset*Command->Text.Size.Y, 3.0f),
                                               V2Multiply(Command->Text.Size, V2(Info->Width, Info->Height)),
                                               Command->Text.Color,
                                               Info->UV);
                            
                            AtX += GlobalWin32State.FontScale*Info->AdvanceWidth*Command->Text.Size.X;
                            
                            if(Index < Command->Text.Text.Size)
                            {
                                s32 Kerning = stbtt_GetCodepointKernAdvance(&GlobalWin32State.FontInfo, CodePoint, Command->Text.Text.Data[Index + 1]);
                                AtX += GlobalWin32State.FontScale*Kerning*Command->Text.Size.X;
                            }
                        }
                    }
                }
            }
        }
        
        // NOTE(kstandbridge): Copy glyph data
        {            
            D3D11_MAPPED_SUBRESOURCE MappedSubresource;
            ID3D11DeviceContext_Map(GlobalWin32State.RenderContext, (ID3D11Resource *)GlobalWin32State.RenderInstanceBuffer, 
                                    0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
            vertex_instance *DataGPU = (vertex_instance *)MappedSubresource.pData;
            
            memcpy(DataGPU, VertexInstances, sizeof(vertex_instance) * CurrentVertexInstanceIndex);
            
            ID3D11DeviceContext_Unmap(GlobalWin32State.RenderContext, (ID3D11Resource *)GlobalWin32State.RenderInstanceBuffer, 0);
        }
        
        // NOTE(kstandbridge): Draw Glyph data
        {
            
            ID3D11DeviceContext_PSSetShaderResources(GlobalWin32State.RenderContext, 0, 1, &GlobalWin32State.RenderGlyphTextureView);
            
            ID3D11DeviceContext_PSSetShader(GlobalWin32State.RenderContext, GlobalWin32State.RenderGlyphPixelShader, 0, 0);
            ID3D11DeviceContext_DrawInstanced(GlobalWin32State.RenderContext, 6, CurrentVertexInstanceIndex, 0, 0);
        }
    }
}

internal LRESULT CALLBACK
Win32WindowProc(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CREATE:
        {
            GlobalWin32State.Window = Window;
            if(FAILED(Win32RenderCreate()))
            {
                Win32LogError("Failed to create renderer");
                Result = -1;
            }
        } break;
        
        case WM_DESTROY:
        {
            Win32RenderDestroy();
            Win32PostQuitMessage(0);
        } break;
        
        case WM_SIZE:
        {
            if(FAILED(Win32RenderResize(LOWORD(LParam), HIWORD(LParam))))
            {
                Win32DestroyWindow(Window);
            }
        } break;
        
        default:
        {
            Result = Win32DefWindowProcW(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}


s32 WINAPI
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, s32 CmdShow)
{
    s32 Result = 0;
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    
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
    GlobalAppMemory.PlatformAPI.SkipHttpMetrics = Win32SkipHttpMetrics;
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
    GlobalAppMemory.PlatformAPI.ExecuteProcessWithOutput = Win32ExecuteProcessWithOutput;
    
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
    GlobalAppMemory.TelemetryState = GlobalTelemetryState;
    GlobalAppMemory.DebugEventTable = GlobalDebugEventTable;
    // NOTE(kstandbridge): Populate paths for DLL hotloading
    {    
        char Filename[MAX_PATH];
        GetModuleFileNameA(0, Filename, MAX_PATH);
        u32 Length = GetNullTerminiatedStringLength(Filename);
        
        char *At = Filename;
        if(*At == '\"')
        {
            ++At;
            Length -= 2;
        }
        char *lastSlash = At + Length;
        while(*lastSlash != '\\')
        {
            --Length;
            --lastSlash;
        }
        Copy(Length, At, GlobalWin32State.ExeFilePath);
        GlobalWin32State.ExeFilePath[Length] = '\0';
        
        Copy(Length, GlobalWin32State.ExeFilePath, GlobalWin32State.DllFullFilePath);
        AppendCString(GlobalWin32State.DllFullFilePath + Length, "\\kengine.dll");
        
        Copy(Length, GlobalWin32State.ExeFilePath, GlobalWin32State.TempDllFullFilePath);
        AppendCString(GlobalWin32State.TempDllFullFilePath + Length, "\\kengine_temp.dll");
        
        Copy(Length, GlobalWin32State.ExeFilePath, GlobalWin32State.LockFullFilePath);
        AppendCString(GlobalWin32State.LockFullFilePath + Length, "\\lock.tmp");
    }
#endif
    
    Platform = GlobalAppMemory.PlatformAPI;
    
    WNDCLASSEXW WindowClass =
    {
        .cbSize = sizeof(WindowClass),
        .lpfnWndProc = Win32WindowProc,
        .hInstance = Instance,
        .hIcon = Win32LoadIconA(Instance, MAKEINTRESOURCE(IDI_ICON)),
        .hCursor = Win32LoadCursorA(NULL, IDC_ARROW),
        .lpszClassName = L"KengineWindowClass",
    };
    
    LogDebug("Registering %ls window class", WindowClass.lpszClassName);
    if(Win32RegisterClassExW(&WindowClass))
    {
        LogDebug("Creating window");
        HWND Window = Win32CreateWindowExW(WS_EX_APPWINDOW, WindowClass.lpszClassName, L"kengine",
                                           WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                           0, 0, WindowClass.hInstance, 0);
        if(Window)
        {
            LogDebug("Begin application loop");
            
            render_command RenderCommands[4096];
            u32 MaxRenderCommands = ArrayCount(RenderCommands);
            
            for(;;)
            {
                
#if KENGINE_INTERNAL
                b32 DllNeedsToBeReloaded = false;
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(GlobalWin32State.DllFullFilePath);
                if(CompareFileTime(&NewDLLWriteTime, &GlobalWin32State.LastDLLWriteTime) != 0)
                {
                    WIN32_FILE_ATTRIBUTE_DATA Ignored;
                    if(!GetFileAttributesExA(GlobalWin32State.LockFullFilePath, GetFileExInfoStandard, &Ignored))
                    {
                        DllNeedsToBeReloaded = true;
                    }
                }
#endif
                
                MSG Msg;
                if(Win32PeekMessageW(&Msg, 0, 0, 0, PM_REMOVE))
                {
                    if(Msg.message == WM_QUIT)
                    {
                        break;
                    }
                    Win32TranslateMessage(&Msg);
                    Win32DispatchMessageW(&Msg);
                    continue;
                }
                
#if KENGINE_INTERNAL
                if(DllNeedsToBeReloaded)
                {
                    if(GlobalWin32State.AppLibrary && !FreeLibrary(GlobalWin32State.AppLibrary))
                    {
                        Win32LogError("Failed to free app library");
                    }
                    GlobalWin32State.AppLibrary = 0;
                    GlobalWin32State.AppTick_ = 0;
#if KENGINE_CONSOLE
                    GlobalWin32State.AppHandleCommand = 0;
#endif
#if KENGINE_HTTP
                    GlobalWin32State.AppHandleHttpRequest = 0;
#endif // KENGINE_HTTP
                    
                    if(CopyFileA(GlobalWin32State.DllFullFilePath, GlobalWin32State.TempDllFullFilePath, false))
                    {
                        LogDebug("App code reloaded!");
                        GlobalWin32State.AppLibrary = LoadLibraryA(GlobalWin32State.TempDllFullFilePath);
                        if(GlobalWin32State.AppLibrary)
                        {
                            GlobalWin32State.AppTick_ = (app_tick_ *)GetProcAddress(GlobalWin32State.AppLibrary, "AppTick_");
                            Assert(GlobalWin32State.AppTick_);
#if KENGINE_CONSOLE
                            // NOTE(kstandbridge): Command handler is optional
                            GlobalWin32State.AppHandleCommand = (app_handle_command *)GetProcAddress(GlobalWin32State.AppLibrary, "AppHandleCommand");
#endif // KENGINE_CONSOLE
                            
#if KENGINE_HTTP
                            GlobalWin32State.AppHandleHttpRequest = (app_handle_http_request *)Win32GetProcAddressA(GlobalWin32State.AppLibrary, "AppHandleHttpRequest");
                            Assert(GlobalWin32State.AppHandleHttpRequest);
#endif // KENGINE_HTTP
                            
                            GlobalWin32State.LastDLLWriteTime = NewDLLWriteTime;
                        }
                        else
                        {
                            Win32LogError("Failed to load app library");
                        }
                    }
                    else
                    {
                        Win32LogError("Failed to copy %S to %S", GlobalWin32State.DllFullFilePath, GlobalWin32State.TempDllFullFilePath);
                    }
                }
#endif
                
                render_group RenderGroup =
                {
                    .Commands = RenderCommands,
                    .CurrentCommand = 0,
                    .MaxCommands = MaxRenderCommands,
                    .Width = GlobalWindowWidth,
                    .Height = GlobalWindowHeight
                };
                
#if KENGINE_INTERNAL
                if(GlobalWin32State.AppTick_)
                {
                    GlobalWin32State.AppTick_(&GlobalAppMemory, &RenderGroup);
                }
#else
                AppTick_(&RenderGroup);
#endif
                
                Win32RenderFrame(&RenderGroup);
                
                if(FAILED(Win32RenderPresent()))
                {
                    break;
                }
            }
            LogDebug("Ended application loop");
        }
        else
        {
            Win32LogError("Failed to create window");
        }
        
        LogDebug("Unregistering %ls window class", WindowClass.lpszClassName);
        Win32UnregisterClassW(WindowClass.lpszClassName, WindowClass.hInstance);
        
        ClearArena(&GlobalWin32State.Arena);
    }
    else
    {
        Win32LogError("Failed to register window class");
    }
    
    
    LogDebug("Exiting");
    
    PostTelemetryThread(0);
    
    return Result;
}