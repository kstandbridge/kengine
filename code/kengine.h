#ifndef KENGINE_H

/* TODO(kstandbridge): 

- fix: selection exists left/right should go to selection start/end
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
- 

*/

#include "kengine_platform.h"
#include "kengine_shared.h"
#include "kengine_intrinsics.h"
#include "kengine_generated.h"
#include "kengine_math.h"
#include "kengine_render_group.h"
#include "kengine_assets.h"
#include "kengine_ui.h"

typedef struct colors
{
    v4 Clear;
    
    v4 Text;
    v4 TextBorder;
    v4 SelectedTextBorder;
    v4 SelectedTextBackground;
    v4 SelectedText;
    
    v4 SelectedOutline;
    v4 SelectedOutlineAlt;
    
    v4 CheckBoxBorder;
    v4 CheckBoxBackground;
    v4 CheckBoxBorderClicked;
    v4 CheckBoxBackgroundClicked;
    
    v4 TextBackground;
    v4 Caret;
    
    v4 HotButton;
    v4 Button;
    v4 ClickedButton;
    v4 ButtonBorder;
    
} colors;

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
    
    ui_state UiState;
    assets Assets;
    
    f32 Time;
    loaded_bitmap TestBMP;
    loaded_bitmap TestFont;
    
    editable_string TestString;
    v2 TestP;
    s32 TestCounter;
    // TODO(kstandbridge): Switch to a float
    v2 UiScale;
    b32 TestBoolean;
    
    fruit_type TestEnum;
    
} app_state;

#define KENGINE_H
#endif //KENGINE_H
