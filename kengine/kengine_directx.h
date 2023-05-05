#ifndef KENGINE_DIRECTX_H

typedef struct directx_state
{
    b32 RenderOccluded;
    IDXGISwapChain *RenderSwapChain;
    ID3D11Device *RenderDevice;
    ID3D11DeviceContext *RenderContext;
    ID3D11DeviceContext1 *RenderContext1;
    ID3D11RenderTargetView *RenderTargetView;
    HANDLE RenderFrameLatencyWait;
    ID3D11RasterizerState *RenderRasterizerState;
    ID3D11DepthStencilView *RenderDepthStencilView;
    ID3D11DepthStencilState *RenderDepthStencilState;
    
    ID3D11BlendState *RenderBlendState;
    
    ID3D11VertexShader *RenderVertexShader;
    ID3D11InputLayout *RenderInputLayout;
    ID3D11Buffer *RenderInstanceBuffer;
    
    ID3D11PixelShader *RenderGlyphPixelShader;
    
#if 0    
    ID3D11ShaderResourceView *RenderGlyphTextureView;
    ID3D11ShaderResourceView *RenderSpriteTextureView;
#endif
    
    ID3D11PixelShader *RenderSpritePixelShader;
    
    ID3D11PixelShader *RenderRectPixelShader;
    
    ID3D11Buffer *RenderVertexBuffer;
    ID3D11Buffer *RenderConstantBuffer;
    ID3D11SamplerState *RenderSamplerState;
    
} directx_state;

void *
DirectXLoadTexture(s32 TotalWidth, s32 TotalHeight, u32 *TextureBytes);


#define KENGINE_DIRECTX_H
#endif //KENGINE_DIRECTX_H
