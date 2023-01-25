#ifndef KENGINE_UI_H

typedef struct ui_grid
{
    rectangle2 Bounds;
    u32 Columns;
    u32 Rows;
    
    b32 GridSizeCalculated;
    f32 *ColumnWidths;
    f32 *RowHeights;
    
    f32 DefaultRowHeight;
    
    struct ui_grid *Prev;
} ui_grid;

typedef union ui_id_value
{
    void *Void;
    u64 U64;
    u32 U32[2];
} ui_id_value;

typedef struct ui_id
{
    ui_id_value Value[2];
} ui_id;

#define GenerateUIId__(Ptr, File, Line) GenerateUIId___(Ptr, File "|" #Line)
#define GenerateUIId_(Ptr, File, Line) GenerateUIId__(Ptr, File, Line)
#define GenerateUIId(Ptr) GenerateUIId_((Ptr), __FILE__, __LINE__)
inline ui_id
GenerateUIId___(void *Ptr, char *Text)
{
    ui_id Result =
    {
        .Value = 
        {
            Ptr,
            Text
        },
    };
    
    return Result;
}

typedef enum ui_interaction_type
{
    UI_Interaction_None,
    
    UI_Interaction_NOP,
    
    UI_Interaction_Draggable,
    UI_Interaction_ImmediateButton,
    
} ui_interaction_type;

typedef struct ui_interaction
{
    ui_id Id;
    ui_interaction_type Type;
    
    void *Target;
    union
    {
        void *Generic;
        v2 *P;
    };
} ui_interaction;

typedef struct ui_frame
{
    memory_arena *Arena;
    render_group *RenderGroup;
    app_input *Input;
    ui_grid *CurrentGrid;
} ui_frame;

typedef struct glyph_info
{
    u8 *Data;
    
    u32 CodePoint;
    
    s32 Width;
    s32 Height;
    s32 XOffset;
    s32 YOffset;
    
    s32 AdvanceWidth;
    s32 LeftSideBearing;
    
    v4 UV;
    
} glyph_info;

typedef struct ui_state
{
    // NOTE(kstandbridge): Persistent
    memory_arena Arena;
    
    v2 MouseP;
    v2 dMouseP;
    v2 LastMouseP;
    
    f32 FontScale;
    s32 FontAscent;
    s32 FontDescent;
    s32 FontLineGap;
    stbtt_fontinfo FontInfo;
    glyph_info GlyphInfos[256];
    
    ui_interaction Interaction;
    
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    ui_interaction ToExecute;
    ui_interaction NextToExecute;
    
    ui_interaction SelectedInteration;
    
    // NOTE(kstandbridge): Transient
    ui_frame *Frame;
} ui_state;

inline void
ClearInteraction(ui_interaction *Interaction)
{
    Interaction->Type = UI_Interaction_None;
    Interaction->Generic = 0;
}

inline b32
InteractionsAreEqual(ui_interaction A, ui_interaction B)
{
    b32 Result = ((A.Id.Value[0].U64 == B.Id.Value[0].U64) &&
                  (A.Id.Value[1].U64 == B.Id.Value[1].U64) &&
                  (A.Type == B.Type) &&
                  (A.Target == B.Target) &&
                  (A.Generic == B.Generic));
    
    return Result;
}

inline b32
InteractionIsHot(ui_state *UiState, ui_interaction B)
{
    b32 Result = InteractionsAreEqual(UiState->HotInteraction, B);
    
    if(B.Type == UI_Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

global f32 GlobalMargin = 7.0f;

#define RGB(R, G ,B) {R/255.0f,G/255.0f,B/255.0f, 1.0f }
global v4 GlobalBackColor = RGB(240, 240, 240);
global v4 GlobalFormColor = RGB(255, 255, 255);
global v4 GlobalFormTextColor = RGB(0, 0, 0);

global v4 GlobalButtonBackColor = RGB(225, 225, 225);
global v4 GlobalButtonBorderColor = RGB(173, 173, 173);

global v4 GlobalButtonHotBackColor = RGB(229, 241, 251);
global v4 GlobalButtonHotBorderColor = RGB(0, 120, 215);

global v4 GlobalButtonClickedBackColor = RGB(204, 228, 247);
global v4 GlobalButtonClickedBorderColor = RGB(0, 84, 153);

global v4 GlobalTabButtonBorder = RGB(217, 217, 217);
global v4 GlobalTabButtonBackground = RGB(240, 240, 240);
global v4 GlobalTabButtonHot = RGB(216, 234, 249);
global v4 GlobalTabButtonClicked = RGB(204, 228, 247);
global v4 GlobalTabButtonSelected = RGB(255, 255, 255);

global v4 GlobalGroupBorder = RGB(220, 220, 220);


#define KENGINE_UI_H
#endif //KENGINE_UI_H
