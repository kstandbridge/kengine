#ifndef KENGINE_H

/* TODO(kstandbridge): 

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
- 

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

typedef struct colors
{
    // NOTE(kstandbridge): For now I'm intentionally creating way more colors than needed.
    // They can be consolidated once I get a better idea of the common colors used on controls.
    // This could be done after the dark mode pass?
    // TODO(kstandbridge): Consolidate colors
    
    v4 Clear;
    
    v4 LabelBackground;
    v4 LabelBorder;
    v4 LabelText;
    
    v4 CheckboxBackground;
    v4 CheckboxBorder;
    v4 CheckboxText;
    v4 CheckboxHotBackground;
    v4 CheckboxHotBorder;
    v4 CheckboxHotText;
    v4 CheckboxClickedBackground;
    v4 CheckboxClickedBorder;
    v4 CheckboxClickedText;
    v4 CheckboxSelectedBackground;
    v4 CheckboxSelectedBackgroundAlt;
    
    v4 TextboxBackground;
    v4 TextboxBorder;
    v4 TextboxText;
    v4 TextboxSelectedBorder;
    v4 TextboxSelectedBackground;
    v4 TextboxSelectedText;
    
    v4 ButtonBackground;
    v4 ButtonBorder;
    v4 ButtonText;
    v4 ButtonHotBackground;
    v4 ButtonHotBorder;
    v4 ButtonHotText;
    v4 ButtonClickedBackground;
    v4 ButtonClickedBorder;
    v4 ButtonClickedBorderAlt;
    v4 ButtonClickedText;
    v4 ButtonSelectedBackground;
    v4 ButtonSelectedBorder;
    v4 ButtonSelectedBorderAlt;
    v4 ButtonSelectedText;
    
    v4 ScrollbarText;
    v4 ScrollbarBackground;
    v4 ScrollbarSlider;
    v4 ScrollbarHotText;
    v4 ScrollbarHotBackground;
    v4 ScrollbarHotSlider;
    v4 ScrollbarClickedText;
    v4 ScrollbarClickedBackground;
    v4 ScrollbarClickedSlider;
    
    
} colors;

global colors Colors;

inline void
SetColors()
{
    Colors.Clear = RGBColor(255, 255, 255, 255);
    
    Colors.LabelBackground = RGBColor(255, 255, 255, 255);
    Colors.LabelBorder = RGBColor(255, 255, 255, 255);
    Colors.LabelText = RGBColor(0, 0, 0, 255);
    
    Colors.CheckboxBackground = RGBColor(255, 255, 255, 255);
    Colors.CheckboxBorder = RGBColor(51, 51, 51, 255);
    Colors.CheckboxText = RGBColor(0, 0, 0, 255);
    Colors.CheckboxHotBackground = RGBColor(255, 255, 255, 255);
    Colors.CheckboxHotBorder = RGBColor(0, 120, 215, 255);
    Colors.CheckboxHotText = RGBColor(0, 120, 215, 255);
    Colors.CheckboxClickedBackground = RGBColor(204, 228, 247, 255);
    Colors.CheckboxClickedBorder = RGBColor(0, 84, 153, 255);
    Colors.CheckboxClickedText = RGBColor(0, 84, 153, 255);
    Colors.CheckboxSelectedBackground = RGBColor(255, 255, 255, 255);
    Colors.CheckboxSelectedBackgroundAlt = RGBColor(15, 15, 15, 255);
    
    Colors.TextboxBackground = RGBColor(255, 255, 255, 255);
    Colors.TextboxBorder = RGBColor(122, 122, 122, 255);
    Colors.TextboxText = RGBColor(0, 0, 0, 255);
    Colors.TextboxSelectedBorder = RGBColor(0, 120, 215, 255);
    Colors.TextboxSelectedBackground = RGBColor(0, 120, 215, 255);
    Colors.TextboxSelectedText = RGBColor(255, 255, 255, 255);
    
    Colors.ButtonBackground = RGBColor(225, 225, 225, 255);
    Colors.ButtonBorder = RGBColor(173, 173, 173, 255);
    Colors.ButtonText = RGBColor(0, 0, 0, 255);
    Colors.ButtonHotBackground = RGBColor(229, 241, 251, 255);
    Colors.ButtonHotBorder = RGBColor(0, 120, 215, 255);
    Colors.ButtonHotText = RGBColor(0, 120, 215, 255);
    Colors.ButtonClickedBackground = RGBColor(204, 228, 247, 255);
    Colors.ButtonClickedBorder = RGBColor(0, 84, 153, 255);
    Colors.ButtonClickedBorderAlt = RGBColor(60, 20, 7, 255);
    Colors.ButtonClickedText = RGBColor(0, 84, 153, 255);
    Colors.ButtonSelectedBackground = RGBColor(225, 225, 225, 255);
    Colors.ButtonSelectedBorder = RGBColor(0, 120, 215, 255);
    Colors.ButtonSelectedBorderAlt = RGBColor(17, 17, 17, 255);
    Colors.ButtonSelectedText = RGBColor(0, 0, 0, 255);
    
    Colors.ScrollbarText = RGBColor(96, 96, 96, 255);
    Colors.ScrollbarBackground = RGBColor(240, 240, 240, 255);
    Colors.ScrollbarSlider = RGBColor(205, 205, 205, 255);
    Colors.ScrollbarHotText = RGBColor(0, 0, 0, 255);
    Colors.ScrollbarHotBackground = RGBColor(218, 218, 218, 255);
    Colors.ScrollbarHotSlider = RGBColor(166, 166, 166, 255);
    Colors.ScrollbarClickedText = RGBColor(255, 255, 255, 255);
    Colors.ScrollbarClickedBackground = RGBColor(96, 96, 96, 255);
    Colors.ScrollbarClickedSlider = RGBColor(96, 96, 96, 255);
    
}

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
