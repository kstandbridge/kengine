#include "kengine.h"

global platform_api Platform;

#include "kengine_generated.c"
#include "kengine_sort.c"
#include "kengine_render_group.c"

extern void
AppUpdateFrame(platform_api PlatformAPI, render_commands *Commands)
{
    Platform = PlatformAPI;
    
    render_group RenderGroup_;
    ZeroStruct(RenderGroup_);
    render_group *RenderGroup = &RenderGroup_;
    RenderGroup->Commands = Commands;
    RenderGroup->CurrentClipRectIndex = PushRenderCommandClipRectangle(RenderGroup, Rectangle2i(0, Commands->Width, 0, Commands->Height));
    
    PushRenderCommandClear(RenderGroup, 0.0f, V4(0.0f, 0.0f, 0.0f, 1.0f));
    
    f32 Width = (f32)Commands->Width;
    f32 Height = (f32)Commands->Height;
    
    u16 OldClipRect = RenderGroup->CurrentClipRectIndex;
    RenderGroup->CurrentClipRectIndex =
        PushRenderCommandClipRectangle(RenderGroup, Rectangle2i((s32)(Width*0.25f), (s32)(Width*0.75f), 
                                                                (s32)(Height*0.25f), (s32)(Height*0.75f)));
    
    PushRenderCommandRectangle(RenderGroup, V4(1.0f, 0.0f, 0.0f, 1.0f),
                               Rectangle2(V2(0.0f, 0.0f), V2(Width*0.5f, Height*0.5f)), 1.0f);
    
    PushRenderCommandRectangle(RenderGroup, V4(0.0f, 1.0f, 0.0f, 1.0f),
                               Rectangle2(V2(Width*0.5f, 0.0f), V2(Width, Height*0.5f)), 1.0f);
    
    PushRenderCommandRectangle(RenderGroup, V4(0.0f, 0.0f, 1.0f, 1.0f),
                               Rectangle2(V2(0.0f, Height*0.5f), V2(Width*0.5f, Height)), 1.0f);
    
    PushRenderCommandRectangle(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f),
                               Rectangle2(V2(Width*0.5f, Height*0.5f), V2(Width, Height)), 1.0f);
    RenderGroup->CurrentClipRectIndex = OldClipRect;
}
