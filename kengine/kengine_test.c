#ifndef KENGINE_TEST_H

#define AssertTrue(Expression) \
    ++GlobalAppMemory->TotalTests; \
    if(!(Expression)) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): expression assert fail.\n", __FILE__, __LINE__); \
    }

#define AssertFalse(Expression) \
    ++GlobalAppMemory->TotalTests; \
    if((Expression)) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): expression assert fail.\n", __FILE__, __LINE__); \
    }

#define AssertEqualString(Expected, Actual) \
    ++GlobalAppMemory->TotalTests; \
    if(!StringsAreEqual(Expected, Actual)) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%S'\n\t\t\tActual:      '%S'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertNotEqualString(Expected, Actual) \
    ++GlobalAppMemory->TotalTests; \
    if(StringsAreEqual(Expected, Actual)) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%S'\n\t\t\tActual:      '%S'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualU32(Expected, Actual) \
    ++GlobalAppMemory->TotalTests; \
    if(Expected != Actual) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%u'\n\t\t\tActual:      '%u'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualBits(Expected, Actual) \
    ++GlobalAppMemory->TotalTests; \
    if(Expected != Actual) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%b'\n\t\t\tActual:      '%b'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualBits16(Expected, Actual) \
    ++GlobalAppMemory->TotalTests; \
    if(Expected != Actual) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%16b'\n\t\t\tActual:      '%16b'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualHex(Expected, Actual) \
    ++GlobalAppMemory->TotalTests; \
    if(Expected != Actual) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%X'\n\t\t\tActual:      '%X'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualS32(Expected, Actual) \
    ++GlobalAppMemory->TotalTests; \
    if(Expected != Actual) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%d'\n\t\t\tActual:      '%d'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

#define AssertEqualF32(Expected, Actual)\
    { \
        ++GlobalAppMemory->TotalTests; \
        f32 Difference = Expected - Actual; \
        if(!((Difference < 0.00001f) && (-Difference < 0.00001f))) \
        { \
            ++GlobalAppMemory->FailedTests; \
            PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%f'\n\t\t\tActual:      '%f'\n", \
                                __FILE__, __LINE__, Expected, Actual);        \
        } \
    }

#define AssertEqualU64(Expected, Actual) \
    ++GlobalAppMemory->TotalTests; \
    if(Expected != Actual) \
    { \
        ++GlobalAppMemory->FailedTests; \
        PlatformConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%lu'\n\t\t\tActual:      '%lu'\n", \
                           __FILE__, __LINE__, Expected, Actual);        \
    }

typedef struct app_memory
{
    memory_arena Arena;
    
    s32 TotalTests;
    s32 FailedTests;

    u32 ArgCount;
    char **Args;

} app_memory;
global app_memory GlobalAppMemory_;
global app_memory *GlobalAppMemory = &GlobalAppMemory_;


void
RunAllTests(memory_arena *Arena);

s32
main(int ArgCount, char *Args[])
{
    s32 Result = 0;

    GlobalAppMemory->ArgCount = ArgCount - 1;
    GlobalAppMemory->Args = Args + 1;

    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;
    
    RunAllTests(&GlobalAppMemory->Arena);
    
    Result = (GlobalAppMemory->FailedTests != 0);
    
    PlatformConsoleOut("Unit Tests %s: %d/%d passed.\n", 
                       Result ? "Failed" : "Successful", GlobalAppMemory->TotalTests - GlobalAppMemory->FailedTests,                        GlobalAppMemory->TotalTests);
    
    return Result;
}

#define KENGINE_TEST_H
#endif // KENGINE_TEST_H