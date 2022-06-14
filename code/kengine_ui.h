#ifndef KENGINE_UI_H

typedef enum ui_interaction_type
{
    Interaction_None,
    
    Interaction_NOP,
    
    Interaction_EditableBool,
    Interaction_EditableText,
    Interaction_EditableMultilineText,
    Interaction_ImmediateButton
        
} ui_interaction_type;

typedef struct ui_interaction
{
    u32 ID;
    ui_interaction_type Type;
    
    union
    {
        void *Generic;
        b32 *Bool;
        editable_string *Text;
    };
} ui_interaction;

typedef enum ui_element_type
{
    Element_Row,
    Element_Spacer,
    Element_Label,
    Element_Checkbox,
    Element_Textbox,
    Element_MultilineTextbox,
    Element_Button,
    
} ui_element_type;

typedef struct ui_element
{
    u32 Id;
    ui_element_type Type;
    ui_interaction Interaction;
    
    string Text;
    rectangle2 TextBounds;
    
    v2 Dim;
    v2 UsedDim;
    
    s32 ChildCount;
    s32 SetWidthChildCount;
    
    struct ui_element *FirstChild;
    struct ui_element *Parent;
    
    struct ui_element *Next;
} ui_element;

typedef struct ui_layout
{
    memory_arena *Arena;
    assets *Assets;
    f32 Scale;
    f32 Padding;
    f32 DefaultRowHeight;
    
    loaded_bitmap *DrawBuffer;
    
    s32 ChildCount;
    ui_element SentinalElement;
    ui_element *CurrentElement;
    
    u32 CurrentId;
    v2 MouseP;
    v2 dMouseP;
    f32 MouseZ;
    
    f32 DeltaTime;
    
} ui_layout;

typedef struct ui_state
{
    assets *Assets;
    
    v2 LastMouseP;
    
    ui_interaction Interaction;
    
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    ui_interaction ToExecute;
    ui_interaction NextToExecute;
    
    ui_interaction SelectedInteraction;
    ui_interaction ClickedInteraction;
    
} ui_state;

inline b32
InteractionsAreEqual(ui_interaction A, ui_interaction B)
{
    b32 Result = ((A.ID == B.ID) &&
                  (A.Type == B.Type) &&
                  (A.Generic == B.Generic));
    
    return Result;
}

inline b32
InteractionIsToExecute(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->ToExecute, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsNextToExecute(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->NextToExecute, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsHot(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->HotInteraction, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsSelected(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->SelectedInteraction, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsClicked(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->ClickedInteraction, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline void
ClearInteraction(ui_interaction *Interaction)
{
    Interaction->Type = Interaction_None;
    Interaction->Generic = 0;
}

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

// TODO(kstandbridge): Where should colors be located?
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

#define KENGINE_UI_H
#endif //KENGINE_UI_H
