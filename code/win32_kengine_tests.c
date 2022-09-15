#include "win32_kengine_tests.h"
#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"
#include "win32_kengine_shared.c"
#include "kengine_sha512.c"

#include "kengine_random.c"

global s32 TotalTests;
global s32 FailedTests;

#define ASSERT(Expression)                 \
++TotalTests;                          \
if(!(Expression))                      \
{                                      \
++FailedTests;                     \
Win32ConsoleOut(Arena, "%s(%d): failed assert!\n", \
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
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before    42 after");
        string B = FormatString(Arena, "before %5u after", 42);
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
RunDoubleLinkedListInitTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    ASSERT(Sentinel.Next == &Sentinel);
    ASSERT(Sentinel.Prev == &Sentinel);
}

inline void
RunDoubleLinkedListInsertTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    {
        double_linked_list Element;
        DoubleLinkedListInsert(&Sentinel, &Element);
        
        ASSERT(Sentinel.Next != &Sentinel);
        ASSERT(Sentinel.Prev != &Sentinel);
        
        ASSERT(Sentinel.Next == &Element);
        ASSERT(Sentinel.Prev == &Element);
        
        ASSERT(Sentinel.Next->Next == &Sentinel);
        ASSERT(Sentinel.Prev->Prev == &Sentinel);
    }
    {
        double_linked_list First;
        double_linked_list Second;
        double_linked_list Third;
        DoubleLinkedListInsert(&Sentinel, &First);
        DoubleLinkedListInsert(&Sentinel, &Second);
        DoubleLinkedListInsert(&Sentinel, &Third);
        
        ASSERT(Sentinel.Next == &Third);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &First);
    }
}

inline void
RunDoubleLinkedListInsertAtLastTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    double_linked_list First;
    double_linked_list Second;
    double_linked_list Third;
    DoubleLinkedListInsertAtLast(&Sentinel, &First);
    DoubleLinkedListInsertAtLast(&Sentinel, &Second);
    DoubleLinkedListInsertAtLast(&Sentinel, &Third);
    
    ASSERT(Sentinel.Next == &First);
    ASSERT(Sentinel.Next->Next == &Second);
    ASSERT(Sentinel.Next->Next->Next == &Third);
}


inline void
RunDoubleLinkedListRemoveTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    double_linked_list First;
    double_linked_list Second;
    double_linked_list Third;
    DoubleLinkedListInsertAtLast(&Sentinel, &First);
    DoubleLinkedListInsertAtLast(&Sentinel, &Second);
    DoubleLinkedListInsertAtLast(&Sentinel, &Third);
    
    ASSERT(Sentinel.Next == &First);
    ASSERT(Sentinel.Next->Next == &Second);
    ASSERT(Sentinel.Next->Next->Next == &Third);
    
    DoubleLinkedListRemove(&Second);
    
    ASSERT(Sentinel.Next == &First);
    ASSERT(Sentinel.Next->Next == &Third);
    ASSERT(Sentinel.Next->Next->Next == &Sentinel);
}

inline void
RunDoubleLinkedListGetByIndexTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    double_linked_list First;
    double_linked_list Second;
    double_linked_list Third;
    DoubleLinkedListInsertAtLast(&Sentinel, &First);
    DoubleLinkedListInsertAtLast(&Sentinel, &Second);
    DoubleLinkedListInsertAtLast(&Sentinel, &Third);
    
    double_linked_list *Actual = DoubleLinkedListGetByIndex(&Sentinel, 2);
    ASSERT(Actual == &Third);
    Actual = DoubleLinkedListGetByIndex(&Sentinel, 0);
    ASSERT(Actual == &First);
    Actual = DoubleLinkedListGetByIndex(&Sentinel, 1);
    ASSERT(Actual == &Second);
}

inline void
RunDoubleLinkedListIsEmptyTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    Assert(DoubleLinkedListIsEmpty(&Sentinel));
    
    double_linked_list Element;
    DoubleLinkedListInsert(&Sentinel, &Element);
    Assert(!DoubleLinkedListIsEmpty(&Sentinel));
}

inline void
RunDoubleLinkedListSwapTests(memory_arena *Arena)
{
    {
        double_linked_list Sentinel;
        DoubleLinkedListInit(&Sentinel);
        
        double_linked_list First;
        double_linked_list Second;
        double_linked_list Third;
        DoubleLinkedListInsertAtLast(&Sentinel, &First);
        DoubleLinkedListInsertAtLast(&Sentinel, &Second);
        DoubleLinkedListInsertAtLast(&Sentinel, &Third);
        
        ASSERT(Sentinel.Next == &First);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &Third);
        
        DoubleLinkedListSwap(&First, &Third);
        
        ASSERT(Sentinel.Next == &Third);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &First);
    }
    {
        double_linked_list Sentinel;
        DoubleLinkedListInit(&Sentinel);
        
        double_linked_list First;
        double_linked_list Second;
        double_linked_list Third;
        DoubleLinkedListInsertAtLast(&Sentinel, &First);
        DoubleLinkedListInsertAtLast(&Sentinel, &Second);
        DoubleLinkedListInsertAtLast(&Sentinel, &Third);
        
        ASSERT(Sentinel.Next == &First);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &Third);
        
        DoubleLinkedListSwap(&Second, &Third);
        
        ASSERT(Sentinel.Next == &First);
        ASSERT(Sentinel.Next->Next == &Third);
        ASSERT(Sentinel.Next->Next->Next == &Second);
        
        DoubleLinkedListSwap(&First, &Third);
        
        ASSERT(Sentinel.Next == &Third);
        ASSERT(Sentinel.Next->Next == &First);
        ASSERT(Sentinel.Next->Next->Next == &Second);
    }
}

inline void
RunDoubleLinkedListSplitTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    double_linked_list First;
    First.SortKey = 1;
    double_linked_list Second;
    Second.SortKey = 2;
    double_linked_list Third;
    Third.SortKey = 3;
    double_linked_list Fourth;
    Fourth.SortKey = 4;
    DoubleLinkedListInsertAtLast(&Sentinel, &First);
    DoubleLinkedListInsertAtLast(&Sentinel, &Second);
    DoubleLinkedListInsertAtLast(&Sentinel, &Third);
    DoubleLinkedListInsertAtLast(&Sentinel, &Fourth);
    
    ASSERT(Sentinel.Next == &First);
    ASSERT(Sentinel.Next->Next == &Second);
    ASSERT(Sentinel.Next->Next->Next == &Third);
    ASSERT(Sentinel.Next->Next->Next->Next == &Fourth);
    
    
    double_linked_list *FirstHalf;
    double_linked_list *SecondHalf;
    DoubleLinkedListSplit(&Sentinel, &FirstHalf, &SecondHalf);
    
    ASSERT(FirstHalf == &First);
    ASSERT(FirstHalf->Next == &Second);
    ASSERT(FirstHalf->Next->Next == 0);
    
    ASSERT(SecondHalf == &Third);
    ASSERT(SecondHalf->Next == &Fourth);
    ASSERT(SecondHalf->Next->Next == 0);
    
}

internal b32
DoubleLinkedListPredicate(double_linked_list *A, double_linked_list *B)
{
    b32 Result = A->SortKey > B->SortKey;
    return Result;
}

inline void
RunDoubleLinkedListSortTests(memory_arena *Arena)
{
    {
        double_linked_list Sentinel;
        Sentinel.SortKey = U32Max;
        DoubleLinkedListInit(&Sentinel);
        
        double_linked_list First;
        First.SortKey = 10;
        double_linked_list Second;
        Second.SortKey = 5;
        double_linked_list Third;
        Third.SortKey = 15;
        DoubleLinkedListInsertAtLast(&Sentinel, &First);
        DoubleLinkedListInsertAtLast(&Sentinel, &Second);
        DoubleLinkedListInsertAtLast(&Sentinel, &Third);
        
        ASSERT(Sentinel.Next == &First);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &Third);
        
        DoubleLinkedListMergeSort(&Sentinel, DoubleLinkedListPredicate);
        
        ASSERT(Sentinel.Next == &Third);
        ASSERT(Sentinel.Next->Next == &First);
        ASSERT(Sentinel.Next->Next->Next == &Second);
    }
}

inline void
RunFreelistTests(memory_arena *Arena)
{
    {
        double_linked_list_free_list FreeList;
        DoubleLinkedListFreeListInit(&FreeList, Arena);
        
        double_linked_list *First = DoubleLinkedListFreeListAllocate(&FreeList);
        ASSERT(FreeList.Sentinel.Next == First);
        double_linked_list *Second = DoubleLinkedListFreeListAllocate(&FreeList);
        ASSERT(FreeList.Sentinel.Next == Second);
        ASSERT(FreeList.Sentinel.Next->Next == First);
        double_linked_list *Third = DoubleLinkedListFreeListAllocate(&FreeList);
        ASSERT(FreeList.Sentinel.Next == Third);
        ASSERT(FreeList.Sentinel.Next->Next == Second);
        ASSERT(FreeList.Sentinel.Next->Next->Next == First);
        
        DoubleLinkedListFreeListDeallocate(&FreeList, Second);
        ASSERT(FreeList.Sentinel.Next == Third);
        ASSERT(FreeList.Sentinel.Next->Next == First);
        ASSERT(FreeList.Sentinel.Next->Next->Next == &FreeList.Sentinel);
        ASSERT(FreeList.FreeSentinel.Next == Second);
        double_linked_list *Fourth = DoubleLinkedListFreeListAllocate(&FreeList);
        ASSERT(Fourth == Second);
        ASSERT(FreeList.FreeSentinel.Next == &FreeList.FreeSentinel);
    }
}

inline void
RunRadixSortTests(memory_arena *Arena)
{
    LARGE_INTEGER LastCounter;
    Win32QueryPerformanceCounter(&LastCounter);
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
        ASSERT(First == 42);
        ASSERT(Second == 24);
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
        ASSERT(Day == 27);
        ASSERT(StringsAreEqual(Month, String("Nov")));
        ASSERT(Year == 2020);
        ASSERT(Hour == 17);
        ASSERT(Minute == 52);
    }
}

internal string
DEBUGWin32ReadEntireFile(memory_arena *Arena, char *FilePath)
{
    string Result;
    ZeroStruct(Result);
    
    HANDLE FileHandle = Win32CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        b32 ReadResult = Win32GetFileSizeEx(FileHandle, &FileSize);
        Assert(ReadResult);
        if(ReadResult)
        {    
            Result.Size = FileSize.QuadPart;
            Result.Data = PushSize(Arena, Result.Size);
            Assert(Result.Data);
            
            if(Result.Data)
            {
                u32 BytesRead;
                ReadResult = Win32ReadFile(FileHandle, Result.Data, (u32)Result.Size, (LPDWORD)&BytesRead, 0);
                Assert(ReadResult);
                Assert(BytesRead == Result.Size);
            }
        }
        
        Win32CloseHandle(FileHandle);
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
    
    RunV2Tests(Arena);
    
    RunUpperCamelCaseTests(Arena);
    
    RunDoubleLinkedListInitTests(Arena);
    RunDoubleLinkedListInsertTests(Arena);
    RunDoubleLinkedListInsertAtLastTests(Arena);
    RunDoubleLinkedListRemoveTests(Arena);
    RunDoubleLinkedListGetByIndexTests(Arena);
    RunDoubleLinkedListIsEmptyTests(Arena);
    RunDoubleLinkedListSwapTests(Arena);
    RunDoubleLinkedListSplitTests(Arena);
    RunDoubleLinkedListSortTests(Arena);
    
    RunFreelistTests(Arena);
    
    RunRadixSortTests(Arena);
    
    RunParseFromStringTests(Arena);
    
    RunSha512Tests(Arena);
    
    b32 Result = (FailedTests == 0);
    return Result;
}

s32 __stdcall
mainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
#if KENGINE_INTERNAL
    void *BaseAddress = (void *)Terabytes(2);
#else
    void *BaseAddress = 0;
#endif
    
    u64 MemoryBlockSize = Megabytes(16);
    void *MemoryBlock = Win32VirtualAlloc(BaseAddress, MemoryBlockSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(MemoryBlock);
    
    memory_arena Arena_;
    memory_arena *Arena = &Arena_;
    InitializeArena(Arena, MemoryBlockSize, MemoryBlock);
    
    b32 Result = RunAllTests(Arena);
    Win32ConsoleOut(Arena, "Unit Tests %s: %d/%d passed.\n", Result ? "Successful" : "Failed", TotalTests - FailedTests, TotalTests);
    
    Win32ExitProcess(0);
    
    InvalidCodePath;
    
    return 0;
}