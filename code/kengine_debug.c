inline void
IncrementFrameOrdinal(u32 *Ordinal)
{
    *Ordinal = (*Ordinal + 1) % DEBUG_FRAME_COUNT;
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
    Result.GUID = CStringToString(GUID);
    
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
        if(StringsAreEqual(Chain->ParsedName.GUID, CStringToString(GUID)))
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
            Result = PushStruct(&DebugState->Arena, debug_stored_event);
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
DebugUpdateFrame(app_memory *Memory)
{
    GlobalDebugEventTable->CurrentEventArrayIndex = !GlobalDebugEventTable->CurrentEventArrayIndex;
    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugEventTable->EventArrayIndex_EventIndex,
                                                  (u64)GlobalDebugEventTable->CurrentEventArrayIndex << 32);
    
    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
    Assert(EventArrayIndex <= 1);
    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;
    
    debug_state *DebugState = Memory->DebugState;
    if(!DebugState)
    {
        DebugState = Memory->DebugState = BootstrapPushStruct(debug_state, Arena);
        
        DebugState->RootGroup = CreateVariableLink(DebugState, 4, "Root");
        DebugState->ProfileGroup = CreateVariableLink(DebugState, 7, "Profile");
        debug_event RootProfileEvent;
        ZeroStruct(RootProfileEvent);
        RootProfileEvent.GUID = GenerateDebugId("RootProfile");
        DebugState->RootProfileElement = GetElementFromEvent(DebugState, &RootProfileEvent, 0, 0);
        DebugState->CurrentView = DebugView_Profiler;
    }
    
    CollateDebugRecords(DebugState, EventCount, GlobalDebugEventTable->Events[EventArrayIndex]);
    
    if(!DebugState->Paused)
    {
        DebugState->ViewingFrameOrdinal = DebugState->MostRecentFrameOrdinal;
    }
}
