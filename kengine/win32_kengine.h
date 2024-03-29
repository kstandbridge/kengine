#ifndef WIN32_KENGINE_H

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

#define PlatformFileExists(Path) Win32FileExists(Path)
b32
Win32FileExists(string Path);

#define PlatformReadEntireFile(Arena, FilePath) Win32ReadEntireFile(Arena, FilePath)
string
Win32ReadEntireFile(memory_arena *Arena, string FilePath);

#define PlatformWriteTextToFile(Text, FilePath) Win32WriteTextToFile(Text, FilePath)
b32
Win32WriteTextToFile(string Text, string FilePath);

#define PlatformOpenFile(FilePath, Flags) Win32OpenFile(FilePath, Flags)
platform_file
Win32OpenFile(string FilePath, platform_file_access_flags Flags);

#define PlatformWriteFile(File, Text) Win32WriteFile(File, Text)
void
Win32WriteFile(platform_file *File, string Text);

#define PlatformCloseFile(File) Win32CloseFile(File)
void
Win32CloseFile(platform_file *File);

#define PlatformGetOSTimerFrequency() Win32GetOSTimerFrequency()
u64
Win32GetOSTimerFrequency();

#define PlatformReadOSTimer() Win32ReadOSTimer()
u64
Win32ReadOSTimer();

#define PlatformReadOSPageFaultCount() Win32ReadOSPageFaultCount()
u64
Win32ReadOSPageFaultCount();

#define PlatformReadCPUTimer() __rdtsc()

#define WIN32_KENGINE_H
#endif // WIN32_KENGINE_H