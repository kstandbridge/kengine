#include "kengine_platform.h"
#include "kengine_memory.h"
#include "kengine_generated.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "kengine_debug_shared.h"
#include "kengine_debug.h"
#include "kengine_debug_generated.h"
#include "win32_kengine_types.h"
#include "kengine_string.h"

#ifndef VERSION
#define VERSION 0
#endif // VERSION

#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"
#include "kengine_telemetry.c"
#include "win32_kengine_shared.c"
#include "kengine_sha512.c"
#include "kengine_eddsa.c"
#include "kengine_random.c"

#include "kengine_tokenizer.h"
#include "kengine_xml_parser.h"

global s32 TotalTests;
global s32 FailedTests;

#define ASSERT(Expression)                 \
++TotalTests;                          \
if(!(Expression))                      \
{                                      \
++FailedTests;                     \
Win32ConsoleOut("%s(%d): failed assert!\n", \
__FILE__, __LINE__);        \
}

#define AssertEqualString(Expected, Actual) \
++TotalTests; \
if(!StringsAreEqual(Expected, Actual)) \
{ \
++FailedTests; \
Win32ConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%S'\n\t\t\tActual:      '%S'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

#define AssertEqualU32(Expected, Actual) \
++TotalTests; \
if(Expected != Actual) \
{ \
++FailedTests; \
Win32ConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%u'\n\t\t\tActual:      '%u'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

#define AssertEqualS32(Expected, Actual) \
++TotalTests; \
if(Expected != Actual) \
{ \
++FailedTests; \
Win32ConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%d'\n\t\t\tActual:      '%d'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

#define AssertEqualU64(Expected, Actual) \
++TotalTests; \
if(Expected != Actual) \
{ \
++FailedTests; \
Win32ConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%lu'\n\t\t\tActual:      '%lu'\n", \
__FILE__, __LINE__, Expected, Actual);        \
}

inline void
RunStringsAreEqualTests()
{
    ASSERT(StringsAreEqual(String("Foo"), String("Foo")));
    ASSERT(!StringsAreEqual(String("Bar"), String("Foo")));
    ASSERT(StringsAreEqual(String("Foo bar Bas"), String("Foo bar Bas")));
    ASSERT(!StringsAreEqual(String("Foo bar Bas"), String("Bas bar Foo")));
    ASSERT(!StringsAreEqual(String("Foo bar Bas"), String("")));
    ASSERT(!StringsAreEqual(String(""), String("Bas bar Foo")));
}

inline void
RunStringBeginsWithTests()
{
    string HayStack = String("Content-Length: 256");
    ASSERT(StringBeginsWith(String("Content-Length"), HayStack));
    ASSERT(!StringBeginsWith(String("256"), HayStack));
}

inline void
RunStringContainsTests()
{
    ASSERT(StringContains(String("foo"), String("Before foo after")));
    ASSERT(!StringContains(String("foo"), String("Before bar after")));
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
        AssertEqualString(A, B);
    }
    
    {
        string A = String("before    42 after");
        string B = FormatString(Arena, "before %5u after", 42);
        AssertEqualString(A, B);
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
        string A = String("before   3.141 after");
        string B = FormatString(Arena, "before %3.3f after", Pi32);
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
        ASSERT(A.Size == B.Size);
    }
    {
        string A = String("before 3.14159265359 after");
        string B = FormatString(Arena, "before %.11f after", Pi32);
        ASSERT(A.Size == B.Size);
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

inline void
RunFormatStringDateTests(memory_arena *Arena)
{
    {
        date_time Date;
        Date.Year = 2020;
        Date.Month = 9;
        Date.Day = 29;
        Date.Hour = 7;
        Date.Minute = 45;
        Date.Second = 13;
        Date.Milliseconds = 0;
        string A = String("2020-09-29 07:45:13:000");
        string B = FormatString(Arena,
                                "%04d-%02d-%02d %02d:%02d:%02d:%03d",
                                Date.Year, Date.Month, Date.Day, Date.Hour, Date.Minute, Date.Second, Date.Milliseconds);
        AssertEqualString(A, B)
    }
    {
        date_time Date;
        Date.Year = 2020;
        Date.Month = 9;
        Date.Day = 29;
        Date.Hour = 7;
        Date.Minute = 45;
        Date.Second = 00;
        Date.Milliseconds = 123;
        string A = String("2020-09-29 07:45:00:123");
        string B = FormatString(Arena,
                                "%04d-%02d-%02d %02d:%02d:%02d:%03d",
                                Date.Year, Date.Month, Date.Day, Date.Hour, Date.Minute, Date.Second, Date.Milliseconds);
        AssertEqualString(A, B)
    }
    {
        date_time Date;
        Date.Year = 2020;
        Date.Month = 9;
        Date.Day = 29;
        Date.Hour = 7;
        Date.Minute = 45;
        Date.Second = 7;
        Date.Milliseconds = 4;
        string A = String("2020-09-29 07:45:07:004");
        string B = FormatString(Arena,
                                "%04d-%02d-%02d %02d:%02d:%02d:%03d",
                                Date.Year, Date.Month, Date.Day, Date.Hour, Date.Minute, Date.Second, Date.Milliseconds);
        AssertEqualString(A, B)
    }
    {
        date_time Date;
        Date.Year = 2020;
        Date.Month = 9;
        Date.Day = 29;
        Date.Hour = 7;
        Date.Minute = 45;
        Date.Second = 42;
        Date.Milliseconds = 42;
        string A = String("2020-09-29 07:45:42:042");
        string B = FormatString(Arena,
                                "%04d-%02d-%02d %02d:%02d:%02d:%03d",
                                Date.Year, Date.Month, Date.Day, Date.Hour, Date.Minute, Date.Second, Date.Milliseconds);
        AssertEqualString(A, B)
    }
}

inline void
RunFormatStringWithoutArenaTests()
{
    {
        u8 Buffer[256];
        umm BufferSize = sizeof(Buffer);
        AssertEqualU32(256, BufferSize);
        
        format_string_state State = BeginFormatString();
        AppendFormatString(&State, "before %d after", 42);
        string Actual = EndFormatStringToBuffer(&State, Buffer, BufferSize);
        
        AssertEqualString(String("before 42 after"), Actual);
    }
    {
        u8 Buffer[256];
        umm BufferSize = sizeof(Buffer);
        
        string Actual = FormatStringToBuffer(Buffer, BufferSize, "before %d after", 42);
        AssertEqualString(String("before 42 after"), Actual);
    }
}

inline void
RunV2Tests()
{
    {
        v2 A = V2(2.3f, 4.5f);
        ASSERT(A.X == 2.3f);
        ASSERT(A.Y == 4.5f);
    }
    {
        v2 A = V2Set1(3.14f);
        ASSERT(A.X == 3.14f);
        ASSERT(A.Y == 3.14f);
    }
    {
        v2 C = V2Add(V2Set1(3.0f), V2Set1(2.0f));
        ASSERT(C.X == 5.0f);
        ASSERT(C.Y == 5.0f);
    }
    {
        v2 C = V2Subtract(V2Set1(3.0f), V2Set1(2.0f));
        ASSERT(C.X == 1.0f);
        ASSERT(C.Y == 1.0f);
    }
    {
        v2 C = V2Multiply(V2Set1(3.0f), V2Set1(2.0f));
        ASSERT(C.X == 6.0f);
        ASSERT(C.Y == 6.0f);
    }
    {
        v2 C = V2Divide(V2Set1(3.0f), V2Set1(2.0f));
        ASSERT(C.X == 1.5f);
        ASSERT(C.Y == 1.5f);
    }
}

inline void
RunUpperCamelCaseTests(memory_arena *Arena)
{
    {
        string Expected = String("UpperCamelCase");
        string Actual = FormatString(Arena, "upper_camel_case");
        ToUpperCamelCase(&Actual);
        ASSERT(StringsAreEqual(Expected, Actual));
    }
}

inline void
RunRadixSortTests(memory_arena *Arena)
{
    LARGE_INTEGER LastCounter;
    QueryPerformanceCounter(&LastCounter);
    random_state RandomState;
    RandomState.Value = (u32)LastCounter.QuadPart;
    
    u32 Count = RandomU32(&RandomState) % 1000;
    debug_clock_entry *Entries = PushArray(Arena, Count, debug_clock_entry);
    for(u32 Index = 0;
        Index < Count;
        ++Index)
    {
        debug_clock_entry *Entry = Entries + Index;
        Entry->Sum = RandomU32(&RandomState);
    }
    
    debug_clock_entry *TempEntries = PushArray(Arena, Count, debug_clock_entry);
    DebugClockEntryRadixSort(Count, Entries, TempEntries, Sort_Ascending);
    
    u32 LastSum = U32Min;
    for(u32 Index = 0;
        Index < Count;
        ++Index)
    {
        debug_clock_entry *Entry = Entries + Index;
        ASSERT(Entry->Sum >= LastSum);
        LastSum = (u32)Entry->Sum;
    }
    
    DebugClockEntryRadixSort(Count, Entries, TempEntries, Sort_Descending);
    LastSum = U32Max;
    for(u32 Index = 0;
        Index < Count;
        ++Index)
    {
        debug_clock_entry *Entry = Entries + Index;
        ASSERT(Entry->Sum <= LastSum);
        LastSum = (u32)Entry->Sum;
    }
}

inline void
RunParseFromStringTests()
{
    {
        s32 First = 0;
        s32 Second = 0;
        ParseFromString(String("42-24"), "%d-%d", &First, &Second);
        AssertEqualU32(42, First);
        AssertEqualU32(24, Second);
    }
    {
        u64 First = 0;
        s32 Second = 0;
        ParseFromString(String("8527741705-24"), "%ld-%d", &First, &Second);
        AssertEqualU64(8527741705, First);
        AssertEqualU32(24, Second);
    }
    {
        string Input = String("27-Nov-2020 17:52  2.13 MB");
        char *Format = "%d-%S-%d %d:%d";
        s32 Day = 0;
        string Month;
        Month.Size = 0;
        Month.Data = 0;
        s32 Year = 0;
        s32 Hour = 0;
        s32 Minute = 0;
        ParseFromString(Input, Format,
                        &Day, &Month, &Year, &Hour, &Minute);
        AssertEqualU32(27, Day);
        AssertEqualString(String("Nov"), Month);
        AssertEqualU32(2020, Year);
        AssertEqualU32(17, Hour);
        AssertEqualU32(52, Minute);
    }
    
    {
        string Hostname = String("");
        u32 Port = 0;
        string Endpoint = String("");
        ParseFromString(String("http://www.whatever.org:6231/foo/bar/bas/{ignmore}.html"),
                        "http://%S:%u%S{ignore}.html", &Hostname, &Port, &Endpoint);
        AssertEqualString(String("www.whatever.org"), Hostname);
        AssertEqualU32(6231, Port);
        AssertEqualString(String("/foo/bar/bas/"), Endpoint);
    }
}

internal string
DEBUGWin32ReadEntireFile(memory_arena *Arena, char *FilePath)
{
    string Result;
    ZeroStruct(Result);
    
    HANDLE FileHandle = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        b32 ReadResult = GetFileSizeEx(FileHandle, &FileSize);
        Assert(ReadResult);
        if(ReadResult)
        {    
            Result.Size = FileSize.QuadPart;
            Result.Data = PushSize(Arena, Result.Size);
            Assert(Result.Data);
            
            if(Result.Data)
            {
                u32 BytesRead;
                ReadResult = ReadFile(FileHandle, Result.Data, (u32)Result.Size, (LPDWORD)&BytesRead, 0);
                Assert(ReadResult);
                Assert(BytesRead == Result.Size);
            }
        }
        
        CloseHandle(FileHandle);
    }
    
    return Result;
}

inline void
RunSha512Tests()
{
    u8 Seed[32];
    for(u8 Index = 0;
        Index < sizeof(Seed);
        ++Index)
    {
        Seed[Index] = Index;
    }
    
    u8 Output[32];
    ZeroSize(sizeof(Output), Output);
    Sha512(Seed, sizeof(Seed), Output);
    
    ASSERT(Output[0] == 0x3d);
    ASSERT(Output[1] == 0x94);
    ASSERT(Output[2] == 0xee);
    ASSERT(Output[3] == 0xa4);
    ASSERT(Output[4] == 0x9c);
    ASSERT(Output[5] == 0x58);
    ASSERT(Output[6] == 0x0a);
    ASSERT(Output[7] == 0xef);
    ASSERT(Output[8] == 0x81);
    ASSERT(Output[9] == 0x69);
    ASSERT(Output[10] == 0x35);
    ASSERT(Output[11] == 0x76);
    ASSERT(Output[12] == 0x2b);
    ASSERT(Output[13] == 0xe0);
    ASSERT(Output[14] == 0x49);
    ASSERT(Output[15] == 0x55);
    ASSERT(Output[16] == 0x9d);
    ASSERT(Output[17] == 0x6d);
    ASSERT(Output[18] == 0x14);
    ASSERT(Output[19] == 0x40);
    ASSERT(Output[20] == 0xde);
    ASSERT(Output[21] == 0xde);
    ASSERT(Output[22] == 0x12);
    ASSERT(Output[23] == 0xe6);
    ASSERT(Output[24] == 0xa1);
    ASSERT(Output[25] == 0x25);
    ASSERT(Output[26] == 0xf1);
    ASSERT(Output[27] == 0x84);
    ASSERT(Output[28] == 0x1f);
    ASSERT(Output[29] == 0xff);
    ASSERT(Output[30] == 0x8e);
    ASSERT(Output[31] == 0x6f);
}

inline void
RunEdDSATests()
{
    LARGE_INTEGER LastCounter;
    QueryPerformanceCounter(&LastCounter);
    random_state RandomState;
    RandomState.Value = (u32)LastCounter.QuadPart;
    
    ed25519_private_key PersistentKey;
    
    // NOTE(kstandbridge): Generate a random private key for the test
    {    
        for(u32 Index = 0;
            Index < 64;
            ++Index)
        {
            PersistentKey[Index] = RandomU32(&RandomState) % 256;
        }
    }
    
    ed25519_key_pair KeyPair = Ed25519CreateKeyPair(PersistentKey);
    
    string Message = String("some super secret message");
    ed25519_signature Signature;
    Ed25519Sign(Signature, Message, KeyPair);
    
    b32 SignVerify = Ed25519Verify(Signature, Message, KeyPair.Public);
    ASSERT(SignVerify);
    
    
    SignVerify = Ed25519Verify(Signature, String("wrong secret message"), KeyPair.Public);
    ASSERT(!SignVerify);
}

typedef enum node_predicate_type
{
    NodePredicate_ByValue,
    NodePredicate_MatchValue,
    NodePredicate_MatchText
} node_predicate_type;

internal b32
NodePredicate(node_predicate_type *Context, node *A, node *B)
{
    b32 Result = false;
    
    switch(*Context)
    {
        case NodePredicate_ByValue:
        {
            Result = (A->Value < B->Value);
        } break;
        
        case NodePredicate_MatchValue:
        {
            Result = (A->Value == B->Value);
        } break;
        
        case NodePredicate_MatchText:
        {
            s32 SortIndex = StringComparisonLowercase(A->Text, B->Text);
            if(SortIndex > 0)
            {
                Result = false;
            }
            else
            {
                Result = true;
            }
        } break;
        
        InvalidDefaultCase;
    }
    return Result;
}

inline void
RunLinkedListMergeSortTests(memory_arena *Arena)
{
    node *Head = 0;
    
    node *FortyOne = PushbackNode(&Head, Arena);
    FortyOne->Value = 41;
    FortyOne->Text = String("ddaa");
    
    node *Five = PushbackNode(&Head, Arena);
    Five->Value = 5;
    Five->Text = String("ee");
    
    node *Seven = PushbackNode(&Head, Arena);
    Seven->Value = 7;
    Seven->Text = String("gg");
    
    node *TwentyTwo = PushbackNode(&Head, Arena);
    TwentyTwo->Value = 22;
    TwentyTwo->Text = String("bbbb");
    
    node *TwentyEight = PushbackNode(&Head, Arena);
    TwentyEight->Value = 28;
    TwentyEight->Text = String("bbhh");
    
    node *SixtyThree = PushbackNode(&Head, Arena);
    SixtyThree->Value = 63;
    SixtyThree->Text = String("ffcc");
    
    node *Four = PushbackNode(&Head, Arena);
    Four->Value = 4;
    Four->Text = String("dd");
    
    node *Eight = PushbackNode(&Head, Arena);
    Eight->Value = 8;
    Eight->Text = String("hh");
    
    node *Two = PushbackNode(&Head, Arena);
    Two->Value = 2;
    Two->Text = String("bb");
    
    node *Eleven = PushbackNode(&Head, Arena);
    Eleven->Value = 11;
    Eleven->Text = String("aa");
    
    ASSERT(Head == FortyOne);
    ASSERT(Head->Next == Five);
    ASSERT(Head->Next->Next == Seven);
    ASSERT(Head->Next->Next->Next == TwentyTwo);
    ASSERT(Head->Next->Next->Next->Next == TwentyEight);
    ASSERT(Head->Next->Next->Next->Next->Next == SixtyThree);
    ASSERT(Head->Next->Next->Next->Next->Next->Next == Four);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next == Eight);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next == Two);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next == Eleven);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next->Next == 0);
    
    node_predicate_type PredicateType = NodePredicate_ByValue;
    
    NodeMergeSort(&Head, NodePredicate, &PredicateType, Sort_Ascending);
    
    ASSERT(Head == Two);
    ASSERT(Head->Next == Four);
    ASSERT(Head->Next->Next == Five);
    ASSERT(Head->Next->Next->Next == Seven);
    ASSERT(Head->Next->Next->Next->Next == Eight);
    ASSERT(Head->Next->Next->Next->Next->Next == Eleven);
    ASSERT(Head->Next->Next->Next->Next->Next->Next == TwentyTwo);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next == TwentyEight);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next == FortyOne);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next == SixtyThree);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next->Next == 0);
    
    NodeMergeSort(&Head, NodePredicate, &PredicateType, Sort_Descending);
    
    ASSERT(Head == SixtyThree);
    ASSERT(Head->Next == FortyOne);
    ASSERT(Head->Next->Next == TwentyEight);
    ASSERT(Head->Next->Next->Next == TwentyTwo);
    ASSERT(Head->Next->Next->Next->Next == Eleven);
    ASSERT(Head->Next->Next->Next->Next->Next == Eight);
    ASSERT(Head->Next->Next->Next->Next->Next->Next == Seven);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next == Five);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next == Four);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next == Two);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next->Next == 0);
    
    PredicateType = NodePredicate_MatchText;
    
    NodeMergeSort(&Head, NodePredicate, &PredicateType, Sort_Ascending);
    
    ASSERT(Head == Eleven);
    ASSERT(Head->Next == TwentyTwo);
    ASSERT(Head->Next->Next == TwentyEight);
    ASSERT(Head->Next->Next->Next == Two);
    ASSERT(Head->Next->Next->Next->Next == FortyOne);
    ASSERT(Head->Next->Next->Next->Next->Next == Four);
    ASSERT(Head->Next->Next->Next->Next->Next->Next == Five);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next == SixtyThree);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next == Seven);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next == Eight);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next->Next == 0);
    
    NodeMergeSort(&Head, NodePredicate, &PredicateType, Sort_Descending);
    
    ASSERT(Head == Eight);
    ASSERT(Head->Next == Seven);
    ASSERT(Head->Next->Next == SixtyThree);
    ASSERT(Head->Next->Next->Next == Five);
    ASSERT(Head->Next->Next->Next->Next == Four);
    ASSERT(Head->Next->Next->Next->Next->Next == FortyOne);
    ASSERT(Head->Next->Next->Next->Next->Next->Next == Two);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next == TwentyEight);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next == TwentyTwo);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next == Eleven);
    ASSERT(Head->Next->Next->Next->Next->Next->Next->Next->Next->Next->Next == 0);
    
}

inline void
RunNodeGetCountTests(memory_arena *Arena)
{
    node *Head = 0;
    
    node *FortyOne = PushbackNode(&Head, Arena);
    FortyOne->Value = 41;
    node *Five = PushbackNode(&Head, Arena);
    Five->Value = 5;
    node *Seven = PushbackNode(&Head, Arena);
    Seven->Value = 7;
    
    ASSERT(Head == FortyOne);
    ASSERT(Head->Next == Five);
    ASSERT(Head->Next->Next == Seven);
    
    u32 Actual = GetNodeCount(Head);
    AssertEqualU32(3, Actual);
}

inline void
RunGetNodeByIndexTests(memory_arena *Arena)
{
    node *Head = 0;
    
    node *FortyOne = PushbackNode(&Head, Arena);
    FortyOne->Value = 41;
    node *Five = PushbackNode(&Head, Arena);
    Five->Value = 5;
    node *Seven = PushbackNode(&Head, Arena);
    Seven->Value = 7;
    
    ASSERT(Head == FortyOne);
    ASSERT(Head->Next == Five);
    ASSERT(Head->Next->Next == Seven);
    
    node *Actual = GetNodeByIndex(Head, 0);
    ASSERT(Actual == Head);
    Actual = GetNodeByIndex(Head, 1);
    ASSERT(Actual == Five);
    Actual = GetNodeByIndex(Head, 2);
    ASSERT(Actual == Seven);
    Actual = GetNodeByIndex(Head, 3);
    ASSERT(Actual == 0);
}

inline void
RunGetIndexOfNodeTests(memory_arena *Arena)
{
    node *Head = 0;
    
    node *FortyOne = PushbackNode(&Head, Arena);
    FortyOne->Value = 41;
    node *Five = PushbackNode(&Head, Arena);
    Five->Value = 5;
    node *Seven = PushbackNode(&Head, Arena);
    Seven->Value = 7;
    node *DummyNode = PushStruct(Arena, node);
    
    ASSERT(Head == FortyOne);
    ASSERT(Head->Next == Five);
    ASSERT(Head->Next->Next == Seven);
    
    s32 Actual = GetIndexOfNode(Head, FortyOne);
    AssertEqualS32(Actual, 0);
    Actual = GetIndexOfNode(Head, Five);
    AssertEqualS32(Actual, 1);
    Actual = GetIndexOfNode(Head, Seven);
    AssertEqualS32(Actual, 2);
    Actual = GetIndexOfNode(Head, DummyNode);
    AssertEqualS32(Actual, -1);
    
}

inline void
RunGetNodeTests(memory_arena *Arena)
{
    node *Head = 0;
    
    node *FortyOne = PushbackNode(&Head, Arena);
    FortyOne->Value = 41;
    node *Five = PushbackNode(&Head, Arena);
    Five->Value = 5;
    node *Seven = PushbackNode(&Head, Arena);
    Seven->Value = 7;
    node *DummyNode = PushStruct(Arena, node);
    ASSERT(Head == FortyOne);
    ASSERT(Head->Next == Five);
    ASSERT(Head->Next->Next == Seven);
    
    node MatchNode = 
    {
        .Value = 41
    };
    
    node_predicate_type PredicateType = NodePredicate_MatchValue;
    node *Actual = GetNode(Head, NodePredicate, &PredicateType, &MatchNode);
    ASSERT(Actual == FortyOne);
    
    MatchNode.Value = 7;
    Actual = GetNode(Head, NodePredicate, &PredicateType, &MatchNode);
    ASSERT(Actual == Seven);
    
    Actual = GetNode(Head, NodePredicate, &PredicateType, DummyNode);
    ASSERT(Actual == 0);
}

inline void
RunParseHtmlTest(memory_arena *Arena)
{
    string HtmlData = String("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
                             "<html>\n"
                             "\t<head>\n"
                             "\t\t<title>The title goes here</title>\n"
                             "\t</head>\n"
                             "\t<body>\n"
                             "\t\t<h1>The header goes here</h1>\n"
                             "\t\t<ul>\n"
                             "\t\t\t<li><a href=\"/Parent/\"> Parent Directory</a></li>\n"
                             "\t\t\t<li><a href=\"foo.img\"> foo image</a></li>\n"
                             "\t\t\t<li><a href=\"bar.txt\"> bar text</a></li>\n"
                             "\t\t\t<li><a href=\"bas.wav\"> bas soundl</a></li>\n"
                             "\t\t</ul>\n"
                             "\t</body>\n"
                             "</html>");
    
    xml_element DocElement = ParseXmlDocument(Arena, HtmlData, String("DummyFile.html"));
    xml_element *HtmlElement = GetXmlElement(&DocElement, String("html"));
    ASSERT(HtmlElement);
    xml_element *BodyElement = GetXmlElement(HtmlElement, String("body"));
    ASSERT(BodyElement);
    
    xml_element *UlElement = GetXmlElement(HtmlElement, String("ul"));
    ASSERT(UlElement);
    
    xml_element *LiElements = GetXmlElements(UlElement, String("li"));
    ASSERT(LiElements);
    
    xml_element *LiElement = LiElements;
    xml_element *AElement = GetXmlElement(LiElement, String("a"));
    ASSERT(AElement);
    xml_attribute *HrefAttribute = GetXmlAttribute(AElement, String("href"));
    ASSERT(HrefAttribute);
    AssertEqualString(String("/Parent/"), HrefAttribute->Value);
    
    LiElement = LiElement->Next;
    AElement = GetXmlElement(LiElement, String("a"));
    ASSERT(AElement);
    HrefAttribute = GetXmlAttribute(AElement, String("href"));
    ASSERT(HrefAttribute);
    AssertEqualString(String("foo.img"), HrefAttribute->Value);
    
    LiElement = LiElement->Next;
    AElement = GetXmlElement(LiElement, String("a"));
    ASSERT(AElement);
    HrefAttribute = GetXmlAttribute(AElement, String("href"));
    ASSERT(HrefAttribute);
    AssertEqualString(String("bar.txt"), HrefAttribute->Value);
    
    LiElement = LiElement->Next;
    AElement = GetXmlElement(LiElement, String("a"));
    ASSERT(AElement);
    HrefAttribute = GetXmlAttribute(AElement, String("href"));
    ASSERT(HrefAttribute);
    AssertEqualString(String("bas.wav"), HrefAttribute->Value);
}

inline void
RunParseXmlTest(memory_arena *Arena)
{
    {
        string FileName = String("/dev/null");
        string FileData = String("<Config>\n"
                                 "\t<Source>SomeSource</Source>\n"
                                 "\t<!--\n"
                                 "\t\tThis is a comment that should be entirely ignored\n"
                                 "\t-->\n"
                                 "\t<Name>SomeName</Name>\n"
                                 "</Config>");
        xml_element DocElement = ParseXmlDocument(Arena, FileData, FileName);
        xml_element *ConfigElement = GetXmlElement(&DocElement, String("Config"));
        ASSERT(ConfigElement);
        
        xml_element *SourceElement = GetXmlElement(ConfigElement, String("Source"));
        ASSERT(SourceElement);
        string SourceElementValue = GetXmlElementValue(SourceElement);
        AssertEqualString(String("SomeSource"), SourceElementValue);
        
        xml_element *NameElement = GetXmlElement(ConfigElement, String("Name"));
        ASSERT(NameElement);
        string NameElementValue = GetXmlElementValue(NameElement);
        AssertEqualString(String("SomeName"), NameElementValue);
    }
    
    {
        string FileName = String("/dev/null");
        string FileData = String("<foo>"
                                 "bar"
                                 "</foo>");
        xml_element DocElement = ParseXmlDocument(Arena, FileData, FileName);
        xml_element *FooElement = GetXmlElement(&DocElement, String("foo"));
        ASSERT(FooElement);
        string Value = GetXmlElementValue(FooElement);
        AssertEqualString(String("bar"), Value);
    }
    
    {
        string FileName = String("/dev/null");
        string FileData = String("<foo abc=\"123\" cba=\"321\">"
                                 "bar"
                                 "</foo>");
        xml_element DocElement = ParseXmlDocument(Arena, FileData, FileName);
        xml_element *FooElement = GetXmlElement(&DocElement, String("foo"));
        ASSERT(FooElement);
        string Value = GetXmlElementValue(FooElement);
        AssertEqualString(String("bar"), Value);
        xml_attribute *CbaAttribute = GetXmlAttribute(FooElement, String("cba"));
        AssertEqualString(String("321"), CbaAttribute->Value);
        xml_attribute *AbcAttribute = GetXmlAttribute(FooElement, String("abc"));
        AssertEqualString(String("123"), AbcAttribute->Value);
    }
    
    
    {
        string FileName = String("/dev/null");
        string FileData = String("<foo>"
                                 "\t<bar A=\"1\" />"
                                 "\t<bar A=\"2\" />"
                                 "\t<bar A=\"3\" />"
                                 "</foo>");
        xml_element DocElement = ParseXmlDocument(Arena, FileData, FileName);
        xml_element *FooElement = GetXmlElement(&DocElement, String("foo"));
        ASSERT(FooElement);
        
        xml_element *BarElement = GetXmlElements(FooElement, String("bar"));
        ASSERT(BarElement);
        xml_attribute *AAttribute = GetXmlAttribute(BarElement, String("A"));
        ASSERT(AAttribute);
        AssertEqualString(String("1"), AAttribute->Value);
        
        BarElement = BarElement->Next;
        ASSERT(BarElement);
        AAttribute = GetXmlAttribute(BarElement, String("A"));
        ASSERT(AAttribute);
        AssertEqualString(String("2"), AAttribute->Value);
        
        BarElement = BarElement->Next;
        ASSERT(BarElement);
        AAttribute = GetXmlAttribute(BarElement, String("A"));
        ASSERT(AAttribute);
        AssertEqualString(String("3"), AAttribute->Value);
    }
    
    {
        string FileName = String("/dev/null");
        string FileData = String("<foo>"
                                 "<bar>1</bar>"
                                 "<bar>2</bar>"
                                 "<bar>3</bar>"
                                 "</foo>");
        xml_element DocElement = ParseXmlDocument(Arena, FileData, FileName);
        xml_element *FooElement = GetXmlElement(&DocElement, String("foo"));
        ASSERT(FooElement);
        
        xml_element *BarElement = GetXmlElements(FooElement, String("bar"));
        ASSERT(BarElement);
        string Value = GetXmlElementValue(BarElement);
        AssertEqualString(String("1"), Value);
        
        BarElement = BarElement->Next;
        ASSERT(BarElement);
        Value = GetXmlElementValue(BarElement);
        AssertEqualString(String("2"), Value);
        
        BarElement = BarElement->Next;
        ASSERT(BarElement);
        Value = GetXmlElementValue(BarElement);
        AssertEqualString(String("3"), Value);
    }
}

internal b32
RunAllTests(memory_arena *Arena)
{
    RunStringsAreEqualTests();
    RunStringBeginsWithTests();
    RunStringContainsTests();
    RunFormatStringSignedDecimalIntegerTests(Arena);
    RunFormatStringUnsignedDecimalIntegerTests(Arena);
    RunFormatStringDecimalFloatingPoint(Arena);
    RunFormatStringStringOfCharactersTests(Arena);
    RunFormatStringStringTypeTests(Arena);
    RunFormatStringPercentTests(Arena);
    RunFormatStringDateTests(Arena);
    RunFormatStringWithoutArenaTests();
    
    RunV2Tests();
    
    RunUpperCamelCaseTests(Arena);
    
    RunRadixSortTests(Arena);
    
    RunParseFromStringTests();
    
    RunSha512Tests();
    
    RunEdDSATests();
    
    RunLinkedListMergeSortTests(Arena);
    RunNodeGetCountTests(Arena);
    RunGetNodeByIndexTests(Arena);
    RunGetIndexOfNodeTests(Arena);
    RunGetNodeTests(Arena);
    
    RunParseHtmlTest(Arena);
    
    RunParseXmlTest(Arena);
    
    b32 Result = (FailedTests == 0);
    return Result;
}

s32 __stdcall
main()
{
    Platform.AllocateMemory = Win32AllocateMemory;
    Platform.DeallocateMemory = Win32DeallocateMemory;
    
    Platform.GetSystemTimestamp = Win32GetSystemTimestamp;
    Platform.GetDateTimeFromTimestamp = Win32GetDateTimeFromTimestamp;
    Platform.ConsoleOut = Win32ConsoleOut;
    
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
    
    memory_arena Arena;
    ZeroStruct(Arena);
    
    b32 Result = RunAllTests(&Arena);
    Win32ConsoleOut("Unit Tests %s: %d/%d passed.\n", Result ? "Successful" : "Failed", TotalTests - FailedTests, TotalTests);
    
    return 0;
}