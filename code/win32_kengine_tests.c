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

#define AssertEqualU64(Expected, Actual) \
++TotalTests; \
if(Expected != Actual) \
{ \
++FailedTests; \
Win32ConsoleOut("%s(%d): string assert fail.\n\t\t\tExpected:    '%lu'\n\t\t\tActual:      '%lu'\n", \
__FILE__, __LINE__, Expected, Actual);        \
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
RunStringBeginsWithTests(memory_arena *Arena)
{
    string HayStack = String("Content-Length: 256");
    ASSERT(StringBeginsWith(String("Content-Length"), HayStack));
    ASSERT(!StringBeginsWith(String("256"), HayStack));
}

inline void
RunStringContainsTests(memory_arena *Arena)
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
RunFormatStringWithoutArenaTests(memory_arena *Arena)
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
RunV2Tests(memory_arena *Arena)
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
        LastSum = Entry->Sum;
    }
    
    DebugClockEntryRadixSort(Count, Entries, TempEntries, Sort_Descending);
    LastSum = U32Max;
    for(u32 Index = 0;
        Index < Count;
        ++Index)
    {
        debug_clock_entry *Entry = Entries + Index;
        ASSERT(Entry->Sum <= LastSum);
        LastSum = Entry->Sum;
    }
}

inline void
RunParseFromStringTests(memory_arena *Arena)
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
RunSha512Tests(memory_arena *Arena)
{
    u8 Seed[32];
    for(u32 Index = 0;
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
RunEdDSATests(memory_arena *Arena)
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

internal b32
NodePredicate(node *A, node *B)
{
    b32 Result = (A->Value < B->Value);
    return Result;
}

inline void
RunLinkedListMergeSortTests(memory_arena *Arena)
{
    node *Head = 0;
    
    node *FortyOne = NodePushBack(&Head, Arena);
    FortyOne->Value = 41;
    node *Five = NodePushBack(&Head, Arena);
    Five->Value = 5;
    node *Seven = NodePushBack(&Head, Arena);
    Seven->Value = 7;
    node *TwentyTwo = NodePushBack(&Head, Arena);
    TwentyTwo->Value = 22;
    node *TwentyEight = NodePushBack(&Head, Arena);
    TwentyEight->Value = 28;
    node *SixtyThree = NodePushBack(&Head, Arena);
    SixtyThree->Value = 63;
    node *Four = NodePushBack(&Head, Arena);
    Four->Value = 4;
    node *Eight = NodePushBack(&Head, Arena);
    Eight->Value = 8;
    node *Two = NodePushBack(&Head, Arena);
    Two->Value = 2;
    node *Eleven = NodePushBack(&Head, Arena);
    Eleven->Value = 11;
    
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
    
    NodeMergeSort(&Head, NodePredicate, Sort_Ascending);
    
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
    
    NodeMergeSort(&Head, NodePredicate, Sort_Descending);
    
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
}

typedef struct build_categorization
{
    string Key;
    string Value;
    
    struct build_categorization *Next;
} build_categorization;


typedef struct launch_option
{
    string Name;
    string CmdTemplate;
    
    struct launch_option *Next;
} launch_option;

typedef struct build
{
    string Wbn;
    
    build_categorization *BuildCategorizations;
    launch_option *LaunchOptions;
} build;

internal void
RunTokenizerBuildTests(memory_arena *Arena)
{
    string FileName = String("D:\\GalaQ\\assets\\build_meta.xml");
    string FileData = Win32ReadEntireFile(Arena, FileName);
    
    LogDebug("\n%S", FileData);
    
    build Build = {0};
    
    xml_element DocElement = ParseXmlDocument(Arena, FileData, FileName);
    xml_element BuildElement = GetXmlElement(DocElement, String("build"));
    xml_attribute *WbnAttribute = GetXmlAttribute(&BuildElement, String("wbn"));
    Build.Wbn = WbnAttribute->Value;
    
    // NOTE(kstandbridge): Parse launch options
    {    
        launch_option *CurrentLaunchOption = 0;
        xml_element LaunchOptionsElement = GetXmlElement(BuildElement, String("launch_options"));
        xml_element *LaunchOptionElements = GetXmlElements(LaunchOptionsElement, String("launch_option"));
        for(xml_element *LaunchOptionElement = LaunchOptionElements;
            LaunchOptionElement;
            LaunchOptionElement = LaunchOptionElement->Next)
        {
            launch_option *LaunchOption = CurrentLaunchOption;
            if(LaunchOption == 0)
            {
                LaunchOption = CurrentLaunchOption = Build.LaunchOptions = PushStruct(Arena, launch_option);
            }
            else
            {
                CurrentLaunchOption->Next = PushStruct(Arena, launch_option);
                LaunchOption = CurrentLaunchOption = CurrentLaunchOption->Next;
            }
            xml_attribute *NameAttribute = GetXmlAttribute(LaunchOptionElement, String("name"));
            xml_attribute *CmdTemplateAttribute = GetXmlAttribute(LaunchOptionElement, String("cmd_template"));
            LaunchOption->Name = NameAttribute->Value;
            LaunchOption->CmdTemplate = CmdTemplateAttribute->Value;
        }
    }
    
    // NOTE(kstandbridge): Parse build categories
    {
        build_categorization *CurrentBuildCategory = 0;
        xml_element BuildCategorizationElement = GetXmlElement(BuildElement, String("build_categorization"));
        xml_element *EntryElements = GetXmlElements(BuildCategorizationElement, String("entry"));
        for(xml_element *EntryElement = EntryElements;
            EntryElement;
            EntryElement = EntryElement->Next)
        {
            build_categorization *BuildCategory = CurrentBuildCategory;
            if(BuildCategory == 0)
            {
                BuildCategory = CurrentBuildCategory = Build.BuildCategorizations = PushStruct(Arena, build_categorization);
            }
            else
            {
                CurrentBuildCategory->Next = PushStruct(Arena, build_categorization);
                BuildCategory = CurrentBuildCategory = CurrentBuildCategory->Next;
            }
            xml_attribute *KeyAttribute = GetXmlAttribute(EntryElement, String("key"));
            xml_attribute *ValueAttribute = GetXmlAttribute(EntryElement, String("value"));
            BuildCategory->Key = KeyAttribute->Value;
            BuildCategory->Value = ValueAttribute->Value;
        }
    }
    
    
#if 0    
    // TODO(kstandbridge): <some_tag>foo</some_tag>
    xml_element EmployeesElement = GetXmlElement(BuildElement, String("employees"));
    AssertEqualString(String("jeff"), EmployeesElement.Value);
#endif
    
    AssertEqualString(String("joel_parkey"), Build.Wbn);
    
    build_categorization *BuildCategory = Build.BuildCategorizations;
    ASSERT(BuildCategory);
    AssertEqualString(String("friendly_name"), BuildCategory->Key);
    AssertEqualString(String("SpaceBlockShmup"), BuildCategory->Value);
    ASSERT(BuildCategory->Next == 0);
    
    launch_option *LaunchOption = Build.LaunchOptions;
    ASSERT(LaunchOption);
    AssertEqualString(String("2022-09-08_1516"), LaunchOption->Name);
    AssertEqualString(String("2022-09-08_1516\\index.html"), LaunchOption->CmdTemplate);
    LaunchOption = LaunchOption->Next;
    ASSERT(LaunchOption);
    AssertEqualString(String("2022-08-31_1431"), LaunchOption->Name);
    AssertEqualString(String("2022-08-31_1431\\index.html"), LaunchOption->CmdTemplate);
    LaunchOption = LaunchOption->Next;
    ASSERT(LaunchOption);
    AssertEqualString(String("2022-08-30_1407"), LaunchOption->Name);
    AssertEqualString(String("2022-08-30_1407\\index.html"), LaunchOption->CmdTemplate);
    ASSERT(LaunchOption->Next == 0);
}

typedef struct world_categorization
{
    string Key;
    string Value;
    
    struct world_categorization *Next;
} world_categorization;

typedef struct world
{
    string Name;
    
    world_categorization *Categories;
    world_categorization *CurrentCategory;
    
} world;

internal void
RunTokenizerWorldTests(memory_arena *Arena)
{
    string FileName = String("D:\\GalaQ\\assets\\world_meta.xml");
    string FileData = Win32ReadEntireFile(Arena, FileName);
    
    LogDebug("\n%S", FileData);
    
    world World = {0};
    tokenizer Tokenizer_ = Tokenize(FileData, FileName);
    tokenizer *Tokenizer = &Tokenizer_;
    
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_OpenAngleBracket)
        {
            if(PeekTokenType(Tokenizer, Token_ForwardSlash))
            {
                RequireToken(Tokenizer, Token_ForwardSlash);
                RequireIdentifierToken(Tokenizer, String("world"));
                RequireToken(Tokenizer, Token_CloseAngleBracket);
                break;
            }
            else
            {            
                RequireIdentifierToken(Tokenizer, String("world"));
                while(Parsing(Tokenizer) &&
                      !PeekTokenType(Tokenizer, Token_CloseAngleBracket))
                {
                    token Key = RequireToken(Tokenizer, Token_Identifier);
                    RequireToken(Tokenizer, Token_Equals);
                    token Value = RequireToken(Tokenizer, Token_String);
                    if(StringsAreEqual(Key.Text, String("name")))
                    {
                        World.Name = Value.Text;
                    }
                    else
                    {
                        LogDebug("Ignoring attribute %S=\"%S\"", Key.Text, Value.Text);
                    }
                }
                
                RequireToken(Tokenizer, Token_CloseAngleBracket);
                
                // NOTE(kstandbridge): Parse world categories
                {            
                    RequireToken(Tokenizer, Token_OpenAngleBracket);
                    RequireIdentifierToken(Tokenizer, String("world_categorization"));
                    RequireToken(Tokenizer, Token_CloseAngleBracket);
                    while(Parsing(Tokenizer))
                    {
                        RequireToken(Tokenizer, Token_OpenAngleBracket);
                        if(PeekTokenType(Tokenizer, Token_ForwardSlash))
                        {
                            RequireToken(Tokenizer, Token_ForwardSlash);
                            RequireIdentifierToken(Tokenizer, String("world_categorization"));
                            RequireToken(Tokenizer, Token_CloseAngleBracket);
                            break;
                        }
                        else
                        {
                            // NOTE(kstandbridge): Parse entry
                            {                        
                                RequireIdentifierToken(Tokenizer, String("entry"));
                                while(Parsing(Tokenizer))
                                {
                                    if(PeekTokenType(Tokenizer, Token_ForwardSlash))
                                    {
                                        RequireToken(Tokenizer, Token_ForwardSlash);
                                        RequireToken(Tokenizer, Token_CloseAngleBracket);
                                        break;
                                    }
                                    else
                                    {
                                        RequireToken(Tokenizer, Token_Identifier);
                                        RequireToken(Tokenizer, Token_Equals);
                                        token Key = RequireToken(Tokenizer, Token_String);
                                        
                                        RequireToken(Tokenizer, Token_Identifier);
                                        RequireToken(Tokenizer, Token_Equals);
                                        token Value = RequireToken(Tokenizer, Token_String);
                                        
                                        world_categorization *Category = World.CurrentCategory;
                                        if(Category == 0)
                                        {
                                            Category = World.CurrentCategory = World.Categories = PushStruct(Arena, world_categorization);
                                        }
                                        else
                                        {
                                            World.CurrentCategory->Next = PushStruct(Arena, world_categorization);
                                            Category = World.CurrentCategory = World.CurrentCategory->Next;
                                        }
                                        Category->Key = Key.Text;
                                        Category->Value = Value.Text;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    AssertEqualString(String("ugs2019lem"), World.Name);
    world_categorization *Category = World.Categories;
    AssertEqualString(String("tags"), Category->Key);
    AssertEqualString(String("ugs"), Category->Value);
    Category = Category->Next;
    AssertEqualString(String("deploy_time"), Category->Key);
    AssertEqualString(String("2019-09-04T00:00:00Z"), Category->Value);
    Category = Category->Next;
    AssertEqualString(String("comment"), Category->Key);
    AssertEqualString(String("Ubisoft Gaming School 2019 LEM"), Category->Value);
    ASSERT(Category->Next == 0);
    
}

internal b32
RunAllTests(memory_arena *Arena)
{
    RunStringsAreEqualTests(Arena);
    RunStringBeginsWithTests(Arena);
    RunStringContainsTests(Arena);
    RunFormatStringSignedDecimalIntegerTests(Arena);
    RunFormatStringUnsignedDecimalIntegerTests(Arena);
    RunFormatStringDecimalFloatingPoint(Arena);
    RunFormatStringStringOfCharactersTests(Arena);
    RunFormatStringStringTypeTests(Arena);
    RunFormatStringPercentTests(Arena);
    RunFormatStringDateTests(Arena);
    RunFormatStringWithoutArenaTests(Arena);
    
    RunV2Tests(Arena);
    
    RunUpperCamelCaseTests(Arena);
    
    RunRadixSortTests(Arena);
    
    RunParseFromStringTests(Arena);
    
    RunSha512Tests(Arena);
    
    RunEdDSATests(Arena);
    
    RunLinkedListMergeSortTests(Arena);
    
    //RunTokenizerWorldTests(Arena);
    RunTokenizerBuildTests(Arena);
    
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