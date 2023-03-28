
global directx_state GlobalDirectXState;

// TODO(kstandbridge): Put this in win32_state;
global s32 GlobalWindowWidth;
global s32 GlobalWindowHeight;

v2
TransformV2(m4x4 Matrix, v2 Vector)
{
    v2 Result;
    
    Result.X = Vector.X * Matrix.E[0][0] + Vector.Y * Matrix.E[0][1] + Matrix.E[0][3];
    Result.Y = Vector.X * Matrix.E[1][0] + Vector.Y * Matrix.E[1][1] + Matrix.E[1][3];
    
    return Result;
}

internal m4x4
M4x4OrthographicOffCenterLH(f32 ViewLeft,
                            f32 ViewRight,
                            f32 ViewBottom,
                            f32 ViewTop,
                            f32 NearZ,
                            f32 FarZ)
{
    f32 ReciprocalWidth = 1.0f / (ViewRight - ViewLeft);
    f32 ReciprocalHeight = 1.0f / (ViewTop - ViewBottom);
    f32 fRange = 1.0f / (FarZ - NearZ);
    
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

typedef struct constant
{
    m4x4 Transform;
} constant;


internal HRESULT
DirectXRenderCreate()
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
        
        if(FAILED(HResult = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, Flags, 0, 0, D3D11_SDK_VERSION,
                                              &GlobalDirectXState.RenderDevice, &FeatureLevel, &GlobalDirectXState.RenderContext)))
        {
            LogWarning("Failed to create D3D11 hardware device, attempting software mode");
            if(FAILED(HResult = D3D11CreateDevice(0, D3D_DRIVER_TYPE_WARP, 0, Flags, 0, 0, D3D11_SDK_VERSION,
                                                  &GlobalDirectXState.RenderDevice, &FeatureLevel, &GlobalDirectXState.RenderContext)))
            {
                Win32LogError_(HResult, "Failed to create D3D11 software device");
            }
        }
        
        if(SUCCEEDED(HResult))
        {
            if(SUCCEEDED(HResult = ID3D11DeviceContext_QueryInterface(GlobalDirectXState.RenderContext, &IID_ID3D11DeviceContext1, &GlobalDirectXState.RenderContext1)))
            {
                LogDebug("Using ID3D11DeviceContext1");
                ID3D11DeviceContext_Release(GlobalDirectXState.RenderContext);
                GlobalDirectXState.RenderContext = (ID3D11DeviceContext *)GlobalDirectXState.RenderContext1;
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
        if(SUCCEEDED(HResult = CreateDXGIFactory(&IID_IDXGIFactory, &Factory)))
        {
            DXGI_SWAP_CHAIN_DESC SwapChainDesc = 
            {
                .BufferDesc =
                {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
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
                .OutputWindow = GlobalAppMemory.Window,
                .Windowed = TRUE,
            };
            
            SwapChainDesc.BufferCount = 2;
            SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            SwapChainDesc.Flags = 0;
            if(FAILED(HResult = 
                      IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)GlobalDirectXState.RenderDevice, &SwapChainDesc, &GlobalDirectXState.RenderSwapChain)))
            {
                LogDebug("Creating flip discard swap chain failed, attempting flip sequential");
                SwapChainDesc.BufferCount = 2;
                SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                if(FAILED(HResult =
                          IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)GlobalDirectXState.RenderDevice, &SwapChainDesc, &GlobalDirectXState.RenderSwapChain)))
                {
                    LogDebug("Creating flip sequential swap chain failed, attempting discard");
                    SwapChainDesc.BufferCount = 1;
                    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                    SwapChainDesc.Flags = 0;
                    if(SUCCEEDED(HResult =
                                 IDXGIFactory_CreateSwapChain(Factory, (IUnknown *)GlobalDirectXState.RenderDevice, &SwapChainDesc, &GlobalDirectXState.RenderSwapChain)))
                    {
                        IDXGISwapChain2 *SwapChain2;
                        if(SUCCEEDED(HResult = 
                                     IDXGISwapChain_QueryInterface(GlobalDirectXState.RenderSwapChain, &IID_IDXGISwapChain2, &SwapChain2)))
                        {
                            LogDebug("Using IDXGISwapChain2 for frame latency control");
                            GlobalDirectXState.RenderFrameLatencyWait = IDXGISwapChain2_GetFrameLatencyWaitableObject(SwapChain2);
                            IDXGISwapChain2_Release(SwapChain2);
                        }
                    }
                }
            }
            
            if(SUCCEEDED(HResult))
            {
                if(FAILED(HResult = IDXGIFactory_MakeWindowAssociation(Factory, GlobalAppMemory.Window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER)))
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
                  ID3D11Device_CreateRasterizerState(GlobalDirectXState.RenderDevice, &RasterizerDesc, &GlobalDirectXState.RenderRasterizerState)))
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
                  ID3D11Device_CreateDepthStencilState(GlobalDirectXState.RenderDevice, &DepthStencilDesc, &GlobalDirectXState.RenderDepthStencilState)))
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
                  ID3D11Device_CreateBlendState(GlobalDirectXState.RenderDevice, &BlendDesc, &GlobalDirectXState.RenderBlendState)))
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
                     ID3D11Device_CreateVertexShader(GlobalDirectXState.RenderDevice, VertexShaderData, VertexShaderDataSize, 0, &GlobalDirectXState.RenderVertexShader)))
        {
            if(FAILED(HResult =
                      ID3D11Device_CreateInputLayout(GlobalDirectXState.RenderDevice, InputElementDesc, ArrayCount(InputElementDesc),
                                                     VertexShaderData, sizeof(VertexShaderData), &GlobalDirectXState.RenderInputLayout)))
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
                  ID3D11Device_CreatePixelShader(GlobalDirectXState.RenderDevice, GlyphPixelShaderData, GlyphPixelShaderDataSize, 0, &GlobalDirectXState.RenderGlyphPixelShader)))
        {
            Win32LogError_(HResult, "Failed to create glyph pixel shader");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating sprite pixel shader");
        size_t SpritePixelShaderDataSize = sizeof(SpritePixelShaderData);
        if(FAILED(HResult =
                  ID3D11Device_CreatePixelShader(GlobalDirectXState.RenderDevice, SpritePixelShaderData, SpritePixelShaderDataSize, 0, &GlobalDirectXState.RenderSpritePixelShader)))
        {
            Win32LogError_(HResult, "Failed to create sprite pixel shader");
        }
    }
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating rect pixel shader");
        size_t RectPixelShaderDataSize = sizeof(RectPixelShaderData);
        if(FAILED(HResult =
                  ID3D11Device_CreatePixelShader(GlobalDirectXState.RenderDevice, RectPixelShaderData, RectPixelShaderDataSize, 0, &GlobalDirectXState.RenderRectPixelShader)))
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
                  ID3D11Device_CreateBuffer(GlobalDirectXState.RenderDevice, &BufferDesc, &SubresourceData, &GlobalDirectXState.RenderVertexBuffer)))
        {
            Win32LogError_(HResult, "Failed to create vertex buffer");
        }
    }
    
    if(SUCCEEDED(HResult))
    {
        LogDebug("Creating vertex instance buffer");
        D3D11_BUFFER_DESC BufferDesc =
        {
            .ByteWidth = VertexBufferSize,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        
        if(FAILED(HResult =
                  ID3D11Device_CreateBuffer(GlobalDirectXState.RenderDevice, &BufferDesc, 0, &GlobalDirectXState.RenderInstanceBuffer)))
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
                  ID3D11Device_CreateBuffer(GlobalDirectXState.RenderDevice, &BufferDesc, 0, &GlobalDirectXState.RenderConstantBuffer)))
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
                  ID3D11Device_CreateSamplerState(GlobalDirectXState.RenderDevice, &SamplerDesc, &GlobalDirectXState.RenderSamplerState)))
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
        // TODO(kstandbridge): free TextureBytes
        u8 *TextureBytes = stbi_load("directx_hello\\test_tree.bmp", &TotalWidth, &TotalHeight,
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
                     ID3D11Device_CreateTexture2D(GlobalDirectXState.RenderDevice, &TextureDesc, &SubresourceData, &Texture)))
        {
            
            if(FAILED(HResult =
                      ID3D11Device_CreateShaderResourceView(GlobalDirectXState.RenderDevice, (ID3D11Resource *)Texture, 0, &GlobalDirectXState.RenderSpriteTextureView)))
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
    
    return HResult;
}

#define D3D11SafeRelease(Release, Obj) if (Obj) { Release##_Release(Obj); }

internal void
DirectXRenderDestroy()
{
    LogDebug("Destroying renderer");
    
    if(GlobalDirectXState.RenderContext)
    {
        ID3D11DeviceContext_ClearState(GlobalDirectXState.RenderContext);
    }
    
    D3D11SafeRelease(ID3D11Buffer, GlobalDirectXState.RenderConstantBuffer);
    D3D11SafeRelease(ID3D11Buffer, GlobalDirectXState.RenderInstanceBuffer);
    D3D11SafeRelease(ID3D11Buffer, GlobalDirectXState.RenderVertexBuffer);
    D3D11SafeRelease(ID3D11InputLayout, GlobalDirectXState.RenderInputLayout);
    D3D11SafeRelease(ID3D11VertexShader, GlobalDirectXState.RenderVertexShader);
    
    D3D11SafeRelease(ID3D11ShaderResourceView, GlobalDirectXState.RenderGlyphTextureView);
    D3D11SafeRelease(ID3D11PixelShader, GlobalDirectXState.RenderGlyphPixelShader);
    
    D3D11SafeRelease(ID3D11ShaderResourceView, GlobalDirectXState.RenderSpriteTextureView);
    D3D11SafeRelease(ID3D11PixelShader, GlobalDirectXState.RenderSpritePixelShader);
    
    D3D11SafeRelease(ID3D11PixelShader, GlobalDirectXState.RenderRectPixelShader);
    
    D3D11SafeRelease(ID3D11RasterizerState, GlobalDirectXState.RenderRasterizerState);
    D3D11SafeRelease(ID3D11DepthStencilState, GlobalDirectXState.RenderDepthStencilState);
    D3D11SafeRelease(ID3D11BlendState, GlobalDirectXState.RenderBlendState);
    D3D11SafeRelease(ID3D11DepthStencilView, GlobalDirectXState.RenderDepthStencilView);
    D3D11SafeRelease(ID3D11RenderTargetView, GlobalDirectXState.RenderTargetView);
    D3D11SafeRelease(ID3D11Device, GlobalDirectXState.RenderDevice);
    D3D11SafeRelease(ID3D11DeviceContext, GlobalDirectXState.RenderContext);
    D3D11SafeRelease(IDXGISwapChain, GlobalDirectXState.RenderSwapChain);
    D3D11SafeRelease(ID3D11SamplerState, GlobalDirectXState.RenderSamplerState);
    
    GlobalDirectXState.RenderContext1 = 0;
    GlobalDirectXState.RenderFrameLatencyWait = 0;
}

internal HRESULT
DirectXRecreateDevice()
{
    HRESULT Result;
    
    LogDebug("Recreating renderer");
    DirectXRenderDestroy();
    if(FAILED(Result = DirectXRenderCreate()))
    {
        LogDebug("Recreating renderer");
    }
    
    return Result;
}

inline HRESULT
DirectXFatalDeviceLostError()
{
    HRESULT Result = E_FAIL;
    MessageBoxW(0, L"Cannot recreate D3D11 device, it is reset or removed!", L"Error", MB_ICONEXCLAMATION);
    return Result;
}

internal HRESULT
DirectXRenderResize(s32 Width, s32 Height)
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
        
        if(GlobalDirectXState.RenderTargetView)
        {
            ID3D11DeviceContext_OMSetRenderTargets(GlobalDirectXState.RenderContext, 0, 0, 0);
            ID3D11RenderTargetView_Release(GlobalDirectXState.RenderTargetView);
            GlobalDirectXState.RenderTargetView = 0;
        }
        
        if(GlobalDirectXState.RenderDepthStencilView)
        {
            ID3D11DepthStencilView_Release(GlobalDirectXState.RenderDepthStencilView);
            GlobalDirectXState.RenderDepthStencilView = 0;
        }
        
        LogDebug("Resizing buffers");
        Result = IDXGISwapChain_ResizeBuffers(GlobalDirectXState.RenderSwapChain, 0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
        if((Result == DXGI_ERROR_DEVICE_REMOVED) ||
           (Result == DXGI_ERROR_DEVICE_RESET) ||
           (Result == DXGI_ERROR_DRIVER_INTERNAL_ERROR))
        {
            if(FAILED(DirectXRecreateDevice()))
            {
                Result = DirectXFatalDeviceLostError();
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
                         IDXGISwapChain_GetBuffer(GlobalDirectXState.RenderSwapChain, 0, &IID_ID3D11Texture2D, &WindowBuffer)))
            {
                if(FAILED(Result =
                          ID3D11Device_CreateRenderTargetView(GlobalDirectXState.RenderDevice, (ID3D11Resource *)WindowBuffer, 0, &GlobalDirectXState.RenderTargetView)))
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
                         ID3D11Device_CreateTexture2D(GlobalDirectXState.RenderDevice, &DepthStencilDesc, 0, &DepthStencil)))
            {
                if(FAILED(Result =
                          ID3D11Device_CreateDepthStencilView(GlobalDirectXState.RenderDevice, (ID3D11Resource *)DepthStencil, 0,
                                                              &GlobalDirectXState.RenderDepthStencilView)))
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
            ID3D11DeviceContext_RSSetViewports(GlobalDirectXState.RenderContext, 1, &Viewport);
        }
    }
    
    return Result;
}

internal HRESULT
DirectXRenderPresent()
{
    HRESULT Result = S_OK;
    
    if(GlobalDirectXState.RenderOccluded)
    {
        Result = IDXGISwapChain_Present(GlobalDirectXState.RenderSwapChain, 0, DXGI_PRESENT_TEST);
        if((SUCCEEDED(Result)) &&
           (Result != DXGI_STATUS_OCCLUDED))
        {
            LogDebug("DXGI window is back to normal, resuming rendering");
            GlobalDirectXState.RenderOccluded = false;
        }
    }
    
    if(!GlobalDirectXState.RenderOccluded)
    {
        Result = IDXGISwapChain_Present(GlobalDirectXState.RenderSwapChain, 1, 0);
    }
    
    if((Result == DXGI_ERROR_DEVICE_RESET) ||
       (Result == DXGI_ERROR_DEVICE_REMOVED))
    {
        LogDebug("Device reset or removed, recreating");
        if(FAILED(DirectXRecreateDevice()))
        {
            Result = DirectXFatalDeviceLostError();
        }
        else
        {
            RECT Rect;
            if(!GetClientRect(GlobalAppMemory.Window, &Rect))
            {
                LogError("Failed to get window rect");
            }
            else
            {
                DirectXRenderResize(Rect.right - Rect.left, Rect.bottom - Rect.top);
            }
        }
    }
    else if(Result == DXGI_STATUS_OCCLUDED)
    {
        LogDebug("DXGI window is occluded, skipping rendering");
        GlobalDirectXState.RenderOccluded = true;
    }
    else if(FAILED(Result))
    {
        Win32LogError_(Result, "Swap chain present failed");
    }
    
    if(GlobalDirectXState.RenderOccluded)
    {
        Sleep(10);
    }
    else
    {
        if(GlobalDirectXState.RenderContext1)
        {
            ID3D11DeviceContext1_DiscardView(GlobalDirectXState.RenderContext1, (ID3D11View *)GlobalDirectXState.RenderTargetView);
        }
    }
    
    return Result;
}

internal void
DirectXRenderFrame(render_group *RenderGroup)
{
    if(!GlobalDirectXState.RenderOccluded)
    {
        if(GlobalDirectXState.RenderFrameLatencyWait)
        {
            WaitForSingleObjectEx(GlobalDirectXState.RenderFrameLatencyWait, INFINITE, TRUE);
        }
        
        // NOTE(kstandbridge): BeginDraw!
        {        
            ID3D11DeviceContext_OMSetRenderTargets(GlobalDirectXState.RenderContext, 1, &GlobalDirectXState.RenderTargetView, GlobalDirectXState.RenderDepthStencilView);
            ID3D11DeviceContext_ClearDepthStencilView(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderDepthStencilView, 
                                                      D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            ID3D11DeviceContext_OMSetDepthStencilState(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderDepthStencilState, 1);
        }
        
        {        
            // NOTE(kstandbridge): Clear background
            v4 Color = V4(0.1f, 0.2f, 0.6f, 1.0f);
            ID3D11DeviceContext_ClearRenderTargetView(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderTargetView,
                                                      (f32 *)&Color);
        }
        
        // NOTE(kstandbridge): Map the constant buffer which has our orthographic matrix in
        // TODO(kstandbridge): only when its dirty!
        {            
            D3D11_MAPPED_SUBRESOURCE MappedSubresource;
            ID3D11DeviceContext_Map(GlobalDirectXState.RenderContext, (ID3D11Resource *)GlobalDirectXState.RenderConstantBuffer, 
                                    0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
            constant *Constants = (constant *)MappedSubresource.pData;
            Constants->Transform = M4x4Orthographic((f32)GlobalWindowWidth, (f32)GlobalWindowHeight, 10.0f, 0.0f);
            //Constants->Transform = M4x4Orthographic(GlobalWindowWidth, GlobalWindowHeight, 0.0f, 1.0f);
            //Constants->Transform = M4x4OrthographicOffCenterLH(0, GlobalWindowWidth, GlobalWindowHeight, 0, 0.0f, 1.0f);
            ID3D11DeviceContext_Unmap(GlobalDirectXState.RenderContext, (ID3D11Resource *)GlobalDirectXState.RenderConstantBuffer, 0);
        }
        
        {        
            ID3D11DeviceContext_IASetInputLayout(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderInputLayout);
            ID3D11DeviceContext_VSSetConstantBuffers(GlobalDirectXState.RenderContext, 0, 1, &GlobalDirectXState.RenderConstantBuffer);
            u32 Strides[] = { sizeof(v4), sizeof(vertex_instance) };
            u32 Offsets[] = { 0, 0 };
            
            ID3D11Buffer *VertexBuffers[] =
            {
                GlobalDirectXState.RenderVertexBuffer,
                GlobalDirectXState.RenderInstanceBuffer
            };
            
            ID3D11DeviceContext_IASetVertexBuffers(GlobalDirectXState.RenderContext, 0, 2, VertexBuffers, Strides, Offsets);
            
            ID3D11DeviceContext_IASetPrimitiveTopology(GlobalDirectXState.RenderContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            ID3D11DeviceContext_VSSetShader(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderVertexShader, 0, 0);
            
            
            ID3D11DeviceContext_PSSetSamplers(GlobalDirectXState.RenderContext, 0, 1, &GlobalDirectXState.RenderSamplerState);
            
            ID3D11DeviceContext_RSSetState(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderRasterizerState);
            ID3D11DeviceContext_OMSetBlendState(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderBlendState, 0, 0xffffffff);
            
        }
        
        // NOTE(kstandbridge): Iterate commands
        {
            for(u32 CommandIndex = 0;
                CommandIndex <= RenderGroup->CurrentCommand;
                ++CommandIndex)
            {
                render_command *Command = RenderGroup->Commands + CommandIndex;
                
                switch(Command->Type)
                {
                    case RenderCommand_Rect:
                    {
                        D3D11_MAPPED_SUBRESOURCE MappedSubresource;
                        ID3D11DeviceContext_Map(GlobalDirectXState.RenderContext, (ID3D11Resource *)GlobalDirectXState.RenderInstanceBuffer, 
                                                0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
                        vertex_instance *DataGPU = (vertex_instance *)MappedSubresource.pData;
                        
                        u8 *VertexBuffer = RenderGroup->VertexBuffer;
                        VertexBuffer += Command->VertexBufferOffset;
                        
                        umm SizeInBytes = VertexInstanceSize * Command->VertexCount;
                        Assert(SizeInBytes < VertexBufferSize);
                        
                        memcpy(DataGPU, VertexBuffer, SizeInBytes);
                        
                        ID3D11DeviceContext_Unmap(GlobalDirectXState.RenderContext, (ID3D11Resource *)GlobalDirectXState.RenderInstanceBuffer, 0);
                        
                        ID3D11DeviceContext_PSSetShader(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderRectPixelShader, 0, 0);
                        
                        ID3D11DeviceContext_DrawInstanced(GlobalDirectXState.RenderContext, 6, Command->VertexCount, 0, 0);
                        
                    } break;
                    
                    case RenderCommand_Sprite:
                    {
                        D3D11_MAPPED_SUBRESOURCE MappedSubresource;
                        ID3D11DeviceContext_Map(GlobalDirectXState.RenderContext, (ID3D11Resource *)GlobalDirectXState.RenderInstanceBuffer, 
                                                0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
                        vertex_instance *DataGPU = (vertex_instance *)MappedSubresource.pData;
                        
                        u8 *VertexBuffer = RenderGroup->VertexBuffer;
                        VertexBuffer += Command->VertexBufferOffset;
                        
                        memcpy(DataGPU, VertexBuffer, VertexInstanceSize * Command->VertexCount);
                        
                        ID3D11DeviceContext_Unmap(GlobalDirectXState.RenderContext, (ID3D11Resource *)GlobalDirectXState.RenderInstanceBuffer, 0);
                        
                        ID3D11DeviceContext_PSSetShaderResources(GlobalDirectXState.RenderContext, 0, 1, &GlobalDirectXState.RenderSpriteTextureView);
                        
                        ID3D11DeviceContext_PSSetShader(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderSpritePixelShader, 0, 0);
                        
                        ID3D11DeviceContext_DrawInstanced(GlobalDirectXState.RenderContext, 6, Command->VertexCount, 0, 0);
                    } break;
                    
                    case RenderCommand_Glyph:
                    {
                        D3D11_MAPPED_SUBRESOURCE MappedSubresource;
                        ID3D11DeviceContext_Map(GlobalDirectXState.RenderContext, (ID3D11Resource *)GlobalDirectXState.RenderInstanceBuffer, 
                                                0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
                        vertex_instance *DataGPU = (vertex_instance *)MappedSubresource.pData;
                        
                        u8 *VertexBuffer = RenderGroup->VertexBuffer;
                        VertexBuffer += Command->VertexBufferOffset;
                        
                        memcpy(DataGPU, VertexBuffer, VertexInstanceSize * Command->VertexCount);
                        
                        ID3D11DeviceContext_Unmap(GlobalDirectXState.RenderContext, (ID3D11Resource *)GlobalDirectXState.RenderInstanceBuffer, 0);
                        
                        ID3D11DeviceContext_PSSetShaderResources(GlobalDirectXState.RenderContext, 0, 1, &GlobalDirectXState.RenderGlyphTextureView);
                        
#if 0
                        ID3D11DeviceContext_PSSetShader(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderGlyphPixelShader, 0, 0);
#else
                        ID3D11DeviceContext_PSSetShader(GlobalDirectXState.RenderContext, GlobalDirectXState.RenderSpritePixelShader, 0, 0);
#endif
                        
                        ID3D11DeviceContext_DrawInstanced(GlobalDirectXState.RenderContext, 6, Command->VertexCount, 0, 0);
                        
                    } break;
                }
                
            }
        }
    }
}

internal void
DirectXLoadTexture(s32 TotalWidth, s32 TotalHeight, u32 *TextureBytes)
{
    s32 TextureBytesPerRow = 4 * TotalWidth;
    
    HRESULT HResult = S_OK;
    
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
                 ID3D11Device_CreateTexture2D(GlobalDirectXState.RenderDevice, &TextureDesc, &SubresourceData, &Texture)))
    {
        
        if(FAILED(HResult =
                  ID3D11Device_CreateShaderResourceView(GlobalDirectXState.RenderDevice, (ID3D11Resource *)Texture, 0, &GlobalDirectXState.RenderGlyphTextureView)))
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
