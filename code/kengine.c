#include "kengine.h"

#include "kengine_generated.c"
#include "kengine_sort.c"
#include "kengine_renderer.c"

extern void
AppUpdateFrame(render_commands *Commands)
{
    render_group RenderGroup_;
    render_group *RenderGroup = &RenderGroup_;
    RenderGroup->Commands = Commands;
    
    PushRenderCommandClear(RenderGroup, 1.0f, V4(0.0f, 0.5f, 1.0f, 1.0f));
    
    
    PushRenderCommandRectangle(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f),
                               Rectangle2(V2(10.0f, 10.0f), V2(100.0f, 100.0f)), 2.0f);
    
    
}

void __stdcall
_DllMainCRTStartup()
{
    
}