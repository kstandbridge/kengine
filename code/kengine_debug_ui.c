
inline void
BeginDebugStatistic(debug_clock_entry *Entry)
{
    Entry->Min = F32Max;
    Entry->Max = F32Min;
    Entry->Sum = 0.0f;
    Entry->Count = 0;
}

inline void
AccumDebugStatistic(debug_clock_entry *Entry, f64 Value)
{
    ++Entry->Count;
    
    if(Entry->Min > Value)
    {
        Entry->Min = Value;
    }
    
    if(Entry->Max < Value)
    {
        Entry->Max = Value;
    }
    
    Entry->Sum += Value;
}

inline void
EndDebugStatistic(debug_clock_entry *Entry)
{
    if(Entry->Count)
    {
        Entry->Avg = Entry->Sum / (f64)Entry->Count;
    }
    else
    {
        Entry->Min = 0.0f;
        Entry->Max = 0.0f;
        Entry->Avg = 0.0f;
    }
}

inline u64
SumTotalClock(debug_stored_event *RootEvent)
{
    u64 Result = 0;
    
    for(debug_stored_event *Event = RootEvent;
        Event;
        Event = Event->Next)
    {
        Result += Event->ProfileNode.Duration;
    }
    
    return Result;
}

internal void
DrawProfileBars(memory_arena *Arena, render_group *RenderGroup, debug_profile_node *RootNode, v2 MouseP, rectangle2 EventRect,
                u32 TotalDepth, u32 DepthRemaining, f32 Scale)
{
    v2 PixelSpan = Rectangle2GetDim(EventRect);
    
    for(debug_stored_event *StoredEvent = RootNode->FirstChild;
        StoredEvent;
        StoredEvent = StoredEvent->ProfileNode.NextSameParent)
    {
        debug_profile_node *Node = &StoredEvent->ProfileNode;
        debug_element *Element = Node->Element;
        Assert(Element);
        
        v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f);
        f32 ThisMinX = EventRect.Min.X;
        ThisMinX += Scale*(Node->ParentRelativeClock);
        u64 TotalClock = SumTotalClock(StoredEvent);
        f32 ThisMaxX = ThisMinX + Scale*TotalClock;
        f32 BarWidth = ThisMaxX - ThisMinX;
        
        u32 Row = TotalDepth - DepthRemaining;
        f32 HeightPerBar = PixelSpan.Y / (TotalDepth + 1);
        
        f32 ThisMinY = EventRect.Min.Y + Row*HeightPerBar;
        f32 ThisMaxY = ThisMinY + HeightPerBar;
        
#if 0        
        u32 LaneIndex = Node->ThreadOrdinal;
        f32 LaneY = EventRect.Max.Y - PixelSpan.Y*LaneIndex;
#endif
        
        rectangle2 RegionRect = Rectangle2(V2(ThisMinX, ThisMinY),
                                           V2(ThisMaxX, ThisMaxY));
        PushRenderCommandRectangle(RenderGroup, V4(0.3f, 0.3f, 0.3f, 1.0f), RegionRect, 10.0f);
        PushRenderCommandRectangleOutline(RenderGroup, 1.0f, Color, RegionRect, 20.0f);
        
        if(Rectangle2IsIn(RegionRect, MouseP))
        {
            v2 TextP = V2Add(EventRect.Min, V2Set1(Platform.GetVerticleAdvance()));
            PushRenderCommandText(RenderGroup, 1.0f, TextP, V4(0, 0, 0, 1),
                                  FormatString(Arena, "%S %ucy",
                                               Element->ParsedName.Name, TotalClock));
        }
        
        if(DepthRemaining > 0)
        {
            RegionRect.Min.Y = EventRect.Min.Y;
            RegionRect.Max.Y = EventRect.Max.Y;
            DrawProfileBars(Arena, RenderGroup, Node, MouseP, RegionRect, TotalDepth, DepthRemaining - 1, Scale);
        }
    }
}

internal void
DrawProfilerTab(debug_state *DebugState, ui_state *UIState, render_group *RenderGroup, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    v2 MouseP = V2(Input->MouseX, Input->MouseY);
    
    ui_grid ProfileSplit = BeginGrid(UIState, TempArena, Bounds, 2, 1);
    {
        SetRowHeight(&ProfileSplit, 0, 64.0f);
        SetRowHeight(&ProfileSplit, 1, SIZE_AUTO);
        
        u16 FrameCount = ArrayCount(DebugState->Frames);
        
        BEGIN_BLOCK("DrawFrameBars");
        {            
            ui_grid FrameGrid = BeginGrid(UIState, TempArena, GetCellBounds(&ProfileSplit, 0, 0), 1, FrameCount);
            {
                SetRowHeight(&FrameGrid, 0, 64.0f);
                SetColumnWidth(&FrameGrid, 0, 0, 128.0f);
                
                // TODO(kstandbridge): Use toggle button
                if(Button(&FrameGrid, RenderGroup, 0, 0, InteractionIdFromPtr(DebugState), DebugState, DebugState->Paused ? String("Resume") : String("Pause"))) 
                {
                    DebugState->Paused = !DebugState->Paused;
                }
                
                for(u16 FrameIndex = 1;
                    FrameIndex < FrameCount;
                    ++FrameIndex)
                {
                    u16 NewFrameIndex = (u16)((DebugState->MostRecentFrameOrdinal + FrameIndex + 1) % ArrayCount(DebugState->Frames));
                    debug_frame *Frame = DebugState->Frames + NewFrameIndex;
                    rectangle2 BarBounds = GetCellBounds(&FrameGrid, FrameIndex, 0);
                    
                    f32 Thickness = 1.0f;
                    
                    if(Rectangle2IsIn(BarBounds, MouseP))
                    {
                        if(WasPressed(Input->MouseButtons[MouseButton_Left]))
                        {
                            DebugState->ViewingFrameOrdinal = NewFrameIndex;
                            DebugState->Paused = true;
                        }
                        Thickness = 3.0f;
                    }
                    PushRenderCommandRectangleOutline(RenderGroup, Thickness, V4(0, 0.5f, 0, 1), BarBounds, 20.0f);
                    
                    v2 Dim = V2Subtract(BarBounds.Max, BarBounds.Min);
                    f32 MaxScale = 100.0f;
                    BarBounds.Max.Y = BarBounds.Min.Y + (Dim.Y / MaxScale)*(Frame->SecondsElapsed*1000.0f);
                    
                    v4 Color = V4(0.0f, 1.0f, 0.0f, 1.0f);
                    if(NewFrameIndex == DebugState->ViewingFrameOrdinal)
                    {
                        Color = V4(1.0f, 1.0f, 0.0f, 1.0f);
                    }
                    
                    PushRenderCommandRectangle(RenderGroup, Color, BarBounds, 10.0f);
                }
            }
            EndGrid(&FrameGrid);
        }
        END_BLOCK();
        
        ui_grid CurrentFrameGrid = BeginGrid(UIState, TempArena, GetCellBounds(&ProfileSplit, 0, 1), 2, 1);
        {
            SetRowHeight(&CurrentFrameGrid, 0, 32.0f);
            debug_frame *Frame = DebugState->Frames + DebugState->ViewingFrameOrdinal;
            
            f32 MsPerFrame = Frame->SecondsElapsed;
            f32 FramesPerSecond = 1.0f / MsPerFrame;
            platform_memory_stats MemStats = Platform.GetMemoryStats();
            Button(&CurrentFrameGrid, RenderGroup, 0, 0, InteractionIdFromPtr(DebugState), DebugState, FormatString(TempArena, "%d : %.02f ms %.02f fps - Mem: %u blocks, %lu / %lu used", 
                                                                                                                    Frame->FrameIndex, MsPerFrame*1000.0f, FramesPerSecond,
                                                                                                                    MemStats.BlockCount, MemStats.TotalUsed, MemStats.TotalSize));
            
            ui_grid TopClockSplitGrid = BeginGrid(UIState, TempArena, GetCellBounds(&CurrentFrameGrid, 0, 1), 1, 2);
            {
                // NOTE(kstandbridge): Draw top clocks list
                BEGIN_BLOCK("DrawTopClocks");
                {
                    u32 LinkCount = 0;
                    for(debug_variable_link *Link = GetSentinel(DebugState->ProfileGroup)->Next;
                        Link != GetSentinel(DebugState->ProfileGroup);
                        Link = Link->Next)
                    {
                        ++LinkCount;
                    }
                    
                    debug_clock_entry *Entries = PushArray(TempArena, LinkCount, debug_clock_entry);
                    
                    f64 TotalTime = 0.0f;
                    u32 Index = 0;
                    for(debug_variable_link *Link = GetSentinel(DebugState->ProfileGroup)->Next;
                        Link != GetSentinel(DebugState->ProfileGroup);
                        Link = Link->Next, ++Index)
                    {
                        Assert(Link->FirstChild == GetSentinel(Link));
                        
                        debug_clock_entry *Entry = Entries + Index;
                        Entry->Element = Link->Element;
                        debug_element *Element = Entry->Element;
                        
                        BeginDebugStatistic(Entry);
                        for(debug_stored_event *Event = Element->Frames[DebugState->ViewingFrameOrdinal].OldestEvent;
                            Event;
                            Event = Event->Next)
                        {
                            u64 ClocksWithChildren = Event->ProfileNode.Duration;
                            u64 ClocksWithoutChildren = ClocksWithChildren - Event->ProfileNode.DurationOfChildren;
                            AccumDebugStatistic(Entry, (f64)ClocksWithoutChildren);
                        }
                        EndDebugStatistic(Entry);
                        TotalTime += Entry->Sum;
                    }
                    
                    debug_clock_entry *EntriesTemp = PushArray(TempArena, LinkCount, debug_clock_entry);
                    DebugClockEntryRadixSort(LinkCount, Entries, EntriesTemp, Sort_Descending);
                    
                    f64 PC = 0.0f;
                    if(TotalTime > 0)
                    {
                        PC = 100.0f / TotalTime;
                    }
                    
                    f64 RunningSum = 0.0f;
                    
                    // TODO(kstandbridge): Clip?
                    rectangle2 CellBounds = GetCellBounds(&TopClockSplitGrid, 0, 0);
                    for(Index = 0;
                        Index < LinkCount;
                        ++Index)
                    {
                        debug_clock_entry *Entry = Entries + Index;
                        if(Entry->Count)
                        {
                            debug_element *Element = Entry->Element;
                            
                            v2 At = V2(CellBounds.Min.X, CellBounds.Max.Y);
                            At.X += UIState->LineAdvance;
                            At.Y -= UIState->LineAdvance*(Index+1);
                            RunningSum += Entry->Sum;
                            PushRenderCommandText(RenderGroup, TopClockSplitGrid.Scale, At, V4(0, 0, 0, 1),
                                                  FormatString(TempArena, "Count:%2d %10ucy %3.2f%% %S",
                                                               Entry->Count,
                                                               (u32)Entry->Sum,
                                                               (PC*Entry->Sum),
                                                               Element->ParsedName.Name)); // TODO(kstandbridge): GetName()?
                            
                            // TODO(kstandbridge): Add tooltip
                        }
                    }
                }
                END_BLOCK();
                
                BEGIN_BLOCK("DrawFrameGraph");
                {
                    rectangle2 CellBounds = GetCellBounds(&TopClockSplitGrid, 1, 0);
                    v2 CellDim = Rectangle2GetDim(CellBounds);
                    u32 LaneCount = DebugState->FrameBarLaneCount;
                    f32 LaneHeight = 0.0f;
                    if(LaneCount > 0)
                    {
                        LaneHeight = CellDim.Y / LaneCount;
                    }
                    
                    debug_element *ViewingElement = DebugState->RootProfileElement;
                    debug_element_frame *RootFrame = ViewingElement->Frames + DebugState->ViewingFrameOrdinal;
                    u64 TotalClock = SumTotalClock(RootFrame->OldestEvent);
                    
                    for(debug_stored_event *Event = RootFrame->OldestEvent;
                        Event;
                        Event = Event->Next)
                    {
                        debug_profile_node *RootNode = &Event->ProfileNode;
                        f32 FrameSpan = (f32)RootNode->Duration;
                        v2 PixelSpan = Rectangle2GetDim(CellBounds);
                        f32 Scale = 0.0f;
                        if(FrameSpan > 0)
                        {
                            Scale = PixelSpan.X / FrameSpan;
                        }
                        DrawProfileBars(TempArena, RenderGroup, RootNode, MouseP, CellBounds, 3, 3, Scale);
                        
                    }
                }
                END_BLOCK();
                
            }
            EndGrid(&TopClockSplitGrid);
            
            
        }
        EndGrid(&CurrentFrameGrid);
    }
    EndGrid(&ProfileSplit);
    
}

internal void
DrawDebugGrid(debug_state *DebugState, ui_state *UIState, render_group *RenderGroup, memory_arena *PermArena, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    if(DebugState)
    {
        v2 MouseP = V2(Input->MouseX, Input->MouseY);
        
        string DebugViewTexts[DebugView_Count];
        DebugViewTexts[DebugView_Console] = String("Console");
        DebugViewTexts[DebugView_Settings] = String("Settings");
        DebugViewTexts[DebugView_Profiler] = String("Profiler");
        DebugViewTexts[DebugView_Memory] = String("Memory");
        
        ui_grid DebugGrid = BeginGrid(UIState, TempArena, Bounds, 1, 1);
        {
            rectangle2 TabBounds = TabView(&DebugGrid, RenderGroup, 0, 0, DebugView_Count, DebugViewTexts, (u32 *)&DebugState->CurrentView);
            switch(DebugState->CurrentView)
            {
                case DebugView_Console:
                {
                    ui_grid Grid = BeginGrid(UIState, TempArena, TabBounds, 1, 1);
                    {
                        Label(&Grid, RenderGroup, 0, 0, String("Console"), TextPosition_MiddleMiddle);
                    }
                    EndGrid(&Grid);
                } break;
                
                case DebugView_Settings:
                {
                    ui_grid Grid = BeginGrid(UIState, TempArena, TabBounds, 2, 1);
                    {
                        SetRowHeight(&Grid, 0, 32.0f);
                        SetRowHeight(&Grid, 1, 32.0f);
                        
                        Checkbox(&Grid, RenderGroup, 0, 0, &GlobalDebugEventTable->Settings.ForceSoftwareRendering, 
                                 String("ForceSoftwareRendering"));
                        Checkbox(&Grid, RenderGroup, 0, 1, &GlobalDebugEventTable->Settings.ShowUIElementOutlines, 
                                 String("ShowUIElementOutlines"));
                    }
                    EndGrid(&Grid);
                } break;
                
                case DebugView_Profiler:
                {
                    DrawProfilerTab(DebugState, UIState, RenderGroup, TempArena, Input, TabBounds);
                } break;
                
                case DebugView_Memory:
                {
                    ui_grid Grid = BeginGrid(UIState, TempArena, TabBounds, 1, 1);
                    {
                        // NOTE(kstandbridge): Hack to show memory usage
                        // TODO(kstandbridge): Memory usage tab
                        Label(&Grid, RenderGroup, 0, 0, 
                              FormatString(TempArena, "PermArena %u / %u\nTempArena %u / %u", 
                                           DebugState->Arena.CurrentBlock->Used, DebugState->Arena.CurrentBlock->Size,
                                           TempArena->CurrentBlock->Used, TempArena->CurrentBlock->Size),
                              TextPosition_MiddleMiddle);
                    }
                    EndGrid(&Grid);
                } break;
                
                InvalidDefaultCase;
            }
        }
        EndGrid(&DebugGrid);
        
    }
}
