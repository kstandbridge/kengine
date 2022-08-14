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
                debug_frame *CollectionFrame = DebugState->Frames + DebugState->CollationFrameOrdinal;
                
                CollectionFrame->EndClock = Event->Clock;
                CollectionFrame->SecondsElapsed = Event->Value_F32;
                
                // TODO(kstandbridge): Skip a frame when paused
                {
                    DebugState->MostRecentFrameOrdinal = DebugState->CollationFrameOrdinal;
                    IncrementFrameOrdinal(&DebugState->CollationFrameOrdinal);
                    if(DebugState->CollationFrameOrdinal == DebugState->OldestFrameOrdinal)
                    {
                        //FreeOldestFrame(DebugState);
                    }
                    CollectionFrame = DebugState->Frames + DebugState->CollationFrameOrdinal;
                }
                CollectionFrame->FrameIndex = DebugState->TotalFrameCount++;
                CollectionFrame->BeginClock = Event->Clock;
            }
            else
            {
                InvalidCodePath;
            }
        }
    }
    
    AtomicExchange(&GlobalDebugEventTable->EventIndex, 0);
}
