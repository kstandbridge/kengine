#ifndef KENGINE_UI_H

typedef enum ui_element_type
{
    Element_Row,
    Element_Spacer,
    
    
} ui_element_type;

typedef struct ui_element
{
    u32 Id;
    ui_element_type Type;
    
    v2 Dim;
    
    s32 RowCount;
    s32 ChildCount;
    struct ui_element *FirstChild;
    struct ui_element *Parent;
    
    struct ui_element *Next;
} ui_element;

typedef struct ui_layout
{
    memory_arena *Arena;
    
    loaded_bitmap *DrawBuffer;
    
    s32 ChildCount;
    ui_element SentinalElement;
    ui_element *CurrentElement;
    
    
} ui_layout;

#define KENGINE_UI_H
#endif //KENGINE_UI_H
