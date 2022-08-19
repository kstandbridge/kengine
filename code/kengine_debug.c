inline void
IncrementFrameOrdinal(u32 *Ordinal)
{
    *Ordinal = (*Ordinal + 1) % DEBUG_FRAME_COUNT;
}

extern void
DebugUpdateFrame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena, app_input *Input)
{
    if(!PlatformAPI->DebugState)
    {
        PlatformAPI->DebugState = PushStruct(Arena, debug_state);
    }
    debug_state *DebugState = PlatformAPI->DebugState;
    
    // NOTE(kstandbridge): Debug collation
    {
        for(u32 EventIndex = 0;
            EventIndex < GlobalDebugEventTable->EventIndex;
            ++EventIndex)
        {
            debug_event *Event = GlobalDebugEventTable->Events + EventIndex;
            if(Event->Type == DebugEvent_FrameEnd)
            {
                debug_frame *CollationFrame = DebugState->Frames + DebugState->CollationFrameOrdinal;
                
                CollationFrame->EndClock = Event->Clock;
                CollationFrame->SecondsElapsed = Event->Value_F32;
                
                // TODO(kstandbridge): Skip a frame when paused
                if(DebugState->Paused)
                {
                    
                }
                else
                {
                    DebugState->MostRecentFrameOrdinal = DebugState->CollationFrameOrdinal;
                    IncrementFrameOrdinal(&DebugState->CollationFrameOrdinal);
                    if(DebugState->CollationFrameOrdinal == DebugState->OldestFrameOrdinal)
                    {
                        //FreeOldestFrame(DebugState);
                    }
                    CollationFrame = DebugState->Frames + DebugState->CollationFrameOrdinal;
                }
                CollationFrame->FrameIndex = DebugState->TotalFrameCount++;
                CollationFrame->BeginClock = Event->Clock;
            }
            else
            {
                InvalidCodePath;
            }
        }
    }
    
    AtomicExchange(&GlobalDebugEventTable->EventIndex, 0);
}

internal void
DrawDebugGrid(debug_state *DebugState, ui_state *UIState, render_group *RenderGroup, memory_arena *PermArena, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    if(DebugState)
    {
        u16 FrameCount = ArrayCount(DebugState->Frames);
        
        ui_grid DebugGrid = BeginGrid(UIState, TempArena, Bounds, FrameCount, 2);
        {
            SetAllColumnWidths(&DebugGrid, 0, 256.0f);
            
            for(u16 FrameIndex = 1;
                FrameIndex < FrameCount;
                ++FrameIndex)
            {
                u16 NewFrameIndex = (u16)((DebugState->MostRecentFrameOrdinal + FrameIndex + 1) % ArrayCount(DebugState->Frames));
                debug_frame *Frame = DebugState->Frames + NewFrameIndex;
                f32 MsPerFrame = Frame->SecondsElapsed;
                f32 FramesPerSecond = 1.0f / MsPerFrame;
                string Text = FormatString(TempArena, "%d : %.02f ms %.02f fps", Frame->FrameIndex, MsPerFrame*1000.0f, FramesPerSecond);
                
                ui_interaction Interaction;
                ZeroStruct(Interaction);
                Interaction.Type = Interaction_None;
                
                ui_element Element = BeginUIElement(&DebugGrid, 0, FrameIndex - 1);
                SetUIElementDefaultAction(&Element, Interaction);
                EndUIElement(&Element);
                
                v2 ElementDim = V2Subtract(Element.Bounds.Max, Element.Bounds.Min);
                v2 ElementHalfDim = V2Multiply(ElementDim, V2Set1(0.5f));
                
                rectangle2 TextBounds = GetTextSize(RenderGroup, DebugGrid.Scale, V2Set1(0.0f), Text);
                v2 TextDim = Rectangle2GetDim(TextBounds);
                v2 TextHalfDim = V2Multiply(TextDim, V2Set1(0.5f));
                
                v4 TextColor = NewFrameIndex == DebugState->MostRecentFrameOrdinal ? V4(0.0f, 0.0f, 1.0f, 1) : V4(0.0f, 0.0f, 0.0f, 1);
                
                v2 TextOffset = V2Add(V2Subtract(Element.Bounds.Min, TextHalfDim),ElementHalfDim);
                PushRenderCommandText(RenderGroup, DebugGrid.Scale, TextOffset, TextColor, Text);
                
            }
            
            // TODO(kstandbridge): Use toggle button
            if(Button(&DebugGrid, RenderGroup, 0, FrameCount - 1, GenerateInteractionId(DebugState), DebugState, DebugState->Paused ? String("Resume") : String("Pause"))) 
            {
                DebugState->Paused = !DebugState->Paused;
            }
        }
        EndGrid(&DebugGrid);
    }
}
