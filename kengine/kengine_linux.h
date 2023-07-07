#ifndef KENGINE_LINUX_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

//extern long write(int, const char *, unsigned long);
//extern int open(const char *path, int oflag, ... );

#define PlatformConsoleOut(Format, ...) LinuxConsoleOut(Format, ##__VA_ARGS__)
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

#define PlatformFileExists(Path) LinuxFileExists(Path)
b32
LinuxFileExists(string Path);

#define PlatformReadEntireFile(Arena, FilePath) LinuxReadEntireFile(Arena, FilePath)
string
LinuxReadEntireFile(memory_arena *Arena, string FilePath);

#define PlatformWriteTextToFile(Text, FilePath) LinuxWriteTextToFile(Text, FilePath)
b32
LinuxWriteTextToFile(string Text, string FilePath);

#define KENGINE_LINUX_H
#endif // KENGINE_LINUX_H