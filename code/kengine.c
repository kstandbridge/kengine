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
    
    PushRenderEntryClear(RenderGroup, V4(1.0f, 0.5f, 0.0f, 1.0f), 1.0f);
}

void __stdcall
_DllMainCRTStartup()
{
    
}