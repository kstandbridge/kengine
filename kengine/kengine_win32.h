#ifndef KENGINE_WIN32_H

#define PlatformConsoleOut(Format, ...) Win32ConsoleOut(Format, __VA_ARGS__)
internal void
Win32ConsoleOut(char *Format, ...);

#define PlatformConsoleOut_(Format, ArgsList) Win32ConsoleOut_(Format, ArgsList)
internal void
Win32ConsoleOut_(char *Format, va_list ArgsList);

#define PlatformAllocateMemory(Size, Flags) Win32AllocateMemory(Size, Flags)
platform_memory_block *
Win32AllocateMemory(umm Size, u64 Flags);

#define PlatformDeallocateMemory(Block) Win32DeallocateMemory(Block)
void
Win32DeallocateMemory(platform_memory_block *Block);

#define KENGINE_WIN32_H
#endif // KENGINE_WIN32_H