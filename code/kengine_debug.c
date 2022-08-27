inline void
IncrementFrameOrdinal(u32 *Ordinal)
{
    *Ordinal = (*Ordinal + 1) % DEBUG_FRAME_COUNT;
}

inline debug_variable_link *
GetSentinel(debug_variable_link *From)
{
    debug_variable_link *Result = (debug_variable_link *)(&From->FirstChild);
    return Result;
}

inline debug_variable_link *
CreateVariableLink(debug_state *DebugState, u32 NameLength, char *Name)
{
    debug_variable_link *Result = PushStruct(&DebugState->Arena, debug_variable_link);
    ZeroStruct(*Result);
    DLIST_INIT(GetSentinel(Result));
    Result->Next = Result->Prev = 0;
    Result->Name = NameLength ? PushString_(&DebugState->Arena, NameLength, (u8 *)Name) : String("");
    Result->Element = 0;
    
    return Result;
}

inline debug_variable_link *
AddElementToGroup(debug_state *DebugState, debug_variable_link *Parent, debug_element *Element)
{
    debug_variable_link *Result = CreateVariableLink(DebugState, 0, 0);
    
    if(Parent)
    {
        DLIST_INSERT_AS_LAST(GetSentinel(Parent), Result);
    }
    Result->Element = Element;
    
    return Result;
}

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

internal debug_thread *
GetDebugThread(debug_state *DebugState, u32 ThreadId)
{
    // TODO(kstandbridge): Are threads ever actually freed? If not turn this into a linked list
    
    debug_thread *Result = 0;
    for(debug_thread *Thread = DebugState->FirstThread;
        Thread;
        Thread = Thread->Next)
    {
        if(Thread->Id == ThreadId)
        {
            Result = Thread;
            break;
        }
    }
    
    if(!Result)
    {
        FREELIST_ALLOCATE(Result, DebugState->FirstFreeThread, PushStruct(&DebugState->Arena, debug_thread));
        
        Result->Id = ThreadId;
        Result->LaneIndex = DebugState->FrameBarLaneCount++;
        Result->FirstOpenCodeBlock = 0;
        //Result->FirstOpenDataBlock = 0;
        Result->Next = DebugState->FirstThread;
        DebugState->FirstThread = Result;
    }
    
    return Result;
}

inline debug_parsed_name
DebugParseName(char *GUID)
{
    debug_parsed_name Result;
    ZeroStruct(Result);
    //Result.GUID = PushString_(Arena, GetNullTerminiatedStringLength(GUID), (u8 *)GUID);
    // TODO(kstandbridge): This cannot work with dll hotloading, since the memory could no longer exist
    Result.GUID = String_(GetNullTerminiatedStringLength(GUID), (u8 *)GUID);
    
    u32 PipeCount = 0;
    u8 *Scan = Result.GUID.Data;
    u32 Index = 0;
    for(;
        Index < Result.GUID.Size;
        ++Scan, ++Index)
    {
        if(*Scan == '|')
        {
            if(PipeCount == 0)
            {
                u32 FileNameEndsAt = (u32)(Scan - Result.GUID.Data);
                Result.FileName = String_(FileNameEndsAt, (u8 *)(Result.GUID.Data));
                Result.LineNumber = U32FromString((char *)Scan + 1);
            }
            else if(PipeCount == 1)
            {
                
            }
            else
            {
                u32 NameStartsAt = (u32)(Scan - Result.GUID.Data + 1);
                Result.Name = String_(Result.GUID.Size - NameStartsAt, (u8 *)(Result.GUID.Data + NameStartsAt));
            }
            
            ++PipeCount;
        }
        // TODO(kstandbridge): Better hash function
        Result.HashValue = 65599*Result.HashValue + *Scan;
    }
    
    return Result;
}

inline debug_element *
GetElementFromGUID(debug_state *DebugState, u32 Index, char *GUID)
{
    // TODO(kstandbridge): Get element from hastable introspection?
    
    debug_element *Result = 0;
    
    for(debug_element *Chain = DebugState->ElementHash[Index];
        Chain;
        Chain = Chain->NextInHash)
    {
        if(StringsAreEqual(Chain->ParsedName.GUID, String_(GetNullTerminiatedStringLength(GUID), (u8 *)GUID)))
        {
            Result = Chain;
            break;
        }
    }
    
    return Result;
}

enum debug_element_add_op
{
    DebugElement_AddToGroup = 0x1,
    DebugElement_CreateHierarchy = 0x2,
};

internal debug_element *
GetElementFromEvent(debug_state *DebugState, debug_event *Event, debug_variable_link *Parent,
                    u32 Op)
{
    Assert(Event->GUID);
    
    if(!Parent)
    {
        Parent = DebugState->RootGroup;
    }
    
    debug_parsed_name ParsedName = DebugParseName(Event->GUID);
    u32 Index = (ParsedName.HashValue % ArrayCount(DebugState->ElementHash));
    
    debug_element *Result = GetElementFromGUID(DebugState, Index, Event->GUID);
    if(!Result)
    {
        Result = PushStruct(&DebugState->Arena, debug_element);
        ZeroStruct(*Result);
        Result->ParsedName = ParsedName;
        Result->Type = (debug_event_type)Event->Type;
        
        Result->NextInHash = DebugState->ElementHash[Index];
        DebugState->ElementHash[Index] = Result;
        
        debug_variable_link *ParentGroup = Parent;
        if(Op & DebugElement_CreateHierarchy)
        {
            InvalidCodePath
        }
        
        if(Op & DebugElement_AddToGroup)
        {
            AddElementToGroup(DebugState, ParentGroup, Result);
        }
        
    }
    
    return Result;
}

internal void
FreeFrame(debug_state *DebugState, u32 FrameOrdinal)
{
    Assert(FrameOrdinal < DEBUG_FRAME_COUNT);
    
    u32 FreedEventCount = 0;
    
    for(u32 ElementHashIndex = 0;
        ElementHashIndex < ArrayCount(DebugState->ElementHash);
        ++ElementHashIndex)
    {
        for(debug_element *Element = DebugState->ElementHash[ElementHashIndex];
            Element;
            Element = Element->NextInHash)
        {
            debug_element_frame *ElementFrame = Element->Frames + FrameOrdinal;
            while(ElementFrame->OldestEvent)
            {
                debug_stored_event *FreeEvent = ElementFrame->OldestEvent;
                ElementFrame->OldestEvent = FreeEvent->Next;
                FREELIST_DEALLOCATE(FreeEvent, DebugState->FirstFreeStoredEvent);
                ++FreedEventCount;
            }
            ZeroStruct(*ElementFrame);
        }
    }
    
    debug_frame *Frame = DebugState->Frames + FrameOrdinal;
    Assert(Frame->StoredEventCount == FreedEventCount);
    
    ZeroStruct(*Frame);
}

inline void
FreeOldestFrame(debug_state *DebugState)
{
    FreeFrame(DebugState, DebugState->OldestFrameOrdinal);
    if(DebugState->OldestFrameOrdinal == DebugState->MostRecentFrameOrdinal)
    {
        IncrementFrameOrdinal(&DebugState->MostRecentFrameOrdinal);
    }
    IncrementFrameOrdinal(&DebugState->OldestFrameOrdinal);
}

internal debug_stored_event *
StoreEvent(debug_state *DebugState, debug_element *Element, debug_event *Event)
{
    debug_stored_event *Result = 0;
    while(!Result)
    {
        // TODO(kstandbridge): Freelist introspection?
        Result = DebugState->FirstFreeStoredEvent;
        if(Result)
        {
            DebugState->FirstFreeStoredEvent = Result->NextFree;
        }
        else
        {
            if(ArenaHasRoomFor(&DebugState->Arena, sizeof(debug_stored_event)))
            {
                Result = PushStruct(&DebugState->Arena, debug_stored_event);
            }
            else
            {
                FreeOldestFrame(DebugState);
            }
        }
    }
    
    debug_frame *CollationFrame = DebugState->Frames + DebugState->CollationFrameOrdinal;
    
    Result->Next = 0;
    Result->FrameIndex = CollationFrame->FrameIndex;
    Result->Event = *Event;
    
    ++CollationFrame->StoredEventCount;
    
    debug_element_frame *Frame = Element->Frames + DebugState->CollationFrameOrdinal;
    if(Frame->MostRecentEvent)
    {
        Frame->MostRecentEvent = Frame->MostRecentEvent->Next = Result;
    }
    else
    {
        Frame->OldestEvent = Frame->MostRecentEvent = Result;
    }
    
    return Result;
}

inline open_debug_block *
AllocateOpenDebugBlock(debug_state *DebugState, debug_element *Element, u32 FrameIndex, debug_event *Event, open_debug_block **FirstOpenBlock)
{
    open_debug_block *Result = 0;
    FREELIST_ALLOCATE(Result, DebugState->FirstFreeBlock, PushStruct(&DebugState->Arena, open_debug_block));
    
    Result->StartingFrameIndex = FrameIndex;
    Result->BeginClock = Event->Clock;
    Result->Element = Element;
    Result->NextFree = 0;
    
    Result->Parent = *FirstOpenBlock;
    *FirstOpenBlock = Result;
    
    return Result;
    
}

inline void
DeallocateOpenDebugBlock(debug_state *DebugState, open_debug_block **FirstOpenBlock)
{
    open_debug_block *FreeBlock = *FirstOpenBlock;
    *FirstOpenBlock = FreeBlock->Parent;
    
    FreeBlock->NextFree = DebugState->FirstFreeBlock;
    DebugState->FirstFreeBlock = FreeBlock;
}

internal void
CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
{
    for(u32 EventIndex = 0;
        EventIndex < EventCount;
        ++EventIndex)
    {
        debug_event *Event = EventArray + EventIndex;
        if(Event->Type == DebugEvent_FrameEnd)
        {
            debug_frame *CollationFrame = DebugState->Frames + DebugState->CollationFrameOrdinal;
            
            CollationFrame->EndClock = Event->Clock;
            if(CollationFrame->RootProfileNode)
            {
                CollationFrame->RootProfileNode->ProfileNode.Duration = (CollationFrame->EndClock - CollationFrame->BeginClock);
            }
            CollationFrame->SecondsElapsed = Event->Value_F32;
            
            // TODO(kstandbridge): Skip a frame when paused
            if(DebugState->Paused)
            {
                FreeFrame(DebugState, DebugState->CollationFrameOrdinal);
            }
            else
            {
                DebugState->MostRecentFrameOrdinal = DebugState->CollationFrameOrdinal;
                IncrementFrameOrdinal(&DebugState->CollationFrameOrdinal);
                if(DebugState->CollationFrameOrdinal == DebugState->OldestFrameOrdinal)
                {
                    FreeOldestFrame(DebugState);
                }
                CollationFrame = DebugState->Frames + DebugState->CollationFrameOrdinal;
            }
            CollationFrame->FrameIndex = DebugState->TotalFrameCount++;
            CollationFrame->BeginClock = Event->Clock;
        }
        else
        {
            debug_frame *CollationFrame = DebugState->Frames + DebugState->CollationFrameOrdinal;
            Assert(CollationFrame);
            u32 FrameIndex = DebugState->TotalFrameCount - 1;
            debug_thread *Thread = GetDebugThread(DebugState, Event->ThreadId);
            u64 RelativeClock = Event->Clock - CollationFrame->BeginClock;
            
            switch(Event->Type)
            {
                case DebugEvent_BeginBlock:
                {
                    ++CollationFrame->ProfileBlockCount;
                    debug_element *Element = GetElementFromEvent(DebugState, Event, DebugState->ProfileGroup,
                                                                 DebugElement_AddToGroup);
                    debug_stored_event *ParentEvent = CollationFrame->RootProfileNode;
                    u64 ClockBasis = CollationFrame->BeginClock;
                    if(Thread->FirstOpenCodeBlock)
                    {
                        ParentEvent = Thread->FirstOpenCodeBlock->Node;
                        ClockBasis = Thread->FirstOpenCodeBlock->BeginClock;
                    } 
                    else if(!ParentEvent)
                    {
                        debug_event NullEvent;
                        ZeroStruct(NullEvent);
                        ParentEvent = StoreEvent(DebugState, DebugState->RootProfileElement, &NullEvent);
                        debug_profile_node *Node = &ParentEvent->ProfileNode;
                        Node->Element = 0;
                        Node->FirstChild = 0;
                        Node->NextSameParent = 0;
                        Node->ParentRelativeClock = 0;
                        Node->Duration = 0;
                        Node->DurationOfChildren = 0;
                        Node->ThreadOrdinal = 0;
                        
                        ClockBasis = CollationFrame->BeginClock;
                        CollationFrame->RootProfileNode = ParentEvent;
                    }
                    
                    debug_stored_event *StoredEvent = StoreEvent(DebugState, Element, Event);
                    debug_profile_node *Node = &StoredEvent->ProfileNode;
                    Node->Element = Element;
                    Node->FirstChild = 0;
                    Node->ParentRelativeClock = Event->Clock - ClockBasis;
                    Node->Duration = 0;
                    Node->DurationOfChildren = 0;
                    Node->ThreadOrdinal = (u16)Thread->LaneIndex;
                    
                    Node->NextSameParent = ParentEvent->ProfileNode.FirstChild;
                    ParentEvent->ProfileNode.FirstChild = StoredEvent;
                    
                    open_debug_block *DebugBlock = AllocateOpenDebugBlock(DebugState, Element, FrameIndex, Event,
                                                                          &Thread->FirstOpenCodeBlock);
                    DebugBlock->Node = StoredEvent;
                } break;
                
                case DebugEvent_EndBlock:
                {
                    if(Thread->FirstOpenCodeBlock)
                    {
                        open_debug_block *MatchingBlock = Thread->FirstOpenCodeBlock;
                        Assert(Thread->Id == Event->ThreadId);
                        
                        debug_profile_node *Node = &MatchingBlock->Node->ProfileNode;
                        Node->Duration = Event->Clock - MatchingBlock->BeginClock;
                        
                        DeallocateOpenDebugBlock(DebugState, &Thread->FirstOpenCodeBlock);
                        
                        if(Thread->FirstOpenCodeBlock)
                        {
                            debug_profile_node *ParentNode = &Thread->FirstOpenCodeBlock->Node->ProfileNode;
                            ParentNode->DurationOfChildren += Node->Duration;
                        }
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                } break;
                
                InvalidDefaultCase;
            }
        }
    }
}

extern void
DebugUpdateFrame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena, app_input *Input)
{
    GlobalDebugEventTable->CurrentEventArrayIndex = !GlobalDebugEventTable->CurrentEventArrayIndex;
    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugEventTable->EventArrayIndex_EventIndex,
                                                  (u64)GlobalDebugEventTable->CurrentEventArrayIndex << 32);
    
    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
    Assert(EventArrayIndex <= 1);
    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;
    
    if(!PlatformAPI->DebugState)
    {
        PlatformAPI->DebugState = PushStruct(Arena, debug_state);
        ZeroStruct(*PlatformAPI->DebugState);
        debug_state *DebugState = PlatformAPI->DebugState;
        SubArena(&DebugState->Arena, Arena, Kilobytes(256));
        
        DebugState->RootGroup = CreateVariableLink(DebugState, 4, "Root");
        DebugState->ProfileGroup = CreateVariableLink(DebugState, 7, "Profile");
        debug_event RootProfileEvent;
        ZeroStruct(RootProfileEvent);
        RootProfileEvent.GUID = GenerateDebugId("RootProfile");
        DebugState->RootProfileElement = GetElementFromEvent(DebugState, &RootProfileEvent, 0, 0);
    }
    debug_state *DebugState = PlatformAPI->DebugState;
    
    CollateDebugRecords(DebugState, EventCount, GlobalDebugEventTable->Events[EventArrayIndex]);
    
    if(!DebugState->Paused)
    {
        DebugState->ViewingFrameOrdinal = DebugState->MostRecentFrameOrdinal;
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
        PushRenderCommandRectangle(RenderGroup, V4(0.3f, 0.3f, 0.3f, 1.0f), RegionRect, 1.0f);
        PushRenderCommandRectangleOutline(RenderGroup, 1.0f, Color, RegionRect, 2.0f);
        
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
DrawDebugGrid(debug_state *DebugState, ui_state *UIState, render_group *RenderGroup, memory_arena *PermArena, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    if(DebugState)
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
                    if(Button(&FrameGrid, RenderGroup, 0, 0, GenerateInteractionId(DebugState), DebugState, DebugState->Paused ? String("Resume") : String("Pause"))) 
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
                        PushRenderCommandRectangleOutline(RenderGroup, Thickness, V4(0, 0.5f, 0, 1), BarBounds, 2.0f);
                        
                        v2 Dim = V2Subtract(BarBounds.Max, BarBounds.Min);
                        f32 MaxScale = 100.0f;
                        BarBounds.Max.Y = BarBounds.Min.Y + (Dim.Y / MaxScale)*(Frame->SecondsElapsed*1000.0f);
                        
                        v4 Color = V4(0.0f, 1.0f, 0.0f, 1.0f);
                        if(NewFrameIndex == DebugState->ViewingFrameOrdinal)
                        {
                            Color = V4(1.0f, 1.0f, 0.0f, 1.0f);
                        }
                        
                        PushRenderCommandRectangle(RenderGroup, Color, BarBounds, 1.0f);
                    }
                }
                EndGrid(&FrameGrid);
            }
            END_BLOCK();
            
            ui_grid CurrentFrameGrid = BeginGrid(UIState, TempArena, GetCellBounds(&ProfileSplit, 0, 1), 2, 1);
            {
                SetRowHeight(&CurrentFrameGrid, 1, SIZE_AUTO);
                debug_frame *Frame = DebugState->Frames + DebugState->ViewingFrameOrdinal;
                
                f32 MsPerFrame = Frame->SecondsElapsed;
                f32 FramesPerSecond = 1.0f / MsPerFrame;
                Button(&CurrentFrameGrid, RenderGroup, 0, 0, GenerateInteractionId(DebugState), DebugState, FormatString(TempArena, "%d : %.02f ms %.02f fps | Arena %u / %u", 
                                                                                                                         Frame->FrameIndex, MsPerFrame*1000.0f, FramesPerSecond,
                                                                                                                         TempArena->Used/1024, TempArena->Size/1024));
                
                ui_grid TopClockSplitGrid = BeginGrid(UIState, TempArena, GetCellBounds(&CurrentFrameGrid, 0, 1), 1, 2);
                {
                    SetRowHeight(&TopClockSplitGrid, 0, SIZE_AUTO);
                    
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
            
#if 0            
            ui_grid DebugGrid = BeginGrid(UIState, TempArena, GetCellBounds(&ProfileSplit, 0, 2), FrameCount, 2);
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
            }
            EndGrid(&DebugGrid);
#endif
            PushRenderCommandText(RenderGroup, 2.0f, V2(50, 800), V4(0, 0, 0, 1), 
                                  FormatString(TempArena, "PermArena %u / %u\nTempArena %u / %u", 
                                               DebugState->Arena.Used, DebugState->Arena.Size,
                                               TempArena->Used, TempArena->Size));
            
        }
        EndGrid(&ProfileSplit);
        
    }
}
