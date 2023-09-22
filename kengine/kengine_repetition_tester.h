#include "kengine/kengine_types.h"
typedef enum test_mode_type
{
    TestMode_Unitialized,
    TestMode_Testing,
    TestMode_Completed,
    TestMode_Error,
} test_mode_type;

typedef enum repetition_value_type
{
    RepValue_TestCount,

    RepValue_CPUTimer,
    RepValue_MemPageFaults,
    RepValue_ByteCount,

    RepValue_Count,
} repetition_value_type;

typedef struct repetition_value
{
    u64 E[RepValue_Count];
} repetition_value;

typedef struct repetition_test_results
{
    repetition_value Total;
    repetition_value Min;
    repetition_value Max;
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

    repetition_value AccumlatedOnThisTest;
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
RepetitionTestPrintValue(char *Label, repetition_value Value, u64 CPUTimerFreq)
{
    u64 TestCount = Value.E[RepValue_TestCount];
    f64 Divisor = TestCount ? (f64)TestCount : 1;

    f64 E[RepValue_Count];
    for(u32 EIndex = 0;
        EIndex < ArrayCount(E);
        ++EIndex)
    {
        E[EIndex] = (f64)Value.E[EIndex] / Divisor;
    }

    PlatformConsoleOut("%s: %.0f", Label, E[RepValue_CPUTimer]);
    if(CPUTimerFreq)
    {
        f64 Seconds = SecondsFromCPUTime(E[RepValue_CPUTimer], CPUTimerFreq);
        PlatformConsoleOut(" (%fms)", 1000.0f*Seconds);

        if(E[RepValue_ByteCount] > 0)
        {
            f64 Bandwidth = E[RepValue_ByteCount] / (Gigabytes(1) * Seconds);
            PlatformConsoleOut(" %fgb/s", Bandwidth);
        }
    }

    if(E[RepValue_MemPageFaults] > 0)
    {
        PlatformConsoleOut(" PF: %0.4f (%0.4fk/fault)", E[RepValue_MemPageFaults], E[RepValue_ByteCount] / (E[RepValue_MemPageFaults] * 1024.0));
    }
}

internal void
RepetitionTestPrintResults(repetition_test_results Results, u64 CPUTimerFreq)
{
    RepetitionTestPrintValue("Min", Results.Min, CPUTimerFreq);
    PlatformConsoleOut("\n", 0);
    RepetitionTestPrintValue("Max", Results.Max, CPUTimerFreq);
    PlatformConsoleOut("\n", 0);
    RepetitionTestPrintValue("Avg", Results.Total, CPUTimerFreq);
    PlatformConsoleOut("\n", 0);
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
        Tester->Results.Min.E[RepValue_CPUTimer] = (u64)-1;
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

    repetition_value *Accum = &Tester->AccumlatedOnThisTest;
    Accum->E[RepValue_MemPageFaults] -= PlatformReadOSPageFaultCount();
    Accum->E[RepValue_CPUTimer] -= PlatformReadCPUTimer();
}

internal void
RepetitionTestEndTime(repetition_tester *Tester)
{
    repetition_value *Accum = &Tester->AccumlatedOnThisTest;
    Accum->E[RepValue_CPUTimer] += PlatformReadCPUTimer();
    Accum->E[RepValue_MemPageFaults] += PlatformReadOSPageFaultCount();

    ++Tester->CloseBlockCount;
}

internal void
RepetitionTestCountBytes(repetition_tester *Tester, u64 ByteCount)
{
    repetition_value *Accum = &Tester->AccumlatedOnThisTest;
    Accum->E[RepValue_ByteCount] += ByteCount;

}

internal b32
RepetitionTestIsTesting(repetition_tester *Tester)
{
    if(Tester->Mode == TestMode_Testing)
    {
        repetition_value Accum = Tester->AccumlatedOnThisTest;
        u64 CurrentTime = PlatformReadCPUTimer();

        // NOTE(kstandbridge): We don't that hads no timing blocks, we assume they took some other path
        if(Tester->OpenBlockCount)
        {
            if(Tester->OpenBlockCount != Tester->CloseBlockCount)
            {
                RepetitionTestError(Tester, "Unbalanced BeginTime/EndTime");
            }

            if(Accum.E[RepValue_ByteCount] != Tester->TargetProcessedByteCount)
            {
                RepetitionTestError(Tester, "Processed byte count mismatch");
            }

            if(Tester->Mode == TestMode_Testing)
            {
                repetition_test_results *Results = &Tester->Results;

                Accum.E[RepValue_TestCount] = 1;
                for(u32 EIndex = 0;
                    EIndex < ArrayCount(Accum.E);
                    ++EIndex)
                {
                    Results->Total.E[EIndex] += Accum.E[EIndex];
            
                }
                if(Results->Max.E[RepValue_CPUTimer] < Accum.E[RepValue_CPUTimer])
                {
                    Results->Max = Accum;
                }

                if(Results->Min.E[RepValue_CPUTimer] > Accum.E[RepValue_CPUTimer])
                {
                    Results->Min = Accum;

                    // NOTE(kstandbridge): Whenever we get a new minimum time, we reset the clock to the full trial time
                    Tester->TestsStartedAt = CurrentTime;

                    if(Tester->PrintNewMinimums)
                    {
                        RepetitionTestPrintValue("Min", Results->Min, Tester->CPUTimerFreq);
                        PlatformConsoleOut("                                   \r", 0);
                    }
                }

                Tester->OpenBlockCount = 0;
                Tester->CloseBlockCount = 0;
                ZeroStruct(Tester->AccumlatedOnThisTest);
            }
        }

        if((CurrentTime - Tester->TestsStartedAt) > Tester->TryForTime)
        {
            Tester->Mode = TestMode_Completed;

            PlatformConsoleOut("                                                          \r", 0);
            RepetitionTestPrintResults(Tester->Results, Tester->CPUTimerFreq);
        }
    }

    b32 Result = (Tester->Mode == TestMode_Testing);
    return Result;
}
