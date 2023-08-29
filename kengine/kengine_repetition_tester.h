#include "kengine/kengine_types.h"
typedef enum test_mode_type
{
    TestMode_Unitialized,
    TestMode_Testing,
    TestMode_Completed,
    TestMode_Error,
} test_mode_type;

typedef struct repetition_test_results
{
    u64 TestCount;
    u64 TotalTime;
    u64 MaxTime;
    u64 MinTime;
} repetition_test_results;

typedef struct repetition_tester
{
    u64 TargetProcessedByteCount;
    u64 CPUTimerFreq;
    u64 TryForTime;
    u64 TestsStartedAt;

    test_mode_type Mode;
    b32 PrintNewMinimums;
    u32 OpenBlockCount;
    u32 CloseBlockCount;
    u64 TimeAccumlatedOnThisTest;
    u64 BytesAccumulatedOnThisTest;

    repetition_test_results Results;
} repetition_tester;

internal f64
SecondsFromCPUTime(f64 CPUTime, u64 CPUTimerFreq)
{
    f64 Result = 0.0;

    if(CPUTimerFreq)
    {
        Result = (CPUTime / (f64)CPUTimerFreq);
    }

    return Result;
}

internal void
PrintTime(char *Label, u64 CPUTime, u64 CPUTimerFreq, u64 ByteCount)
{
    PlatformConsoleOut("%s: %u", Label, CPUTime);

    if(CPUTimerFreq)
    {
        f64 Seconds = SecondsFromCPUTime(CPUTime, CPUTimerFreq);
        PlatformConsoleOut(" (%fms)", 1000.0f*Seconds);

        if(ByteCount)
        {
            f64 BestBandwidth = ByteCount / (Gigabytes(1)*Seconds);
            PlatformConsoleOut(" %fgb/s", BestBandwidth);
        }
    }
}

internal void
RepetitionTestPrintResults(repetition_test_results Results, u64 CPUTimerFreq, u64 ByteCount)
{
    PrintTime("Min", (f64)Results.MinTime, CPUTimerFreq, ByteCount);
    PlatformConsoleOut("\n");

    PrintTime("Max", (f64)Results.MaxTime, CPUTimerFreq, ByteCount);
    PlatformConsoleOut("\n");

    if(Results.TestCount)
    {
        PrintTime("Avg", (f64)Results.TotalTime / (f64)Results.TestCount, CPUTimerFreq, ByteCount);
        PlatformConsoleOut("\n");
    }
}

internal void
RepetitionTestError(repetition_tester *Tester, char *Message)
{
    Tester->Mode = TestMode_Error;
    PlatformConsoleOut("Error: %s\n", Message);
}

internal void
RepetitionTestNewTestWave(repetition_tester *Tester, u64 TargetProcessedByteCount, u64 CPUTimerFreq, u32 SecondsToType)
{
    if(Tester->Mode == TestMode_Unitialized)
    {
        Tester->Mode = TestMode_Testing;
        Tester->TargetProcessedByteCount = TargetProcessedByteCount;
        Tester->CPUTimerFreq = CPUTimerFreq;
        Tester->PrintNewMinimums = true;
        Tester->Results.MinTime = (u64)-1;
    }
    else if(Tester->Mode == TestMode_Completed)
    {
        Tester->Mode = TestMode_Testing;
        
        if(Tester->TargetProcessedByteCount != TargetProcessedByteCount)
        {
            RepetitionTestError(Tester, "TargetProcessedByteCount changed");
        }

        if(Tester->CPUTimerFreq != CPUTimerFreq)
        {
            RepetitionTestError(Tester, "CPU frequency changed");
        }
    }

    Tester->TryForTime = SecondsToType*CPUTimerFreq;
    Tester->TestsStartedAt = PlatformReadCPUTimer();
}

internal void
RepetitionTestBeginTime(repetition_tester *Tester)
{
    ++Tester->OpenBlockCount;
    Tester->TimeAccumlatedOnThisTest -= PlatformReadCPUTimer();
}

internal void
RepetitionTestEndTime(repetition_tester *Tester)
{
    ++Tester->CloseBlockCount;
    Tester->TimeAccumlatedOnThisTest += PlatformReadCPUTimer();
}

internal void
RepetitionTestCountBytes(repetition_tester *Tester, u64 ByteCount)
{
    Tester->BytesAccumulatedOnThisTest += ByteCount;
}

internal b32
RepetitionTestIsTesting(repetition_tester *Tester)
{
    if(Tester->Mode == TestMode_Testing)
    {
        u64 CurrentTime = PlatformReadCPUTimer();

        // NOTE(kstandbridge): We don't that hads no timing blocks, we assume they took some other path
        if(Tester->OpenBlockCount)
        {
            if(Tester->OpenBlockCount != Tester->CloseBlockCount)
            {
                RepetitionTestError(Tester, "Unbalanced BeginTime/EndTime");
            }

            if(Tester->BytesAccumulatedOnThisTest != Tester->TargetProcessedByteCount)
            {
                RepetitionTestError(Tester, "Processed byte count mismatch");
            }

            if(Tester->Mode == TestMode_Testing)
            {
                repetition_test_results *Results = &Tester->Results;
                u64 ElapsedTime = Tester->TimeAccumlatedOnThisTest;
                Results->TestCount += 1;
                Results->TotalTime += ElapsedTime;
                if(Results->MaxTime < ElapsedTime)
                {
                    Results->MaxTime = ElapsedTime;
                }

                if(Results->MinTime > ElapsedTime)
                {
                    Results->MinTime = ElapsedTime;

                    // NOTE(kstandbridge): Whenever we get a new minimum time, we reset the clock to the full trial time
                    Tester->TestsStartedAt = CurrentTime;

                    if(Tester->PrintNewMinimums)
                    {
                        PrintTime("Min", Results->MinTime, Tester->CPUTimerFreq, Tester->BytesAccumulatedOnThisTest);
                        PlatformConsoleOut("              \r");
                    }
                }

                Tester->OpenBlockCount = 0;
                Tester->CloseBlockCount = 0;
                Tester->TimeAccumlatedOnThisTest = 0;
                Tester->BytesAccumulatedOnThisTest = 0;
            }
        }

        if((CurrentTime - Tester->TestsStartedAt) > Tester->TryForTime)
        {
            Tester->Mode = TestMode_Completed;

            PlatformConsoleOut("                                                          \r");
            RepetitionTestPrintResults(Tester->Results, Tester->CPUTimerFreq, Tester->TargetProcessedByteCount);
        }
    }

    b32 Result = (Tester->Mode == TestMode_Testing);
    return Result;
}
