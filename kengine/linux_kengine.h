#ifndef LINUX_KENGINE_H

#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

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

#define PlatformOpenFile(FilePath, Flags) LinuxOpenFile(FilePath, Flags)
platform_file
LinuxOpenFile(string FilePath, platform_file_access_flags Flags);

#define PlatformWriteFile(File, Text) LinuxWriteFile(File, Text)
void
LinuxWriteFile(platform_file *File, string Text);

#define PlatformCloseFile(File) LinuxCloseFile(File)
void
LinuxCloseFile(platform_file *File);

#define PlatformGetOSTimerFrequency() LinuxGetOSTimerFrequency()
u64
LinuxGetOSTimerFrequency();

#define PlatformReadOSTimer() LinuxReadOSTimer()
u64
LinuxReadOSTimer();

#define PlatformReadOSPageFaultCount() LinuxReadOSPageFaultCount()
u64
LinuxReadOSPageFaultCount();

#define PlatformReadCPUTimer() __rdtsc()

#define LINUX_KENGINE_H
#endif // LINUX_KENGINE_H