#ifndef WIN32_D3D_H
#include "kengine_platform.h"
#include "kengine_memory.h"
#include "kengine_generated.h"
#include "win32_kengine_types.h"
#include "win32_resource.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"

typedef struct win32_window_state
{
    // NOTE(kstandbridge): Windows stuff
    HINSTANCE Instance;
    char *ClassName;
    HWND Hwnd;
    
    // NOTE(kstandbridge): Direct3D stuff
    ID3D11Device *Device;
    IDXGISwapChain *SwapChain;
    ID3D11DeviceContext *DeviceContext;
    ID3D11RenderTargetView *RenderTargetView;
    ID3D11DepthStencilView *DepthStencilView;
    
    // NOTE(kstandbridge): Misc
    s32 Width;
    s32 Height;
    b32 MouseInWindow;
    b32 IsRunning;
    v2 MouseP;
    
    
} win32_window_state;

#define WIN32_D3D_H
#endif //WIN32_D3D_H
