#include "kengine_platform.h"
#include "kengine_memory.h"
#include "kengine_generated.h"
#include "win32_kengine_types.h"
#include "win32_resource.h"
#include "win32_kengine_opengl.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "kengine_renderer_shared.h"

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
#include "win32_pixel_shader_generated.h"

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
#else
#include "kengine.h"
#include "kengine.c"
#endif
#include "kengine_renderer.c"
#include "win32_kengine_shared.c"
#if KENGINE_HTTP
#include "win32_kengine_http.c"
#endif

// TODO(kstandbridge): Put this in win32_state;
global s32 GlobalWindowWidth;
global s32 GlobalWindowHeight;

typedef struct vertex_instance
{
    v2 Offset;
    v2 Size;
    v4 Color;
    v4 GlyphUV;
} vertex_instance;

#define MAX_VERTEX_INSTANCES 2048
global vertex_instance VertexInstances[MAX_VERTEX_INSTANCES];
global u32 CurrentVertexInstanceIndex;

inline void
PushVertexInstance(v2 Offset, v2 Size, v4 Color, v4 GlyphUV)
{
    if(CurrentVertexInstanceIndex < MAX_VERTEX_INSTANCES)
    {
        vertex_instance *Vertex = VertexInstances + CurrentVertexInstanceIndex;
        CurrentVertexInstanceIndex++;
        
        Vertex->Offset = Offset;
        Vertex->Size = Size;
        Vertex->Color = Color;
        Vertex->GlyphUV = GlyphUV;
    }
    else
    {
        LogError("VertexInstance buffer too small");
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
Win32RenderCreate(win32_state *Win32State)
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
                                                   &Win32State->RenderDevice, &FeatureLevel, &Win32State->RenderContext)))
        {
            LogWarning("Failed to create D3D11 hardware device, attempting software mode");
            if(FAILED(HResult = Win32D3D11CreateDevice(0, D3D_DRIVER_TYPE_WARP, 0, Flags, 0, 0, D3D11_SDK_VERSION,
                                                       &Win32State->RenderDevice, &FeatureLevel, &Win32State->RenderContext)))
            {
                Win32LogError_(HResult, "Failed to create D3D11 software device");
            }
        }
        
        if(SUCCEEDED(HResult))
        {
            if(SUCCEEDED(HResult = ID3D11DeviceContext_QueryInterface(Win32State->RenderContext, &IID_ID3D11DeviceContext1, &Win32State->RenderContext1)))
            {
                LogDebug("Using ID3D11DeviceContext1");
                ID3D11DeviceContext_Release(Win32State->RenderContext);
                Win32State->RenderContext = (ID3D11DeviceContext *)Win32State->RenderContext1;
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
                .OutputWindow = Win32State->Window,
                .Windowed = TRUE,
            };
            
            SwapChainDesc.BufferCount = 2;
            SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            SwapChainDesc.Flags = 0;
            if(FAILED(HResult = 
                      IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)Win32State->RenderDevice, &SwapChainDesc, &Win32State->RenderSwapChain)))
            {
                LogDebug("Creating flip discard swap chain failed, attempting flip sequential");
                SwapChainDesc.BufferCount = 2;
                SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                if(FAILED(HResult =
                          IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)Win32State->RenderDevice, &SwapChainDesc, &Win32State->RenderSwapChain)))
                {
                    LogDebug("Creating flip sequential swap chain failed, attempting discard");
                    SwapChainDesc.BufferCount = 1;
                    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                    SwapChainDesc.Flags = 0;
                    if(SUCCEEDED(HResult =
                                 IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)Win32State->RenderDevice, &SwapChainDesc, &Win32State->RenderSwapChain)))
                    {
                        IDXGISwapChain2 *SwapChain2;
                        if(SUCCEEDED(HResult = 
                                     IDXGISwapChain_QueryInterface(Win32State->RenderSwapChain, &IID_IDXGISwapChain2, &SwapChain2)))
                        {
                            LogDebug("Using IDXGISwapChain2 for frame latency control");
                            Win32State->RenderFrameLatencyWait = IDXGISwapChain2_GetFrameLatencyWaitableObject(SwapChain2);
                            IDXGISwapChain2_Release(SwapChain2);
                        }
                    }
                }
            }
            
            if(SUCCEEDED(HResult))
            {
                if(FAILED(HResult = IDXGIFactory_MakeWindowAssociation(Factory, Win32State->Window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER)))
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
                  ID3D11Device_CreateRasterizerState(Win32State->RenderDevice, &RasterizerDesc, &Win32State->RenderRasterizerState)))
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
                  ID3D11Device_CreateDepthStencilState(Win32State->RenderDevice, &DepthStencilDesc, &Win32State->RenderDepthStencilState)))
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
                  ID3D11Device_CreateBlendState(Win32State->RenderDevice, &BlendDesc, &Win32State->RenderBlendState)))
        {
            Win32LogError_(HResult, "Failed to create blend state");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating vertex shader and input layout");
        D3D11_INPUT_ELEMENT_DESC InputElementDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "OFFSET", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "GLYPHUV", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };
        
        size_t VertexShaderDataSize = sizeof(VertexShaderData);
        if(SUCCEEDED(HResult = 
                     ID3D11Device_CreateVertexShader(Win32State->RenderDevice, VertexShaderData, VertexShaderDataSize, 0, &Win32State->RenderVertexShader)))
        {
            if(FAILED(HResult =
                      ID3D11Device_CreateInputLayout(Win32State->RenderDevice, InputElementDesc, ArrayCount(InputElementDesc),
                                                     VertexShaderData, sizeof(VertexShaderData), &Win32State->RenderInputLayout)))
            {
                Win32LogError_(HResult, "Failed to create input layout");
            }
        }
        else
        {
            Win32LogError_(HResult, "Failed to create vertex shader");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating pixel shader");
        size_t PixelShaderDataSize = sizeof(PixelShaderData);
        if(FAILED(HResult =
                  ID3D11Device_CreatePixelShader(Win32State->RenderDevice, PixelShaderData, PixelShaderDataSize, 0, &Win32State->RenderPixelShader)))
        {
            Win32LogError_(HResult, "Failed to create pixel shader");
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
                  ID3D11Device_CreateBuffer(Win32State->RenderDevice, &BufferDesc, &SubresourceData, &Win32State->RenderVertexBuffer)))
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
                  ID3D11Device_CreateBuffer(Win32State->RenderDevice, &BufferDesc, 0, &Win32State->RenderInstanceBuffer)))
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
                  ID3D11Device_CreateBuffer(Win32State->RenderDevice, &BufferDesc, 0, &Win32State->RenderConstantBuffer)))
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
                  ID3D11Device_CreateSamplerState(Win32State->RenderDevice, &SamplerDesc, &Win32State->RenderSamplerState)))
        {
            Win32LogError_(HResult, "Failed to create samper state");
        }
        
    }
    
    if(SUCCEEDED(HResult))
    {
#if 0
        LogDebug("Loading texture");
        s32 TotalWidth;
        s32 TotalHeight;
        s32 TextureChannelCount;
        s32 TextureForceChannelCount = 4;
        u8 *TextureBytes = stbi_load("..\\data\\Mushroom.png", &TotalWidth, &TotalHeight,
                                     &TextureChannelCount, TextureForceChannelCount);
        Assert(TextureBytes);
        s32 TextureBytesPerRow = 4 * TotalWidth;
#else
        string FontData = Win32ReadEntireFile(&Win32State->Arena, String("C:\\Windows\\Fonts\\segoeui.ttf"));
        stbtt_InitFont(&Win32State->FontInfo, FontData.Data, 0);
        
        f32 MaxFontHeightInPixels = 32;
        Win32State->FontScale = stbtt_ScaleForPixelHeight(&Win32State->FontInfo, MaxFontHeightInPixels);
        stbtt_GetFontVMetrics(&Win32State->FontInfo, &Win32State->FontAscent, &Win32State->FontDescent, &Win32State->FontLineGap);
        
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
        
        glyph_info *GlyphInfo = Win32State->GlyphInfos;
        
        for(u32 CodePoint = FirstChar;
            CodePoint < LastChar;
            ++CodePoint)
        {                
            GlyphInfo->Data = stbtt_GetCodepointSDF(&Win32State->FontInfo, Win32State->FontScale, CodePoint, Padding, OnEdgeValue, PixelDistanceScale, 
                                                    &GlyphInfo->Width, &GlyphInfo->Height, 
                                                    &GlyphInfo->XOffset, &GlyphInfo->YOffset);
            stbtt_GetCodepointHMetrics(&Win32State->FontInfo, CodePoint, &GlyphInfo->AdvanceWidth, &GlyphInfo->LeftSideBearing);
            
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
        u32 *TextureBytes = PushSize(&Win32State->Arena, TextureSize);
        s32 TextureBytesPerRow = 4 * TotalWidth;
        
        u32 AtX = 0;
        u32 AtY = 0;
        
        ColumnAt = 0;
        
        for(u32 Index = 0;
            Index < ArrayCount(Win32State->GlyphInfos);
            ++Index)
        {
            GlyphInfo = Win32State->GlyphInfos + Index;
            
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
                     ID3D11Device_CreateTexture2D(Win32State->RenderDevice, &TextureDesc, &SubresourceData, &Texture)))
        {
            
            if(FAILED(HResult =
                      ID3D11Device_CreateShaderResourceView(Win32State->RenderDevice, (ID3D11Resource *)Texture, 0, &Win32State->RenderTextureView)))
            {
                Win32LogError_(HResult, "Failed to create texture view");
            }
            
            ID3D11Texture2D_Release(Texture);
        }
        else
        {
            Win32LogError_(HResult, "Failed to create texture");
        }
    }
    
    return HResult;
}

#define D3D11SafeRelease(Release, Obj) if (Obj) { Release##_Release(Obj); }

internal void
Win32RenderDestroy(win32_state *Win32State)
{
    LogDebug("Destroying renderer");
    
    if(Win32State->RenderContext)
    {
        ID3D11DeviceContext_ClearState(Win32State->RenderContext);
    }
    
    D3D11SafeRelease(ID3D11Buffer, Win32State->RenderConstantBuffer);
    D3D11SafeRelease(ID3D11Buffer, Win32State->RenderInstanceBuffer);
    D3D11SafeRelease(ID3D11Buffer, Win32State->RenderVertexBuffer);
    D3D11SafeRelease(ID3D11InputLayout, Win32State->RenderInputLayout);
    D3D11SafeRelease(ID3D11VertexShader, Win32State->RenderVertexShader);
    D3D11SafeRelease(ID3D11PixelShader, Win32State->RenderPixelShader);
    D3D11SafeRelease(ID3D11RasterizerState, Win32State->RenderRasterizerState);
    D3D11SafeRelease(ID3D11DepthStencilState, Win32State->RenderDepthStencilState);
    D3D11SafeRelease(ID3D11BlendState, Win32State->RenderBlendState);
    D3D11SafeRelease(ID3D11DepthStencilView, Win32State->RenderDepthStencilView);
    D3D11SafeRelease(ID3D11RenderTargetView, Win32State->RenderTargetView);
    D3D11SafeRelease(ID3D11Device, Win32State->RenderDevice);
    D3D11SafeRelease(ID3D11DeviceContext, Win32State->RenderContext);
    D3D11SafeRelease(IDXGISwapChain, Win32State->RenderSwapChain);
    D3D11SafeRelease(ID3D11SamplerState, Win32State->RenderSamplerState);
    D3D11SafeRelease(ID3D11ShaderResourceView, Win32State->RenderTextureView);
    
    Win32State->RenderContext1 = 0;
    Win32State->RenderFrameLatencyWait = 0;
}

internal HRESULT
Win32RecreateDevice(win32_state *Win32State)
{
    HRESULT Result;
    
    LogDebug("Recreating renderer");
    Win32RenderDestroy(Win32State);
    if(FAILED(Result = Win32RenderCreate(Win32State)))
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
Win32RenderResize(win32_state *Win32State, s32 Width, s32 Height)
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
        
        if(Win32State->RenderTargetView)
        {
            ID3D11DeviceContext_OMSetRenderTargets(Win32State->RenderContext, 0, 0, 0);
            ID3D11RenderTargetView_Release(Win32State->RenderTargetView);
            Win32State->RenderTargetView = 0;
        }
        
        if(Win32State->RenderDepthStencilView)
        {
            ID3D11DepthStencilView_Release(Win32State->RenderDepthStencilView);
            Win32State->RenderDepthStencilView = 0;
        }
        
        LogDebug("Resizing buffers");
        Result = IDXGISwapChain_ResizeBuffers(Win32State->RenderSwapChain, 0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
        if((Result == DXGI_ERROR_DEVICE_REMOVED) ||
           (Result == DXGI_ERROR_DEVICE_RESET) ||
           (Result == DXGI_ERROR_DRIVER_INTERNAL_ERROR))
        {
            if(FAILED(Win32RecreateDevice(Win32State)))
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
                         IDXGISwapChain_GetBuffer(Win32State->RenderSwapChain, 0, &IID_ID3D11Texture2D, &WindowBuffer)))
            {
                if(FAILED(Result =
                          ID3D11Device_CreateRenderTargetView(Win32State->RenderDevice, (ID3D11Resource *)WindowBuffer, 0, &Win32State->RenderTargetView)))
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
                         ID3D11Device_CreateTexture2D(Win32State->RenderDevice, &DepthStencilDesc, 0, &DepthStencil)))
            {
                if(FAILED(Result =
                          ID3D11Device_CreateDepthStencilView(Win32State->RenderDevice, (ID3D11Resource *)DepthStencil, 0,
                                                              &Win32State->RenderDepthStencilView)))
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
            ID3D11DeviceContext_RSSetViewports(Win32State->RenderContext, 1, &Viewport);
        }
    }
    
    return Result;
}

internal HRESULT
Win32RenderPresent(win32_state *Win32State)
{
    HRESULT Result = S_OK;
    
    if(Win32State->RenderOccluded)
    {
        Result = IDXGISwapChain_Present(Win32State->RenderSwapChain, 0, DXGI_PRESENT_TEST);
        if((SUCCEEDED(Result)) &&
           (Result != DXGI_STATUS_OCCLUDED))
        {
            LogDebug("DXGI window is back to normal, resuming rendering");
            Win32State->RenderOccluded = false;
        }
    }
    
    if(!Win32State->RenderOccluded)
    {
        Result = IDXGISwapChain_Present(Win32State->RenderSwapChain, 1, 0);
    }
    
    if((Result == DXGI_ERROR_DEVICE_RESET) ||
       (Result == DXGI_ERROR_DEVICE_REMOVED))
    {
        LogDebug("Device reset or removed, recreating");
        if(FAILED(Win32RecreateDevice(Win32State)))
        {
            Result = Win32FatalDeviceLostError();
        }
        else
        {
            RECT Rect;
            if(!Win32GetClientRect(Win32State->Window, &Rect))
            {
                Win32LogError("Failed to get window rect");
            }
            else
            {
                Win32RenderResize(Win32State, Rect.right - Rect.left, Rect.bottom - Rect.top);
            }
        }
    }
    else if(Result == DXGI_STATUS_OCCLUDED)
    {
        LogDebug("DXGI window is occluded, skipping rendering");
        Win32State->RenderOccluded = true;
    }
    else if(FAILED(Result))
    {
        Win32LogError_(Result, "Swap chain present failed");
    }
    
    if(Win32State->RenderOccluded)
    {
        Sleep(10);
    }
    else
    {
        if(Win32State->RenderContext1)
        {
            ID3D11DeviceContext1_DiscardView(Win32State->RenderContext1, (ID3D11View *)Win32State->RenderTargetView);
        }
    }
    
    return Result;
}

internal void
Win32RenderFrame(win32_state *Win32State)
{
    if(!Win32State->RenderOccluded)
    {
        if(Win32State->RenderFrameLatencyWait)
        {
            WaitForSingleObjectEx(Win32State->RenderFrameLatencyWait, INFINITE, TRUE);
        }
        
        // NOTE(kstandbridge): BeginDraw!
        {        
            ID3D11DeviceContext_OMSetRenderTargets(Win32State->RenderContext, 1, &Win32State->RenderTargetView, Win32State->RenderDepthStencilView);
            ID3D11DeviceContext_ClearDepthStencilView(Win32State->RenderContext, Win32State->RenderDepthStencilView, 
                                                      D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            ID3D11DeviceContext_OMSetDepthStencilState(Win32State->RenderContext, Win32State->RenderDepthStencilState, 1);
        }
        
        
        // NOTE(kstandbridge): PushClearCommand(color);
        {        
            // NOTE(kstandbridge): Clear background
            f32 ClearColor[] = { 0.1f, 0.2f, 0.6f, 1.0f };
            ID3D11DeviceContext_ClearRenderTargetView(Win32State->RenderContext, Win32State->RenderTargetView, ClearColor);
        }
        
        // NOTE(kstandbridge): PushRectGlyphBatchCommand(Position, color); ??
        // This currently draws multiple textures from a single call
        {        
            // NOTE(kstandbridge): Map the constant buffer which has our orthographic matrix in
            {            
                D3D11_MAPPED_SUBRESOURCE MappedSubresource;
                ID3D11DeviceContext_Map(Win32State->RenderContext, (ID3D11Resource *)Win32State->RenderConstantBuffer, 
                                        0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
                constant *Constants = (constant *)MappedSubresource.pData;
                Constants->Transform = M4x4Orthographic(GlobalWindowWidth, GlobalWindowHeight, 0.0f, 1.0f);
                //Constants->Transform = M4x4Orthographic(GlobalWindowWidth, GlobalWindowHeight, 0.0f, 1.0f);
                //Constants->Transform = M4x4OrthographicOffCenterLH(0, GlobalWindowWidth, GlobalWindowHeight, 0, 0.0f, 1.0f);
                ID3D11DeviceContext_Unmap(Win32State->RenderContext, (ID3D11Resource *)Win32State->RenderConstantBuffer, 0);
            }
            
            {            
                D3D11_MAPPED_SUBRESOURCE MappedSubresource;
                ID3D11DeviceContext_Map(Win32State->RenderContext, (ID3D11Resource *)Win32State->RenderInstanceBuffer, 
                                        0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
                vertex_instance *DataGPU = (vertex_instance *)MappedSubresource.pData;
                
                memcpy(DataGPU, VertexInstances, sizeof(vertex_instance) * CurrentVertexInstanceIndex);
                
                ID3D11DeviceContext_Unmap(Win32State->RenderContext, (ID3D11Resource *)Win32State->RenderInstanceBuffer, 0);
            }
            
            ID3D11DeviceContext_IASetInputLayout(Win32State->RenderContext, Win32State->RenderInputLayout);
            ID3D11DeviceContext_VSSetConstantBuffers(Win32State->RenderContext, 0, 1, &Win32State->RenderConstantBuffer);
            u32 Strides[] = { sizeof(v4), sizeof(vertex_instance) };
            u32 Offsets[] = { 0, 0 };
            
            ID3D11Buffer *VertexBuffers[] =
            {
                Win32State->RenderVertexBuffer,
                Win32State->RenderInstanceBuffer
            };
            
            ID3D11DeviceContext_IASetVertexBuffers(Win32State->RenderContext, 0, 2, VertexBuffers, Strides, Offsets);
            
            
            ID3D11DeviceContext_IASetPrimitiveTopology(Win32State->RenderContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            ID3D11DeviceContext_VSSetShader(Win32State->RenderContext, Win32State->RenderVertexShader, 0, 0);
            ID3D11DeviceContext_PSSetShader(Win32State->RenderContext, Win32State->RenderPixelShader, 0, 0);
            
            ID3D11DeviceContext_PSSetShaderResources(Win32State->RenderContext, 0, 1, &Win32State->RenderTextureView);
            ID3D11DeviceContext_PSSetSamplers(Win32State->RenderContext, 0, 1, &Win32State->RenderSamplerState);
            
            ID3D11DeviceContext_RSSetState(Win32State->RenderContext, Win32State->RenderRasterizerState);
            ID3D11DeviceContext_OMSetBlendState(Win32State->RenderContext, Win32State->RenderBlendState, 0, 0xffffffff);
            
            ID3D11DeviceContext_DrawInstanced(Win32State->RenderContext, 6, CurrentVertexInstanceIndex, 0, 0);
        }
        
    }
}

internal LRESULT CALLBACK
Win32WindowProc_(win32_state *Win32State, HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CREATE:
        {
            if(FAILED(Win32RenderCreate(Win32State)))
            {
                Win32LogError("Failed to create renderer");
                Result = -1;
            }
        } break;
        
        case WM_DESTROY:
        {
            Win32RenderDestroy(Win32State);
            Win32PostQuitMessage(0);
        } break;
        
        case WM_SIZE:
        {
            if(FAILED(Win32RenderResize(Win32State, LOWORD(LParam), HIWORD(LParam))))
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


internal LRESULT CALLBACK
Win32WindowProc(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result;
    
    win32_state *Win32State = (win32_state *)Win32GetWindowLongPtrW(Window, GWLP_USERDATA);
    
    if(Win32State)
    {
        Result = Win32WindowProc_(Win32State, Window, Message, WParam, LParam);
    }
    else
    {
        LogWarning("Unable to get win32 state, skipping message");
        Result = Win32DefWindowProcW(Window, Message, WParam, LParam);
    }
    
    return Result;
}

internal LRESULT CALLBACK
Win32WindowProcSetup(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result;
    
    win32_state *Win32State = 0;
    if(Message == WM_NCCREATE)
    {
        CREATESTRUCTW *Create = (CREATESTRUCTW *)LParam;
        Win32State = (win32_state *)Create->lpCreateParams;
        Win32SetWindowLongPtrW(Window, GWLP_USERDATA, (LONG_PTR)Win32State);
        Win32SetWindowLongPtrW(Window, GWLP_WNDPROC, (LONG_PTR)Win32WindowProc);
        Win32State->Window = Window;
        
        Result = Win32WindowProc_(Win32State, Window, Message, WParam, LParam);
    }
    else
    {
        Result = Win32DefWindowProcW(Window, Message, WParam, LParam);
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
#endif
    
    Platform = GlobalAppMemory.PlatformAPI;
    
    WNDCLASSEXW WindowClass =
    {
        .cbSize = sizeof(WindowClass),
        .lpfnWndProc = Win32WindowProcSetup,
        .hInstance = Instance,
        .hIcon = Win32LoadIconA(Instance, MAKEINTRESOURCE(IDI_ICON)),
        .hCursor = Win32LoadCursorA(NULL, IDC_ARROW),
        .lpszClassName = L"KengineWindowClass",
    };
    
    LogDebug("Registering %ls window class", WindowClass.lpszClassName);
    if(Win32RegisterClassExW(&WindowClass))
    {
        win32_state *Win32State = BootstrapPushStruct(win32_state, Arena);
        
        LogDebug("Creating window");
        HWND Window = Win32CreateWindowExW(WS_EX_APPWINDOW, WindowClass.lpszClassName, L"kengine",
                                           WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                           0, 0, WindowClass.hInstance, Win32State);
        if(Window)
        {
            LogDebug("Begin application loop");
            
            for(;;)
            {
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
                
                // NOTE(kstandbridge): This should be created each frame in the app layer
                {          
                    CurrentVertexInstanceIndex = 0;
                    
#if 0    
                    
#define BOX_WIDTH 60
#define BOX_HEIGHT 60
#define BOX_PADDING 5
                    u32 Columns = GlobalWindowWidth / (BOX_WIDTH + BOX_PADDING);
                    u32 Rows = GlobalWindowHeight / (BOX_HEIGHT + BOX_PADDING);
                    u32 AtX = BOX_PADDING;
                    u32 AtY = BOX_PADDING;
                    
                    for(u32 Row = 0;
                        Row < Rows;
                        ++Row)
                    {
                        for(u32 Column = 0;
                            Column < Columns;
                            ++Column)
                        {
                            PushVertexInstance(V2(AtX, AtY), V2(BOX_WIDTH, BOX_HEIGHT), V4(0.3f, 0.5f, 0.2f, 1.0f));
                            
                            AtX += BOX_WIDTH + BOX_PADDING;
                        }
                        AtX = BOX_PADDING;
                        AtY += BOX_HEIGHT + BOX_PADDING;
                    }
#else
                    
                    f32 AtX = 0.0f;
                    f32 AtY = Win32State->FontScale*Win32State->FontAscent;
                    string Text = String("Lorem Ipsum is simply dummy text of the printing and typesetting\nindustry. Lorem Ipsum has been the industry's standard dummy\ntext ever since the 1500s, when an unknown printer took a galley\nof type and scrambled it to make a type specimen book. It has\nsurvived not only five centuries, but also the leap into electronic\ntypesetting, remaining essentially unchanged. It was popularised in\nthe 1960s with the release of Letraset sheets containing Lorem\nIpsum passages, and more recently with desktop publishing\nsoftware like Aldus PageMaker including versions of Lorem Ipsum.");
                    u32 PreviousCodePoint = 0;
                    for(umm Index = 0;
                        Index < Text.Size;
                        ++Index)
                    {
                        u32 CodePoint = Text.Data[Index];
                        
                        if(CodePoint == '\n')
                        {
                            AtY += Win32State->FontScale*Win32State->FontAscent;
                            AtX = 0.0f;
                        }
                        else
                        {
                            glyph_info *Info = Win32State->GlyphInfos + CodePoint;
                            
                            Assert(Info->CodePoint == CodePoint);
                            
                            PushVertexInstance(V2(AtX, AtY + Info->YOffset), 
                                               V2(Info->Width, Info->Height), 
                                               V4(1.0f, 1.0f, 1.0f, 1.0f), 
                                               Info->UV);
                            
                            AtX += Win32State->FontScale*Info->AdvanceWidth;
                            
                            if(Index < Text.Size)
                            {
                                s32 Kerning = stbtt_GetCodepointKernAdvance(&Win32State->FontInfo, CodePoint, Text.Data[Index + 1]);
                                AtX += Win32State->FontScale*Kerning;
                            }
                        }
                        
                        PreviousCodePoint = CodePoint;
                    }
                    
#endif
                }
                
                Win32RenderFrame(Win32State);
                
                if(FAILED(Win32RenderPresent(Win32State)))
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
        
        ClearArena(&Win32State->Arena);
    }
    else
    {
        Win32LogError("Failed to register window class");
    }
    
    
    LogDebug("Exiting");
    
    PostTelemetryThread(0);
    
    return Result;
}