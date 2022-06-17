#ifndef KENGINE_H

/* TODO(kstandbridge): 

- test change
- DrawMultilineTextbox should only be drawing, any updates to the state need to be done in enduiframe. Perhaps split the textbox into seperate interactions, up/down buttons, scroll bars, is mousewheel an interaction?
- add padding to all sides of view ports
- draw text pass selectstart/end and color back/forground accordingly
- editable_string ScrollOffset, if textbounds>dim then add scrolloffset to dim...

- del/backspace should clear the selection
- textbox home/end
- textbox ctrl+Shift home+end (find space)
- textbox ctrl left+right (find space)
- textbox ctrl+Shift left+right (find space)
- radio linked to enum?
- dropdown linked to enum?
- dropdown could do enum to string conversation func or similar?
- listview
- meta linked list
- meta double linked list
- meta free list
- fix: FormatString ex: %f,%f
- limited number of glyphs, perhaps with a lookup table of codepoint to index
- introspect method names remove underscores
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

- Smooth font resources:
-> https://github.com/ocornut/imgui/issues/2468
 -> https://www.grc.com/ct/ctwhat.htm

- we need a way to dump a struct for debugging purposes, I need to see values frame by frame, introspect to get the values to output, can be preprocessed
- pulse based on deltatime, we could use it for the caret to fade in and out, and such animations
- rolling string collection, use two arenas, when prim is half full write to prim and sec, when prim is full swap prim and sec, now wipe the origional prim. We'd always have 50% of messages being displayed and can continuelessly push to it.
- add debug toggle with f12.
- debug console
- debug selected element details?
- debug hot interaction details?
- debug show fps, cpu usage, thread details, graph out how long each function is taking
- we need to profile this, are we better off having a seperate thread draw each control, or split the screen up into sections to be drawn.... what is taking all them cycles?

632,0   > 632,340
632,340 > 632,340
421,340 > 842,340
210,170 > 210,340
210,170 > 0,340
210,170 > 210,510
210,170 > 0,510
421,340 > 0,340
632,340 > 632,0
632,340 > 0,0
421,340 > 842,340
210,170 > 210,340
210,170 > 0,340
210,170 > 210,510
210,170 > 0,510

*/




#include "kengine_platform.h"
#include "kengine_shared.h"
#include "kengine_intrinsics.h"
#include "kengine_generated.h"
#include "kengine_math.h"
#include "kengine_render_group.h"
#include "kengine_assets.h"
#include "kengine_ui.h"
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
    ui_state UiState;
    
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
