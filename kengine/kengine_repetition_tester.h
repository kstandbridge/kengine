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

    StatValue_Seconds,
    StatValue_GBPerSecond,
    StatValue_KBPerPageFault,

    RepValue_Count,
} repetition_value_type;

typedef struct repetition_value
{
    u64 E[RepValue_Count];

    // NOTE(kstandbridge): These values are computed fropm the E[] array and the CPUTimerFrequency
    f64 PerCount[RepValue_Count];
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

typedef struct repetition_series_label
{
    char Chars[64];
} repetition_series_label;

typedef struct repetition_test_series
{
    string Buffer;

    u32 MaxRowCount;
    u32 ColumnCount;

    u32 RowIndex;
    u32 ColumnIndex;

    repetition_test_results *TestResults;    // NOTE(kstandbridge): [RowCount][ColumnCount]
    repetition_series_label *RowLabels;     // NOTE(kstandbridge): [RowCount]
    repetition_series_label *ColumnLabels;  // NOTE(kstandbridge): [ColumnCount]

    repetition_series_label RowLabelLabel;
} repetition_test_series;

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
ComputeDerivedValues(repetition_value *Value, u64 CPUTimerFrequency)
{
    u64 TestCount = Value->E[RepValue_TestCount];
    f64 Divisor = TestCount ? (f64)TestCount : 1;

    f64 *PerCount = Value->PerCount;
    for(u32 EIndex = 0; EIndex < ArrayCount(Value->PerCount); ++EIndex)
    {
        PerCount[EIndex] = (f64)Value->E[EIndex] / Divisor;
    }

    if(CPUTimerFrequency)
    {
        f64 Seconds = SecondsFromCPUTime(PerCount[RepValue_CPUTimer], CPUTimerFrequency);
        PerCount[StatValue_Seconds] = Seconds;

        if(PerCount[RepValue_ByteCount] > 0)
        {
            PerCount[StatValue_GBPerSecond] = PerCount[RepValue_ByteCount] / (Gigabytes(1) * Seconds);
        }
    }

    if(PerCount[RepValue_MemPageFaults] > 0)
    {
        PerCount[StatValue_KBPerPageFault] = PerCount[RepValue_ByteCount] / (PerCount[RepValue_MemPageFaults] * 1024.0f);
    }
}

internal void
RepetitionTestPrintValue(char *Label, repetition_value Value)
{
    PlatformConsoleOut("%s: %.0f", Label, Value.PerCount[RepValue_CPUTimer]);
    PlatformConsoleOut(" (%fms)", 1000.0f*Value.PerCount[StatValue_Seconds]);
    if(Value.PerCount[RepValue_ByteCount] > 0)
    {
        PlatformConsoleOut(" %fgb/s", Value.PerCount[StatValue_GBPerSecond]);
    }

    if(Value.PerCount[RepValue_MemPageFaults] > 0)
    {
        PlatformConsoleOut(" PF: %0.4f (%0.4fk/fault)", Value.PerCount[RepValue_MemPageFaults], Value.PerCount[StatValue_KBPerPageFault]);
    }
}

internal void
RepetitionTestPrintResults(repetition_test_results Results)
{
    RepetitionTestPrintValue("Min", Results.Min);
    PlatformConsoleOut("\n");
    RepetitionTestPrintValue("Max", Results.Max);
    PlatformConsoleOut("\n");
    RepetitionTestPrintValue("Avg", Results.Total);
    PlatformConsoleOut("\n");
}

internal void
RepetitionTestError(repetition_tester *Tester, char *Message)
{
    Tester->Mode = TestMode_Error;
    PlatformConsoleOut("Error: %s\n", Message);
}

internal void
RepetitionTestNewTestWave_(repetition_tester *Tester, u64 TargetProcessedByteCount, u64 CPUTimerFreq, u32 SecondsToTry)
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

    Tester->TryForTime = SecondsToTry*CPUTimerFreq;
    Tester->TestsStartedAt = PlatformReadCPUTimer();
}

internal void
RepetitionTestNewTestWave(repetition_test_series *Series, repetition_tester *Tester, u64 TargetProcessedByteCount, u64 CPUTimerFreq, u32 SecondsToTry)
{
    if(RepetitionTestIsInBounds(Series))
    {
        PlatformConsoleOut("\n--- %s %s ---]\n",
                           Series->ColumnLabels[Series->ColumnIndex].Chars,
                           Series->RowLabels[Series->RowIndex].Chars);
    }

    RepetitionTestNewTestWave_(Tester, TargetProcessedByteCount, CPUTimerFreq, SecondsToTry);
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

internal repetition_test_results *
RepetitionTestGetTestResults(repetition_test_series *Series, u32 ColumnIndex, u32 RowIndex)
{
    repetition_test_results *Result = 0;

    if((ColumnIndex < Series->ColumnCount) && (RowIndex < Series->MaxRowCount))
    {
        Result = Series->TestResults + (RowIndex*Series->ColumnCount + ColumnIndex);
    }

    return Result;
}

internal b32
RepetitionTestIsTesting_(repetition_tester *Tester)
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
                        ComputeDerivedValues(&Results->Min, Tester->CPUTimerFreq);
                        RepetitionTestPrintValue("Min", Results->Min);
                        PlatformConsoleOut("                                   \r");
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

            ComputeDerivedValues(&Tester->Results.Total, Tester->CPUTimerFreq);
            ComputeDerivedValues(&Tester->Results.Min, Tester->CPUTimerFreq);
            ComputeDerivedValues(&Tester->Results.Max, Tester->CPUTimerFreq);

            PlatformConsoleOut("                                                          \r");
            RepetitionTestPrintResults(Tester->Results);
        }
    }

    b32 Result = (Tester->Mode == TestMode_Testing);
    return Result;
}

internal b32
RepetitionTestIsTesting(repetition_test_series *Series, repetition_tester *Tester)
{
    b32 Result = RepetitionTestIsTesting_(Tester);

    if(!Result)
    {
        if(RepetitionTestIsInBounds(Series))
        {
            *RepetitionTestGetTestResults(Series, Series->ColumnIndex, Series->RowIndex) = Tester->Results;

            if(++Series->ColumnIndex >= Series->ColumnCount)
            {
                Series->ColumnIndex = 0;
                ++Series->RowIndex;
            }

        }
    }

    return Result;
}

internal repetition_test_series
RepetitionTestAllocateTestSeries(memory_arena *Arena, u32 ColumnCount, u32 MaxRowCount)
{
    u64 TestResultsSize = (ColumnCount*MaxRowCount)*sizeof(repetition_test_results);
    u64 RowLabelsSize = (MaxRowCount)*sizeof(repetition_series_label);
    u64 ColumnLabelsSize = (ColumnCount)*sizeof(repetition_series_label);

    umm TotalSize = (TestResultsSize + RowLabelsSize + ColumnLabelsSize);
    string Buffer = String_(TotalSize, PushSize(Arena, TotalSize));

    repetition_test_series Result = 
    {
        .Buffer = Buffer,
        .MaxRowCount = MaxRowCount,
        .ColumnCount = ColumnCount,

        .TestResults = (repetition_test_results *)Result.Buffer.Data,
        .RowLabels = (repetition_series_label *)((u8 *)Result.TestResults + TestResultsSize),
        .ColumnLabels = (repetition_series_label *)((u8 *)Result.RowLabels + RowLabelsSize),
    };

    return Result;
}

internal b32
RepetitionTestIsInBounds(repetition_test_series *Series)
{
    b32 Result = ((Series->ColumnIndex < Series->ColumnCount) &&
                  (Series->RowIndex < Series->MaxRowCount));

    return Result;
}

internal void
RepetitionTestSetRowLabelLabel(repetition_test_series *Series, char *Format, ...)
{
    repetition_series_label *Label = &Series->RowLabelLabel;
    va_list Args;
    va_start(Args, Format);
    FormatStringToBuffer_(Label->Chars, sizeof(Label->Chars), Format, Args);
    va_end(Args);
}

internal void
RepetitionTestSetRowLabel(repetition_test_series *Series, char *Format, ...)
{
    if(RepetitionTestIsInBounds(Series))
    {
        repetition_series_label *Label = Series->RowLabels + Series->RowIndex;
        va_list Args;
        va_start(Args, Format);
        FormatStringToBuffer_(Label->Chars, sizeof(Label->Chars), Format, Args);
        va_end(Args);
    }
}

internal void
RepetitionTestSetColumnLabel(repetition_test_series *Series, char *Format, ...)
{
    if(RepetitionTestIsInBounds(Series))
    {
        repetition_series_label *Label = Series->ColumnLabels + Series->ColumnIndex;
        va_list Args;
        va_start(Args, Format);
        FormatStringToBuffer_(Label->Chars, sizeof(Label->Chars), Format, Args);
        va_end(Args);
    }
}

internal void
ReptitionTestPrintCSV(repetition_test_series *Series, repetition_value_type ValueType, f64 Coefficient)
{
    PlatformConsoleOut("%s", Series->RowLabelLabel.Chars);
    for(u32 ColumnIndex = 0; ColumnIndex < Series->ColumnCount; ++ColumnIndex)
    {
        PlatformConsoleOut(",%s", Series->ColumnLabels[ColumnIndex].Chars);
    }
    PlatformConsoleOut("\n");

    for(u32 RowIndex = 0; RowIndex < Series->RowIndex; ++RowIndex)
    {
        PlatformConsoleOut("%s", Series->RowLabels[RowIndex].Chars);
        for(u32 ColumnIndex = 0; ColumnIndex < Series->ColumnCount; ++ColumnIndex)
        {
            repetition_test_results *TestResults = RepetitionTestGetTestResults(Series, ColumnIndex, RowIndex);
            PlatformConsoleOut(",%f", Coefficient*TestResults->Min.PerCount[ValueType]);
        }
        PlatformConsoleOut("\n");
    }
}