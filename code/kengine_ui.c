
inline void
BeginRow(ui_layout *Layout)
{
    ui_element *Element = PushStruct(Layout->Arena, ui_element);
    ZeroStruct(*Element);
    
    if((Layout->CurrentElement->FirstChild == 0) || 
       (Layout->CurrentElement->FirstChild->Type != Element_Row))
    {
        ++Layout->CurrentElement->ChildCount;
    }
    
    Element->Next = Layout->CurrentElement->FirstChild;
    Layout->CurrentElement->FirstChild = Element;
    
    
    
    Element->Parent = Layout->CurrentElement;
    Layout->CurrentElement = Element;
    
    Element->Type = Element_Row;
}

inline void
EndRow(ui_layout *Layout)
{
    Layout->CurrentElement = Layout->CurrentElement->Parent;
}

inline void
Spacer(ui_layout *Layout)
{
    ui_element *Element = PushStruct(Layout->Arena, ui_element);
    ZeroStruct(*Element);
    Element->Next = Layout->CurrentElement->FirstChild;
    Layout->CurrentElement->FirstChild = Element;
    
    ++Layout->CurrentElement->ChildCount;
    
    Element->Type = Element_Spacer;
}

internal ui_layout *
BeginUIFrame(memory_arena *Arena, loaded_bitmap *DrawBuffer)
{
    ui_layout *Result = PushStruct(Arena, ui_layout);
    ZeroStruct(*Result);
    
    Result->Arena = Arena;
    Result->DrawBuffer = DrawBuffer;
    Result->SentinalElement.Dim = V2((f32)DrawBuffer->Width, (f32)DrawBuffer->Height);
    Result->CurrentElement = &Result->SentinalElement;
    
    return Result;
}

// TODO(kstandbridge): Ditch this crap
global s32 ColArrayIndex;
global v4 ColArray[25];


internal void
DrawElements(ui_layout *Layout, ui_element *FirstChild, v2 P)
{
    for(ui_element *Element = FirstChild;
        Element;
        Element = Element->Next)
    {
        if(Element->Type == Element_Row)
        {
            Assert(Element->FirstChild);
            
            f32 StartY = P.Y;
            f32 AdvanceX = Element->Dim.X;
            while(Element->Type == Element_Row)
            {
                DrawElements(Layout, Element->FirstChild, P);
                P.Y += Element->FirstChild->Dim.Y;
                if(Element->Next && Element->Next->Type == Element_Row)
                {
                    Element = Element->Next;
                }
                else
                {
                    break;
                }
            }
            P.Y = StartY;
            P.X -= AdvanceX;
        }
        else
        {
            P.X -= Element->Dim.X;
            
            loaded_bitmap *DrawBuffer = PushStruct(Layout->Arena, loaded_bitmap);
            DrawBuffer->Width = (s32)Element->Dim.X;
            DrawBuffer->Height = (s32)Element->Dim.Y;
            DrawBuffer->Pitch = Layout->DrawBuffer->Pitch;
            DrawBuffer->Memory = (u8 *)Layout->DrawBuffer->Memory + (s32)P.X*BITMAP_BYTES_PER_PIXEL + (s32)P.Y*DrawBuffer->Pitch;
            
            DEBUGTextLine(FormatString(Layout->Arena, "At: %.02f %.02f Dim: %.02f %.02f", P.X, P.Y, Element->Dim.X, Element->Dim.Y));
            
            render_group *RenderGroup = AllocateRenderGroup(Layout->Arena, Megabytes(4), DrawBuffer);
            
            PushClear(RenderGroup, ColArray[ColArrayIndex]);
            ColArrayIndex = (ColArrayIndex + 1) % ArrayCount(ColArray);
            
            tile_render_work Work;
            ZeroStruct(Work);
            Work.Group = RenderGroup;
            Work.ClipRect.MinX = 0;
            Work.ClipRect.MaxX = DrawBuffer->Width;
            Work.ClipRect.MinY = 0;
            Work.ClipRect.MaxY = DrawBuffer->Height;
            TileRenderWorkThread(&Work);
        }
    }
}

internal void
CalculateElementDims(ui_layout *Layout, ui_element *FirstChild, s32 ChildCount, v2 Dim)
{
    b32 RowStart = false;
    v2 SubDim = Dim;
    for(ui_element *Element = FirstChild;
        Element;
        Element = Element->Next)
    {
        Element->Dim.X = Dim.X / ChildCount;
        Element->Dim.Y = Dim.Y;
        if(Element->Type == Element_Row)
        {
            if(!RowStart)
            {
                s32 RowCount = 1;
                ui_element *FirstRow = Element;
                while(FirstRow && FirstRow->Next && FirstRow->Next->Type == Element_Row)
                {
                    FirstRow = FirstRow->Next;
                    ++RowCount;
                }
                SubDim = V2(Element->Dim.X, Element->Dim.Y / RowCount);
                RowStart = true;
            }
            
            CalculateElementDims(Layout, Element->FirstChild, Element->ChildCount, SubDim);
        }
        else
        {
            RowStart = false;
        }
    }
}

internal void
EndUIFrame(ui_layout *Layout)
{
    // TODO(kstandbridge): Handle UI Interactions
    
    v2 Dim = V2((f32)Layout->DrawBuffer->Width, (f32)Layout->DrawBuffer->Height);
    
    CalculateElementDims(Layout, Layout->SentinalElement.FirstChild, Layout->SentinalElement.ChildCount, V2(Dim.X, Dim.Y));
    
    ColArrayIndex = 0;
    ColArray[0] = V4(0.0f, 0.0f, 1.0f, 1.0f);
    ColArray[1] = V4(0.0f, 1.0f, 0.0f, 1.0f);
    ColArray[2] = V4(0.0f, 1.0f, 1.0f, 1.0f);
    ColArray[3] = V4(1.0f, 0.0f, 0.0f, 1.0f);
    ColArray[4] = V4(1.0f, 0.0f, 1.0f, 1.0f);
    ColArray[5] = V4(0.0f, 0.0f, 0.5f, 1.0f);
    ColArray[6] = V4(0.0f, 0.5f, 0.0f, 1.0f);
    ColArray[7] = V4(0.0f, 0.5f, 0.5f, 1.0f);
    ColArray[8] = V4(0.5f, 0.0f, 0.0f, 1.0f);
    ColArray[9] = V4(0.5f, 0.0f, 0.5f, 1.0f);
    ColArray[10] = V4(0.0f, 0.0f, 0.3f, 1.0f);
    ColArray[11] = V4(0.0f, 0.3f, 0.0f, 1.0f);
    ColArray[12] = V4(0.0f, 0.3f, 0.3f, 1.0f);
    ColArray[13] = V4(0.3f, 0.0f, 0.0f, 1.0f);
    ColArray[14] = V4(0.3f, 0.0f, 0.3f, 1.0f);
    ColArray[15] = V4(0.0f, 0.0f, 0.75f, 1.0f);
    ColArray[16] = V4(0.0f, 0.75f, 0.0f, 1.0f);
    ColArray[17] = V4(0.0f, 0.75f, 0.75f, 1.0f);
    ColArray[18] = V4(0.75f, 0.0f, 0.0f, 1.0f);
    ColArray[19] = V4(0.75f, 0.0f, 0.75f, 1.0f);
    ColArray[20] = V4(0.0f, 0.0f, 0.25f, 1.0f);
    ColArray[21] = V4(0.0f, 0.25f, 0.0f, 1.0f);
    ColArray[22] = V4(0.0f, 0.25f, 0.25f, 1.0f);
    ColArray[23] = V4(0.25f, 0.0f, 0.0f, 1.0f);
    ColArray[24] = V4(0.25f, 0.0f, 0.25f, 1.0f);
    
    v2 P = V2((f32)Layout->DrawBuffer->Width, 0.0f);
    DrawElements(Layout, Layout->SentinalElement.FirstChild, P);
}

#define Label(Layout, ...) Spacer(Layout)
#define Checkbox(Layout, ...) Spacer(Layout)
#define Textbox(Layout, ...) Spacer(Layout)
#define Splitter(Layout, ...) Spacer(Layout)
#define DropDown(Layout, ...) Spacer(Layout)
#define Button(Layout, ...) Spacer(Layout)
