#ifndef KENGINE_LINUX_H

#include <sys/mman.h>
#include <unistd.h>

//extern long write(int, const char *, unsigned long);

#define PlatformConsoleOut(Format, ...) LinuxConsoleOut(Format, __VA_ARGS__)
internal void
LinuxConsoleOut(char *Format, ...);

#define PlatformConsoleOut_(Format, ArgsList) LinuxConsoleOut_(Format, ArgsList)
internal void
LinuxConsoleOut_(char *Format, va_list ArgsList);


#define PlatformAllocateMemory(Size, Flags) LinuxAllocateMemory(Size, Flags)
platform_memory_block *
LinuxAllocateMemory(umm Size, u64 Flags);

#define PlatformDeallocateMemory(Block) LinuxDeallocateMemory(Block)
void
LinuxDeallocateMemory(platform_memory_block *Block);


#define KENGINE_LINUX_H
#endif // KENGINE_LINUX_H