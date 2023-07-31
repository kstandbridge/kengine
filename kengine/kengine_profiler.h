
internal u64
EstimateCPUTimerFrequency()
{
    u64 Result = 0;

    u64 MillisecondsToWait = 100;
    u64 OSFrequency = PlatformGetOSTimerFrequency();

    u64 CPUStart = PlatformReadCPUTimer();
    u64 OSStart = PlatformReadOSTimer();
    u64 OSEnd = 0;
    u64 OSElapsed = 0;
    u64 OSWaitTime = OSFrequency * MillisecondsToWait / 1000;

    while(OSElapsed < OSWaitTime)
    {
        OSEnd = PlatformReadOSTimer();
        OSElapsed = OSEnd - OSStart;
    }

    u64 CPUEnd = PlatformReadCPUTimer();
    u64 CPUElapsed = CPUEnd - CPUStart;

    if(OSElapsed)
    {
        Result = OSFrequency * CPUElapsed / OSElapsed;
    }

    return Result;
}

typedef struct profile_anchor
{
    u64 TSCElapsedExclusive; //NOTE(kstandbridge): Does NOT include children
    u64 TSCElapsedInclusive; //NOTE(kstandbridge): DOES include children
    u64 HitCount;
    char const *Label;
} profile_anchor;

typedef struct profiler
{
    profile_anchor Anchors[4096];

    u64 StartTSC;
    u64 EndTSC;
} profiler;
global profiler GlobalProfiler;
global u32 GlobalProfilerParent;

typedef struct profile_block
{
    char const *Label;
    u64 OldTSCElapsedInclusive;
    u64 StartTSC;
    u32 ParentIndex;
    u32 AnchorIndex;
} profile_block;

internal void
EndProfileBlock_(profile_block Block)
{
    u64 Elapsed = PlatformReadCPUTimer() - Block.StartTSC;
    GlobalProfilerParent = Block.ParentIndex;
    
    profile_anchor *Parent = GlobalProfiler.Anchors + Block.ParentIndex;
    profile_anchor *Anchor = GlobalProfiler.Anchors + Block.AnchorIndex;
    
    Parent->TSCElapsedExclusive -= Elapsed;
    Anchor->TSCElapsedExclusive += Elapsed;
    Anchor->TSCElapsedInclusive = Block.OldTSCElapsedInclusive + Elapsed;
    ++Anchor->HitCount;

    Anchor->Label = Block.Label;
}

#define END_TIMED_BLOCK(Name) EndProfileBlock_(ProfileBlock##Name)
#define END_TIMED_FUNCTION() END_TIMED_BLOCK(__FUNCTION__)

#define BEGIN_TIMED_BLOCK_(Name, BlockLabel) profile_block ProfileBlock##Name = { .Label = BlockLabel, .StartTSC = PlatformReadCPUTimer(), .ParentIndex = GlobalProfilerParent, .AnchorIndex = __COUNTER__ + 1}; \
{ \
    GlobalProfilerParent = ProfileBlock##Name.AnchorIndex; \
    profile_anchor *Achor = GlobalProfiler.Anchors + ProfileBlock##Name.AnchorIndex; \
    ProfileBlock##Name.OldTSCElapsedInclusive = Achor->TSCElapsedInclusive; \
}
#define BEGIN_TIMED_BLOCK(Name) BEGIN_TIMED_BLOCK_(Name, #Name)
#define BEGIN_TIMED_FUNCTION() BEGIN_TIMED_BLOCK_(__FUNCTION__, __FUNCTION__)

internal void
BeginProfile()
{
    GlobalProfiler.StartTSC = PlatformReadCPUTimer();
}

internal void
EndAndPrintProfile()
{
    // TODO(kstandbridge): Check to see if any profile_blocks were not ended
    
    GlobalProfiler.EndTSC = PlatformReadCPUTimer();
    u64 CPUFrequency = EstimateCPUTimerFrequency();

    u64 TotalCPUElapsed = GlobalProfiler.EndTSC - GlobalProfiler.StartTSC;

    if(CPUFrequency)
    {
        PlatformConsoleOut("\nTotal time: %.4fms (CPU freq %lu)\n", (f64)TotalCPUElapsed / (f64)CPUFrequency, CPUFrequency);
    }

    for(u32 AnchorIndex = 0;
        AnchorIndex < ArrayCount(GlobalProfiler.Anchors);
        ++AnchorIndex)
    {
        profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;
        if(Anchor->TSCElapsedInclusive)
        {
            f64 Percent = 100.0f * ((f64)Anchor->TSCElapsedExclusive / (f64)TotalCPUElapsed);
            PlatformConsoleOut("\t%s[%lu]: %lu (%.2f%%", Anchor->Label, Anchor->HitCount, Anchor->TSCElapsedExclusive, Percent);
            if(Anchor->TSCElapsedInclusive != Anchor->TSCElapsedExclusive)
            {
                f64 PercentWithChildren = 100.0f * ((f64)Anchor->TSCElapsedInclusive / (f64)TotalCPUElapsed);
                PlatformConsoleOut(", %.2f%% w/children", PercentWithChildren);
            }
            PlatformConsoleOut(")\n");
        }
    }

    PlatformConsoleOut("\n");
}
