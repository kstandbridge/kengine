#include "win32_kengine_tests.h"
#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"
#include "win32_kengine_shared.c"

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
        string A = String("before -18446744071562067968 after");
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
        ASSERT(Entry->Sum > LastSum);
        LastSum = Entry->Sum;
    }
    
    DebugClockEntryRadixSort(Count, Entries, TempEntries, Sort_Descending);
    LastSum = U32Max;
    for(u32 Index = 0;
        Index < Count;
        ++Index)
    {
        debug_clock_entry *Entry = Entries + Index;
        ASSERT(Entry->Sum < LastSum);
        LastSum = Entry->Sum;
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

typedef enum http_token_type
{
    HttpToken_OpenTag,
    HttpToken_CloseTag,
    
    HttpToken_Equals,
    
    HttpToken_String,
    
    HttpToken_EndOfStream,
} http_token_type;

typedef struct http_token
{
    http_token_type Type;
    string Text;
} http_token;

typedef struct http_tokenizer
{
    memory_arena *Arena;
    char *At;
} http_tokenizer;

inline void
HttpTokenizerEatallWhitespace(http_tokenizer *Tokenizer)
{
    for(;;)
    {
        // TODO(kstandbridge): Eat comments?
        if(IsWhitespace(Tokenizer->At[0]))
        {
            ++Tokenizer->At;
        }
        else
        {
            break;
        }
    }
}

internal http_token
GetNextHttpToken(http_tokenizer *Tokenizer)
{
    HttpTokenizerEatallWhitespace(Tokenizer);
    
    http_token Result;
    Result.Text = String_(1, (u8 *)Tokenizer->At);
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    switch(C)
    {
        case '\0': {Result.Type = HttpToken_EndOfStream; } break;
        case '=': { Result.Type = HttpToken_Equals; } break;
        case '<': 
        {
            if(Tokenizer->At[0] == '/')
            {
                Result.Type = HttpToken_CloseTag; 
                ++Tokenizer->At;
            }
            else
            {
                Result.Type = HttpToken_OpenTag; 
            }
            
            Result.Text.Data = (u8 *)Tokenizer->At;
            
            while(!IsWhitespace(Tokenizer->At[0]) &&
                  (Tokenizer->At[0] != '>') &&
                  (Tokenizer->At[0] != '/'))
            {
                ++Tokenizer->At;
            }
            
            Result.Text.Size = Tokenizer->At - (char *)Result.Text.Data;
            
            if(Tokenizer->At[0] == '>')
            {
                ++Tokenizer->At;
            }
            else if((Tokenizer->At[0] == '/') &&
                    (Tokenizer->At[1] == '>'))
            {
                Tokenizer->At += 2;
            }
        } break;
        case '/':
        {
            if(Tokenizer->At[0] == '>')
            {
                ++Tokenizer->At;
                Result.Type = HttpToken_CloseTag; 
            }
            else
            {
                // NOTE(kstandbridge): Something didn't parse correctly, we are expecting a nameless end tag
                // like this <meta name="robots" />
                __debugbreak();
            }
        } break;
        
        default:
        {
            --Tokenizer->At;
            Result.Type = HttpToken_String; 
            if(Tokenizer->At[0] == '"')
            {
                
                ++Tokenizer->At;
            }
            
            Result.Text.Data = (u8 *)Tokenizer->At;
            
            while(Tokenizer->At[0] &&
                  Tokenizer->At[0] != '"' &&
                  Tokenizer->At[0] != '=' &&
                  Tokenizer->At[0] != '<')
            {
                ++Tokenizer->At;
            }
            
            Result.Text.Size = Tokenizer->At - (char *)Result.Text.Data;
            
            if(Tokenizer->At[0] == '"')
            {
                ++Tokenizer->At;
            }
        } break;
    }
    
    return Result;
}

inline b32
RequireHttpToken(http_tokenizer *Tokenizer, http_token_type DesiredType)
{
    http_token Token = GetNextHttpToken(Tokenizer);
    b32 Result = (Token.Type == DesiredType);
    return Result;
}

internal void
ParseHttpLinks(http_tokenizer *Tokenizer)
{
    http_token Token;
    for(;;)
    {
        Token = GetNextHttpToken(Tokenizer);
        if(Token.Type == HttpToken_EndOfStream)
        {
            break;
        }
        if(Token.Type == HttpToken_OpenTag)
        {
            if(StringsAreEqual(Token.Text, String("a")))
            {
                break;
            }
        }
    }
    
    for(;;)
    {
        if(Token.Type == HttpToken_EndOfStream)
        {
            break;
        }
        Token = GetNextHttpToken(Tokenizer);
        if(Token.Type == HttpToken_String)
        {
            if(StringsAreEqual(Token.Text, String("href")))
            {
                if(RequireHttpToken(Tokenizer, HttpToken_Equals))
                {
                    Token = GetNextHttpToken(Tokenizer);
                    if(!StringsAreEqual(Token.Text, String("../")))
                    {
                        string FileName = Token.Text;
                        Token = GetNextHttpToken(Tokenizer);
                        // NOTE(kstandbridge): Should be file name again
                        Token = GetNextHttpToken(Tokenizer);
                        // NOTE(kstandbridge): Should be close tag A
                        Token = GetNextHttpToken(Tokenizer);
                        string DateAndSize = Token.Text;
                        int x = 5;
                        // TODO(kstandbridge): ParseDate and compare to get oldest?
                    }
                }
            }
        }
    }
}

internal void
RunParseHttpTests(memory_arena *Arena)
{
    string Response = DEBUGWin32ReadEntireFile(Arena, "response.txt");
    Assert(Response.Size > 0);
    
    http_tokenizer Tokenizer;
    Tokenizer.Arena = Arena;
    Tokenizer.At = (char *)Response.Data;
    
    // TODO(kstandbridge): Hack to skip over http header
    while(Tokenizer.At[0] &&
          Tokenizer.At[0] != '<')
    {
        ++Tokenizer.At;
    }
    ++Tokenizer.At;
    // NOTE(kstandbridge): Skip over DOCTYPE
    while(Tokenizer.At[0] &&
          Tokenizer.At[0] != '<')
    {
        ++Tokenizer.At;
    }
    
    b32 Parsing = true;
    while(Parsing)
    {
        http_token Token = GetNextHttpToken(&Tokenizer);
        switch(Token.Type)
        {
            case HttpToken_EndOfStream:
            {
                Parsing = false;
            } break;
            
            case HttpToken_OpenTag:
            {
                if(StringsAreEqual(Token.Text, String("html")))
                {
                    ParseHttpLinks(&Tokenizer);
                }
            } break;
        }
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
    
    RunParseHttpTests(Arena);
    
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