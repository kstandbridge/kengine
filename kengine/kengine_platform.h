#ifndef KENGINE_PLATFORM_H

typedef void platform_console_out(char *Format, ...);
typedef void platform_console_out_(char *Format, va_list ArgsList);
typedef platform_memory_block *platform_allocate_memory(umm Size, u64 Flags);
typedef void platform_deallocate_memory(platform_memory_block *Block);
typedef b32 platform_file_exists(string Path);
typedef string platform_read_entire_file(memory_arena *Arena, string FilePath);
typedef b32 platform_write_text_to_file(string Text, string FilePath);
typedef platform_file platform_open_file(string FilePath, platform_file_access_flags Flags);
typedef void platform_write_file(platform_file *File, string Text);
typedef void platform_close_file(platform_file *File);
typedef u64 platform_get_os_timer_frequency();
typedef u64 platform_read_os_timer();

typedef struct platform_api
{
    platform_console_out *ConsoleOut;
    platform_console_out_ *ConsoleOut_;
    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;
    platform_file_exists *FileExist;
    platform_read_entire_file *ReadEntireFile;
    platform_write_text_to_file *WriteTextToFile;
    platform_open_file *OpenFile;
    platform_write_file *WriteFile;
    platform_close_file *CloseFile;
    platform_get_os_timer_frequency *GetOSTimerFrequency;
    platform_read_os_timer *ReadOSTimer;
} platform_api;

#if KENGINE_LIBRARY
    global platform_api Platform;
#endif

#define KENGINE_PLATFORM_H
#endif // KENGINE_PLATFORM_H