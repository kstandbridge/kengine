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
        
        ui_grid DebugGrid = BeginGrid(UIState, TempArena, Bounds, FrameCount, 1);
        {
            for(u16 FrameIndex = 0;
                FrameIndex < FrameCount;
                ++FrameIndex)
            {
                debug_frame *Frame = DebugState->Frames + FrameIndex;
                f32 MsPerFrame = Frame->SecondsElapsed;
                f32 FramesPerSecond = 1.0f / MsPerFrame;
                string FPS = FormatString(TempArena, "%.02f ms %.02f fps", MsPerFrame*1000.0f, FramesPerSecond);
                Button(&DebugGrid, RenderGroup, 0, FrameIndex, GenerateInteractionId(DebugState), DebugState, FPS);
            }
        }
        EndGrid(&DebugGrid);
    }
}
