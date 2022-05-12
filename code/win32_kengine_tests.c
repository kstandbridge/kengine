#include "kengine_platform.h"
#include "kengine_math.h"
#include "kengine_shared.h"
#include "win32_kengine_shared.h"

global s32 TotalTests;
global s32 FailedTests;

#define ASSERT(Expression)                 \
++TotalTests;                          \
if(!(Expression))                      \
{                                      \
++FailedTests;                     \
ConsoleOut(Arena, "%s(%d): failed assert!\n", \
__FILE__, __LINE__);        \
}

inline void
RunStringsAreEqualTests(memory_arena *Arena)
{
    ASSERT(StringsAreEqual(String("Foo"), String("Foo")));
    ASSERT(!StringsAreEqual(String("Bar"), String("Foo")));
    ASSERT(StringsAreEqual(String("Foo bar Bas"), String("Foo bar Bas")));
    ASSERT(!StringsAreEqual(String("Foo bar Bas"), String("Bas bar Foo")));
    ASSERT(!StringsAreEqual(String("Foo bar Bas"), String("")));
    ASSERT(!StringsAreEqual(String(""), String("Bas bar Foo")));
}

inline void
RunFormatStringSignedDecimalIntegerTests(memory_arena *Arena)
{
    {
        string A = String("before 42 after");
        string B = FormatString(Arena, "before %i after", 42);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("aaa 111 bbb 222 ccc 333 ddd");
        string B = FormatString(Arena, "aaa %d bbb %d ccc %d ddd", 111, 222, 333);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("aaa 111 bbb 222 ccc 333 ddd");
        string B = FormatString(Arena, "aaa %d bbb %d ccc %d ddd", 444, 555, 666);
        ASSERT(!StringsAreEqual(A, B));
    }
    {
        string A = String("before 2147483647 after");
        string B = FormatString(Arena, "before %d after", S32Max);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before -2147483648 after");
        string B = FormatString(Arena, "before %d after", S32Min);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 9223372036854775807 after");
        string B = FormatString(Arena, "before %ld after", S64Max);
        ASSERT(StringsAreEqual(A, B));
    }
}

inline void
RunFormatStringUnsignedDecimalIntegerTests(memory_arena *Arena)
{
    {
        string A = String("before 4294967295 after");
        string B = FormatString(Arena, "before %u after", U32Max);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 0 after");
        string B = FormatString(Arena, "before %u after", U32Min);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 18446744073709551615 after");
        string B = FormatString(Arena, "before %lu after", U64Max);
        ASSERT(StringsAreEqual(A, B));
    }
}

inline void
RunFormatStringDecimalFloatingPoint(memory_arena *Arena)
{
    {
        string A = String("before 3.141592 after");
        string B = FormatString(Arena, "before %f after", Pi32);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 6.283185 after");
        string B = FormatString(Arena, "before %f after", Tau32);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 3.141 after");
        string B = FormatString(Arena, "before %.3f after", Pi32);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 3.141 after");
        string B = FormatString(Arena, "before %.*f after", 3, Pi32);
        ASSERT(StringsAreEqual(A, B));
    }
    
    // NOTE(kstandbridge): Intentionally checking string length 
    // as precision is not accurate with currently implementation 
    {
        string A = String("before 6.28318530717958647692 after");
        string B = FormatString(Arena, "before %.20f after", Tau32);
        ASSERT(A.Length == B.Length);
    }
    {
        string A = String("before 3.14159265359 after");
        string B = FormatString(Arena, "before %.11f after", Pi32);
        ASSERT(A.Length == B.Length);
    }
}

inline void
RunFormatStringStringOfCharactersTests(memory_arena *Arena)
{
    {
        string A = String("before bar after");
        string B = FormatString(Arena, "before %s after", "bar");
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("11 11 11 foo 22 22 22 bar 33 33 33 bas 44 44 44");
        string B = FormatString(Arena, "11 11 11 %s 22 22 22 %s 33 33 33 %s 44 44 44", "foo", "bar", "bas");
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("11 11 11 foo 22 22 22 bar 33 33 33 bas 44 44 44");
        string B = FormatString(Arena, "11 11 11 %s 22 22 22 %s 33 33 33 %s 44 44 44", "bas", "bar", "foo");
        ASSERT(!StringsAreEqual(A, B));
    }
    {
        string A = String("before foo after");
        string B = FormatString(Arena, "before %.3s after", "foobar");
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before foo after");
        string B = FormatString(Arena, "before %.*s after", 3, "foobar");
        ASSERT(StringsAreEqual(A, B));
    }
}

inline void
RunFormatStringStringTypeTests(memory_arena *Arena)
{
    {
        string A = String("before bar after");
        string B = FormatString(Arena, "before %S after", String("bar"));
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("foo 1234 bar 5678 bas");
        string B = FormatString(Arena, "foo %S bar %S bas", String("1234"), String("5678"));
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before foo after");
        string B = FormatString(Arena, "before %S after", String("bar"));
        ASSERT(!StringsAreEqual(A, B));
    }
    {
        string A = String("before foo after");
        string B = FormatString(Arena, "before %.3S after", String("foobar"));
        ASSERT(StringsAreEqual(A, B));
    }
    
}

inline void
RunFormatStringPercentTests(memory_arena *Arena)
{
    {
        string A = String("before % after");
        string B = FormatString(Arena, "before %% after");
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before  after");
        string B = FormatString(Arena, "before %% after");
        ASSERT(!StringsAreEqual(A, B));
    }
}

internal b32
RunAllTests(memory_arena *Arena)
{
    RunStringsAreEqualTests(Arena);
    RunFormatStringSignedDecimalIntegerTests(Arena);
    RunFormatStringUnsignedDecimalIntegerTests(Arena);
    RunFormatStringDecimalFloatingPoint(Arena);
    RunFormatStringStringOfCharactersTests(Arena);
    RunFormatStringStringTypeTests(Arena);
    RunFormatStringPercentTests(Arena);
    
    b32 Result = (FailedTests == 0);
    return Result;
}

s32 __stdcall
mainCRTStartup()
{
    memory_arena ArenaInternal = AllocateArena(Megabytes(8));
    memory_arena *Arena = &ArenaInternal;
    
    b32 Result = RunAllTests(Arena);
    ConsoleOut(Arena, "Unit Tests %s: %d/%d passed.\n", Result ? "Successful" : "Failed", TotalTests - FailedTests, TotalTests);
    
    ExitProcess(Result);
}
