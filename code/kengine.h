#ifndef KENGINE_H

#include "kengine_platform.h"
#include "kengine_shared.h"
#include "kengine_intrinsics.h"
#include "kengine_generated.h"
#include "kengine_math.h"
#include "kengine_render_group.h"
#include "kengine_assets.h"
#include "kengine_interface.h"
#include "kengine_debug.h"

typedef enum fruit_type
{
    Fruit_Unkown,
    Fruit_Apple,
    Fruit_Banana,
    Fruit_Orange,
    
    Fruit_Count
} fruit_type;

typedef struct app_state
{
    b32 IsInitialized;
    
    memory_arena PermanentArena;
    memory_arena TransientArena;
    
    //ui_state UiState;
    
    assets Assets;
    interface_state InterfaceState;
    
    f32 Time;
    
    b32 ShowEmptyWorlds;
    b32 ShowLocalWorlds;
    b32 ShowAvailableWorlds;
    b32 EditRunParams;
    editable_string FilterText;
    editable_string LaunchParams;
    
    loaded_bitmap TestBMP;
    loaded_bitmap TestFont;
    
    
    v2 TestP;
    s32 TestCounter;
    // TODO(kstandbridge): Switch to a float
    v2 UiScale;
    b32 TestBoolean;
    
    
    fruit_type TestEnum;
    
} app_state;

#define KENGINE_H
#endif //KENGINE_H
