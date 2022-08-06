#include "kengine.h"

global platform_api Platform;

#include "kengine_generated.c"
#include "kengine_sort.c"
#include "kengine_renderer.c"

extern void
AppUpdateFrame(platform_api PlatformAPI, render_commands *Commands)
{
    Platform = PlatformAPI;
    
    render_group RenderGroup_;
    render_group *RenderGroup = &RenderGroup_;
    RenderGroup->Commands = Commands;
    
    PushRenderCommandClear(RenderGroup, 1.0f, V4(1.0f, 0.5f, 0.0f, 1.0f));
    
    f32 Color = 0.1f;
    for(f32 P = 10.0f;
        P < 1150.0f;
        P += 100.0f, Color += 0.1f)
    {
        PushRenderCommandRectangle(RenderGroup, V4(Color, Color, Color, 1.0f),
                                   Rectangle2(V2(P, P*0.5f), V2(P + 90.0f, P*0.5f + 90.0f)), P);
    }
}
