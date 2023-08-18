
#ifdef KENGINE_PROFILER
    #undef KENGINE_PROFILER
    #define KENGINE_PROFILER 1
#endif


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

#if KENGINE_PROFILER

typedef struct profile_anchor
{
    u64 TSCElapsedExclusive; //NOTE(kstandbridge): Does NOT include children
    u64 TSCElapsedInclusive; //NOTE(kstandbridge): DOES include children
    u64 HitCount;
    u64 ProcessedByteCount;
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
EndProfileBlock_(profile_block Block, u64 ByteCount)
{
    u64 Elapsed = PlatformReadCPUTimer() - Block.StartTSC;
    GlobalProfilerParent = Block.ParentIndex;
    
    profile_anchor *Parent = GlobalProfiler.Anchors + Block.ParentIndex;
    profile_anchor *Anchor = GlobalProfiler.Anchors + Block.AnchorIndex;
    
    Parent->TSCElapsedExclusive -= Elapsed;
    Anchor->TSCElapsedExclusive += Elapsed;
    Anchor->TSCElapsedInclusive = Block.OldTSCElapsedInclusive + Elapsed;
    Anchor->ProcessedByteCount += ByteCount;
    ++Anchor->HitCount;

    Anchor->Label = Block.Label;
}

#define END_TIMED_BLOCK(Name) END_TIMED_BANDWIDTH(Name, 0)
#define END_TIMED_FUNCTION() END_TIMED_BANDWIDTH(__FUNCTION__, 0)
#define END_TIMED_BANDWIDTH(Name, ByteCount) EndProfileBlock_(ProfileBlock##Name, ByteCount)

#define BEGIN_TIMED_BLOCK_(Name, BlockLabel) profile_block ProfileBlock##Name = { .Label = BlockLabel, .StartTSC = PlatformReadCPUTimer(), .ParentIndex = GlobalProfilerParent, .AnchorIndex = __COUNTER__ + 1}; \
{ \
    GlobalProfilerParent = ProfileBlock##Name.AnchorIndex; \
    profile_anchor *Anchor = GlobalProfiler.Anchors + ProfileBlock##Name.AnchorIndex; \
    ProfileBlock##Name.OldTSCElapsedInclusive = Anchor->TSCElapsedInclusive; \
}
#define BEGIN_TIMED_BLOCK(Name) BEGIN_TIMED_BLOCK_(Name, #Name)
#define BEGIN_TIMED_BANDWIDTH(Name) BEGIN_TIMED_BLOCK_(Name, #Name)
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
            PlatformConsoleOut(")");

            if(Anchor->ProcessedByteCount)
            {
                f64 Seconds = (f64)Anchor->TSCElapsedInclusive / (f64)CPUFrequency;
                f64 BytesPerSecond = (f64)Anchor->ProcessedByteCount / Seconds;
                f64 ToltalMegabytes = (f64)Anchor->ProcessedByteCount / (f64)Megabytes(1);
                f64 GigabytesPerSecond = BytesPerSecond / Gigabytes(1);

                PlatformConsoleOut(" %.3fmb at %.2fgb/s", ToltalMegabytes, GigabytesPerSecond);
            }

            PlatformConsoleOut("\n");
        }
    }

    PlatformConsoleOut("\n");
}

#else

#define END_TIMED_BLOCK(...)
#define END_TIMED_FUNCTION(...)

#define BEGIN_TIMED_BLOCK(...)
#define BEGIN_TIMED_FUNCTION(...)

typedef struct profiler
{
    u64 StartTSC;
    u64 EndTSC;
} profiler;
global profiler GlobalProfiler;

internal void
BeginProfile()
{
    GlobalProfiler.StartTSC = PlatformReadCPUTimer();
}

internal void
EndAndPrintProfile()
{
    GlobalProfiler.EndTSC = PlatformReadCPUTimer();
    u64 CPUFrequency = EstimateCPUTimerFrequency();

    u64 TotalCPUElapsed = GlobalProfiler.EndTSC - GlobalProfiler.StartTSC;

    if(CPUFrequency)
    {
        PlatformConsoleOut("\nTotal time: %.4fms (CPU freq %lu)\n\n", (f64)TotalCPUElapsed / (f64)CPUFrequency, CPUFrequency);
    }
}

#endif