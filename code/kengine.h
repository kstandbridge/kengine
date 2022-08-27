#ifndef KENGINE_H

#include "kengine_platform.h"
#include "kengine_generated.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#if KENGINE_INTERNAL
#include "kengine_debug_shared.h"
#include "kengine_debug.h"
#include "kengine_debug_generated.h"
#endif
#include "kengine_renderer_shared.h"
#include "kengine_render_group.h"
#include "kengine_ui.h"

typedef struct app_state
{
    memory_arena TranArena;
    
    loaded_glyph Glyphs[255];
    
    ui_state UIState;
} app_state;

#define KENGINE_H
#endif //KENGINE_H