#ifndef KENGINE_H

/* TODO(kstandbridge): 

- meta linked list
- meta double linked list
- meta free list
- fix: FormatString ex: %f,%f
- Format string c	Character
- Format string o	Unsigned octal
- Format string x	Unsigned hexadecimal integer
- Format string X	Unsigned hexadecimal integer (uppercase)
- Format string F	Decimal floating point, uppercase
- Format string e	Scientific notation (mantissa/exponent), lowercase
- Format string E	Scientific notation (mantissa/exponent), uppercase
- Format string g	Use the shortest representation: %e or %f
- Format string G	Use the shortest representation: %E or %F
- Format string a	Hexadecimal floating point, lowercase
- Format string A	Hexadecimal floating point, uppercase
- Format string p	Pointer address
- Format string n	Nothing printed.
- Format string flags
- Format string width
- introspect method names remove underscores

*/

#include "kengine_platform.h"
#include "kengine_shared.h"
#include "kengine_intrinsics.h"
#include "kengine_generated.h"
#include "kengine_math.h"
#include "kengine_render_group.h"
#include "kengine_assets.h"
#include "kengine_ui.h"

typedef struct app_state
{
    b32 IsInitialized;
    
    memory_arena PermanentArena;
    memory_arena TransientArena;
    
    ui_state UiState;
    assets Assets;
    
    f32 Time;
    loaded_bitmap TestBMP;
    loaded_bitmap TestFont;
    
    v2 TestP;
    s32 TestCounter;
    // TODO(kstandbridge): Switch to a float
    v2 UiScale;
    
} app_state;

#define KENGINE_H
#endif //KENGINE_H
