

ui_interaction_state
AddUIInteraction(ui_state *State, rectangle2 Bounds, ui_interaction Interaction)
{
    ui_interaction_state Result = UIInteractionState_None;
    
    Assert(Interaction.Type);
    
    if(Rectangle2IsIn(Bounds, State->MouseP))
    {
        State->NextHotInteraction = Interaction;
    }
    
    if(InteractionsAreEqual(Interaction, State->Interaction) &&
       InteractionsAreEqual(Interaction, State->NextHotInteraction))
    {
        Result = UIInteractionState_HotClicked;
    }
    else if(InteractionsAreEqual(Interaction, State->HotInteraction))
    {
        Result = UIInteractionState_Hot;
    }
    else if(InteractionsAreEqual(Interaction, State->SelectedInteration))
    {
        Result = UIInteractionState_Selected;
    }
    else
    {
        Result = UIInteractionState_None;
    }
    
    return Result;
}

ui_frame
BeginUI(ui_state *State, app_input *Input, memory_arena *Arena)
{
    ui_frame Result =
    {
        .Arena = Arena,
        .CurrentGrid = 0
    };
    
    
    State->LastMouseP = State->MouseP;
    State->MouseP = Input->MouseP;
    State->dMouseP = V2Subtract(State->MouseP, State->LastMouseP);
    
    
    return Result;
}

void
EndUI(ui_state *State, app_input *Input)
{
    State->ToExecute = State->NextToExecute;
    ClearInteraction(&State->NextToExecute);
    
    u32 TransistionCount = Input->MouseButtons[MouseButton_Left].HalfTransitionCount;
    b32 MouseButton = Input->MouseButtons[MouseButton_Left].EndedDown;
    if(TransistionCount % 2)
    {
        MouseButton = !MouseButton;
    }
    
    for(u32 TransitionIndex = 0;
        TransitionIndex <= TransistionCount;
        ++TransitionIndex)
    {
        b32 MouseMove = false;
        b32 MouseDown = false;
        b32 MouseUp = false;
        if(TransitionIndex == 0)
        {
            MouseMove = true;
        }
        else
        {
            MouseDown = MouseButton;
            MouseUp = !MouseButton;
        }
        
        b32 EndInteraction = false;
        
        switch(State->Interaction.Type)
        {
            case UI_Interaction_ImmediateButton:
            {
                if(MouseUp)
                {
                    State->NextToExecute = State->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            case UI_Interaction_None:
            {
                State->HotInteraction = State->NextHotInteraction;
                if(MouseDown)
                {
                    State->Interaction = State->HotInteraction;
                }
            } break;
            
            default:
            {
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            } break;
        }
        
        if(MouseDown)
        {
            ClearInteraction(&State->SelectedInteration);
        }
        
        if(MouseUp)
        {
            State->SelectedInteration = State->Interaction;
        }
        
        if(EndInteraction)
        {
            ClearInteraction(&State->Interaction);
        }
        
        MouseButton = !MouseButton;
    }
    
    ClearInteraction(&State->NextHotInteraction);
}

void
BeginGrid(ui_frame *Frame, rectangle2 Bounds, u32 Columns, u32 Rows)
{
    memory_arena *Arena = Frame->Arena;
    
    ui_grid *Grid = PushStruct(Arena, ui_grid);
    Grid->Prev = Frame->CurrentGrid;
    Frame->CurrentGrid = Grid;
    
    Grid->Bounds = Bounds;
    Grid->Columns = Columns;
    Grid->Rows = Rows;
    
    Grid->GridSizeCalculated = false;
    Grid->ColumnWidths = PushArray(Arena, Columns, f32);
    for(u32 ColumnIndex = 0;
        ColumnIndex < Columns;
        ++ColumnIndex)
    {
        Grid->ColumnWidths[ColumnIndex] = 0.0f;
    }
    
    Grid->RowHeights = PushArray(Arena, Rows, f32);
    for(u32 RowIndex = 0;
        RowIndex < Rows;
        ++RowIndex)
    {
        Grid->RowHeights[RowIndex] = 0.0f;
    }
}

void
EndGrid(ui_frame *Frame)
{
    Assert(Frame->CurrentGrid);
    Frame->CurrentGrid = Frame->CurrentGrid->Prev;
}

rectangle2
GridGetCellBounds(ui_frame *Frame, u32 Column, u32 Row, f32 Margin)
{
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    if(!Grid->GridSizeCalculated)
    {
        v2 Dim = Rectangle2GetDim(Grid->Bounds);
        u32 ColumnsFilled = 0;
        f32 WidthUsed = 0.0f;
        for(u32 ColumnIndex = 0;
            ColumnIndex < Grid->Columns;
            ++ColumnIndex)
        {
            if(Grid->ColumnWidths[ColumnIndex] == 0.0f)
            {
                ++ColumnsFilled;
            }
            else
            {
                WidthUsed += Grid->ColumnWidths[ColumnIndex];
            }
        }
        if(ColumnsFilled > 0)
        {
            f32 WidthRemaining = Dim.X - WidthUsed;
            f32 ColumnWidth = WidthRemaining / ColumnsFilled;
            
            for(u32 ColumnIndex = 0;
                ColumnIndex < Grid->Columns;
                ++ColumnIndex)
            {
                if(Grid->ColumnWidths[ColumnIndex] == 0.0f)
                {
                    Grid->ColumnWidths[ColumnIndex] = ColumnWidth;
                }
            }
        }
        
        u32 RowsFilled = 0;
        f32 HeightUsed = 0.0f;
        for(u32 RowIndex = 0;
            RowIndex < Grid->Rows;
            ++RowIndex)
        {
            if(Grid->RowHeights[RowIndex] == 0.0f)
            {
                ++RowsFilled;
            }
            else
            {
                HeightUsed += Grid->RowHeights[RowIndex];
            }
        }
        if(RowsFilled > 0)
        {
            f32 HeightRemaining = Dim.Y - HeightUsed;
            f32 RowHeight = HeightRemaining / RowsFilled;
            
            for(u32 RowIndex = 0;
                RowIndex < Grid->Rows;
                ++RowIndex)
            {
                if(Grid->RowHeights[RowIndex] == 0.0f)
                {
                    Grid->RowHeights[RowIndex] = RowHeight;
                }
            }
        }
        
        Grid->GridSizeCalculated = true;
    }
    
    v2 Min = Grid->Bounds.Min;
    for(u32 ColumnIndex = 0;
        ColumnIndex < Column;
        ++ColumnIndex)
    {
        Min.X += Grid->ColumnWidths[ColumnIndex];
    }
    
    for(u32 RowIndex = 0;
        RowIndex < Row;
        ++RowIndex)
    {
        Min.Y += Grid->RowHeights[RowIndex];
    }
    
    v2 Max = V2Add(Min, V2(Grid->ColumnWidths[Column], Grid->RowHeights[Row]));
    
    rectangle2 Result = Rectangle2(Min, Max);
    if(Margin > 0.0f)
    {
        if(Grid->Columns == 1)
        {
            Result.Min.X += Margin;
            Result.Max.X -= Margin;
        }
        else if(Column == 0)
        {
            Result.Min.X += Margin;
            Result.Max.X -= Margin*0.5f;
        }
        else if((Column + 1) == Grid->Columns)
        {
            Result.Min.X += Margin*0.5f;
            Result.Max.X -= Margin;
        }
        else
        {
            Result.Min.X += Margin*0.5f;
            Result.Max.X -= Margin*0.5f;
        }
        
        if(Grid->Rows == 1)
        {
            Result.Min.Y += Margin;
            Result.Max.Y -= Margin;
        }
        else if(Row == 0)
        {
            Result.Min.Y += Margin;
            Result.Max.Y -= Margin*0.5f;
        }
        
        else if((Row + 1) == Grid->Rows)
        {
            Result.Min.Y += Margin*0.5f;
            Result.Max.Y -= Margin;
        }
        else
        {
            Result.Min.Y += Margin;
            Result.Max.Y -= Margin*0.5f;
        }
    }
    return Result;
}
