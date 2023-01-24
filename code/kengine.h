#ifndef KENGINE_H

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

#include "kengine_platform.h"
#include "kengine_memory.h"
#if KENGINE_INTERNAL
#include "kengine_generated.h"
#endif
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#if KENGINE_INTERNAL
#include "kengine_debug_shared.h"
#include "kengine_debug.h"
#include "kengine_debug_generated.h"
#endif
#include "kengine_render_group.h"
#include "kengine_ui.h"


typedef enum properties_tab_type
{
    PropertiesTab_General,
    PropertiesTab_Sharing,
    PropertiesTab_Security,
    PropertiesTab_PreviousVersions,
    PropertiesTab_Customize,
} properties_tab_type;

typedef struct app_state
{
    memory_arena Arena;
    
    f32 SomeValue;
    u32 SelectedIndex;
    
    properties_tab_type SelectedPropertyTab;
    
} app_state;

#define KENGINE_H
#endif //KENGINE_H