#include <Windows.h>

internal void
Win32ConsoleOut_(char *Format, va_list ArgList)
{
    u8 Buffer[4096] = {0};
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    AppendFormatString_(&StringState, Format, ArgList);
    
    string Text = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    DWORD NumberOfBytesWritten;
    WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&NumberOfBytesWritten, 0);
    Assert(NumberOfBytesWritten == Text.Size);

    
#if KENGINE_INTERNAL
    Buffer[Text.Size] = '\0';
    OutputDebugStringA((char *)Buffer);
#endif
}

void
Win32ConsoleOut(char *Format, ...)
{
    va_list ArgList;
    va_start(ArgList, Format);
    
    Win32ConsoleOut_(Format, ArgList);
    
    va_end(ArgList);
    
}

typedef struct win32_memory_block
{
    platform_memory_block PlatformBlock;
    struct win32_memory_block *Prev;
    struct win32_memory_block *Next;
    u64 Padding;
} win32_memory_block;

typedef struct win32_state
{
    ticket_mutex MemoryMutex;
    win32_memory_block MemorySentinel;

} win32_state;

global win32_state GlobalWin32State;

platform_memory_block *
Win32AllocateMemory(umm Size, u64 Flags)
{
    // NOTE(kstandbridge): Cache align
    Assert(sizeof(win32_memory_block) == 64);
    
    // NOTE(kstandbridge): Raymond Chen on page sizes: https://devblogs.microsoft.com/oldnewthing/20210510-00/?p=105200
    // TODO(kstandbridge): GetSystemInfo > dwPageSize
    umm PageSize = Kilobytes(4);
    umm TotalSize = Size + sizeof(win32_memory_block);
    umm BaseOffset = sizeof(win32_memory_block);
    umm ProtectOffset = 0;
    if(Flags & PlatformMemoryBlockFlag_UnderflowCheck)
    {
        TotalSize = Size + 2*PageSize;
        BaseOffset = 2*PageSize;
        ProtectOffset = PageSize;
    }
    else if(Flags & PlatformMemoryBlockFlag_OverflowCheck)
    {
        umm SizeRoundedUp = AlignPow2(Size, PageSize);
        TotalSize = SizeRoundedUp + 2*PageSize;
        BaseOffset = PageSize + SizeRoundedUp - Size;
        ProtectOffset = PageSize + SizeRoundedUp;
    }
    
    LogDebug("Allocating %u bytes", TotalSize);
    
    win32_memory_block *Win32Block = (win32_memory_block *)VirtualAlloc(0, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Win32Block);
    Win32Block->PlatformBlock.Base = (u8 *)Win32Block + BaseOffset;
    Assert(Win32Block->PlatformBlock.Used == 0);
    Assert(Win32Block->PlatformBlock.Prev == 0);
    
    if(Flags & (PlatformMemoryBlockFlag_UnderflowCheck|PlatformMemoryBlockFlag_OverflowCheck))
    {
        DWORD OldProtect = 0;
        b32 IsProtected = VirtualProtect((u8 *)Win32Block + ProtectOffset, PageSize, PAGE_NOACCESS, &OldProtect);
        if(!IsProtected)
        {
            InvalidCodePath;
        }
    }
    
    win32_memory_block *Sentinel = &GlobalWin32State.MemorySentinel;
    Win32Block->Next = Sentinel;
    Win32Block->PlatformBlock.Size = Size;
    Win32Block->PlatformBlock.Flags = Flags;
    
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Win32Block->Prev = Sentinel->Prev;
    Win32Block->Prev->Next = Win32Block;
    Win32Block->Next->Prev = Win32Block;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
    
    platform_memory_block *Result = &Win32Block->PlatformBlock;
    return Result;
}

void
Win32DeallocateMemory(platform_memory_block *Block)
{
    win32_memory_block *Win32Block = (win32_memory_block *)Block;
    
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Win32Block->Prev->Next = Win32Block->Next;
    Win32Block->Next->Prev = Win32Block->Prev;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
    
    LogDebug("Freeing %u bytes", Block->Size);
    
    b32 IsFreed = VirtualFree(Win32Block, 0, MEM_RELEASE);
    if(!IsFreed)
    {
        InvalidCodePath;
    }
}