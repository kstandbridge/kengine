
internal void
TextElement(ui_state *UiState, assets *Assets, render_group *RenderGroup, v2 MouseP, v2 BoundsP, string Str, ui_interaction Interaction)
{
    f32 Scale = 1.0f;
    
    rectangle2 TextBounds = GetTextSize(RenderGroup, Assets, BoundsP, Scale, Str);
    
    b32 IsHot = InteractionIsHot(UiState, Interaction);
    
    v4 ButtonColor = IsHot ? V4(0, 0, 1, 1) : V4(1, 0, 0, 1);
    PushRect(RenderGroup, TextBounds.Min, V2Subtract(TextBounds.Max, TextBounds.Min), ButtonColor);
    WriteLine(RenderGroup, Assets, BoundsP, Scale, Str);
    
    if(IsInRectangle(TextBounds, MouseP))
    {
        UiState->NextHotInteraction = Interaction;
    }
    
}

internal b32 
PushButton(ui_state *UiState, assets *Assets, render_group *RenderGroup, u32 ID, v2 MouseP, v2 BoundsP, string Str)
{
    ui_interaction Interaction;
    ZeroStruct(Interaction);
    Interaction.ID = ID;
    Interaction.Type = UiInteraction_ImmediateButton;
    
    b32 Result = InteractionsAreEqual(Interaction, UiState->ToExecute);
    
    TextElement(UiState, Assets, RenderGroup, MouseP, BoundsP, Str, Interaction);
    
    return Result;
}

internal void
EditableV2(ui_state *UiState, assets *Assets, render_group *RenderGroup, u32 ID, v2 MouseP, v2 BoundsP, string Str, v2 *TargetP)
{
    ui_interaction Interaction;
    ZeroStruct(Interaction);
    Interaction.ID = ID;
    Interaction.Type = UiInteraction_Draggable;
    Interaction.P = TargetP;
    
    TextElement(UiState, Assets, RenderGroup, MouseP, BoundsP, Str, Interaction);
    
    if(InteractionsAreEqual(Interaction, UiState->Interaction))
    {
        v2 dMouseP = V2Subtract(MouseP, UiState->LastMouseP);
        v2 *P = UiState->Interaction.P;
        *P = V2Add(*P, dMouseP);
    }
}

internal void
UIUpdateAndRender(app_state *AppState, ui_state *UiState, memory_arena *Arena, render_group *RenderGroup, v2 MouseP)
{
    string Str = FormatString(Arena, "before %d after", AppState->TestCounter);
    EditableV2(UiState, &AppState->Assets, RenderGroup, 1111, MouseP, AppState->TestP, Str, &AppState->TestP);
    
    if(PushButton(UiState, &AppState->Assets, RenderGroup, 2222, MouseP, V2(750, 150), String("Decrement")))
    {
        --AppState->TestCounter;
    }
    
    if(PushButton(UiState, &AppState->Assets, RenderGroup, 3333, MouseP, V2(150, 150), String("Increment")))
    {
        ++AppState->TestCounter;
    }
    
    UiState->ToExecute = UiState->NextToExecute;
    ClearInteraction(&UiState->NextToExecute);
}

internal void
UIInteract(ui_state *UiState, app_input *Input)
{    
    u32 TransitionCount = Input->MouseButtons[AppInputMouseButton_Left].HalfTransitionCount;
    b32 MouseButton = Input->MouseButtons[AppInputMouseButton_Left].EndedDown;
    if(TransitionCount % 2)
    {
        MouseButton = !MouseButton;
    }
    
    for(u32 TransitionIndex = 0;
        TransitionIndex <= TransitionCount;
        ++TransitionIndex)
    {
        b32 MouseDown = false;
        b32 MouseUp = false;
        if(TransitionIndex != 0)
        {
            MouseDown = MouseButton;
            MouseUp = !MouseButton;
        }
        
        b32 EndInteraction = false;
        
        switch(UiState->Interaction.Type)
        {
            case UiInteraction_ImmediateButton:
            case UiInteraction_Draggable:
            {
                if(MouseUp)
                {
                    UiState->NextToExecute = UiState->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            // TODO(kstandbridge): InteractionSelect that could choose a textbox or row in list
            
            case UiInteraction_None:
            {
                UiState->HotInteraction = UiState->NextHotInteraction;
                if(MouseDown)
                {
                    UiState->Interaction = UiState->NextHotInteraction;
                }
            } break;
            
            default:
            {
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            }
        }
        
        if(EndInteraction)
        {
            ClearInteraction(&UiState->Interaction);
        }
        
        MouseButton = !MouseButton;
    }
}
