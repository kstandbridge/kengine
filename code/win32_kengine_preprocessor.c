#include "win32_kengine_preprocessor.h"
#include "win32_kengine_shared.c"

internal void
ExitProcess(u32 ExitCode)
{
    typedef void exit_process(u32 ExitCode);
    
    Assert(Kernel32);
    
    local_persist exit_process *Func = 0;
    if(!Func)
    {
        Func = (exit_process *)GetProcAddressA(Kernel32, "ExitProcess");
    }
    Assert(Func);
    Func(ExitCode);
}

internal HANDLE
GetStdHandle(u32 StdHandle)
{
    typedef HANDLE get_std_handle(u32 StdHandle);
    
    void *Result;
    Assert(Kernel32);
    local_persist get_std_handle *Func = 0;
    if(!Func)
    {
        Func = (get_std_handle *)GetProcAddressA(Kernel32, "GetStdHandle");
    }
    Assert(Func);
    Result = Func(StdHandle);
    
    return Result;
}

internal b32
WriteFile(HANDLE FileHandle, void *Buffer, u32 BytesToWrite, u32 *BytesWritten, OVERLAPPED *Overlapped)
{
    typedef b32 write_file(HANDLE FileHandle, void *Buffer, u32 BytesToWrite, u32 *BytesWritten, OVERLAPPED *Overlapped);
    
    b32 Result;
    Assert(Kernel32);
    local_persist write_file *Func = 0;
    if(!Func)
    {
        Func = (write_file *)GetProcAddressA(Kernel32, "WriteFile");
    }
    Assert(Func);
    Result = Func(FileHandle, Buffer, BytesToWrite, BytesWritten, Overlapped);
    
    return Result;
}

internal HANDLE
CreateFileA(char *FileName, u32 DesiredAccess, u32 ShareMode, SECURITY_ATTRIBUTES *SecurityAttributes, u32 CreationDisposition, u32 FlagsAndAttributes, HANDLE TemplateFile)
{
    typedef HANDLE create_file_a(char *FileName, u32 DesiredAccess, u32 ShareMode, SECURITY_ATTRIBUTES *SecurityAttributes, u32 CreationDisposition, u32 FlagsAndAttributes, HANDLE TemplateFile);
    
    HANDLE Result;
    Assert(Kernel32);
    local_persist create_file_a *Func = 0;
    if(!Func)
    {
        Func = (create_file_a *)GetProcAddressA(Kernel32, "CreateFileA");
    }
    Assert(Func);
    Result = Func(FileName, DesiredAccess, ShareMode, SecurityAttributes, CreationDisposition, FlagsAndAttributes, TemplateFile);
    
    return Result;
}

internal b32
GetFileSizeEx(HANDLE File, LARGE_INTEGER *FileSize)
{
    typedef b32 get_file_size_ex(HANDLE File, LARGE_INTEGER *FileSize);
    
    b32 Result;
    Assert(Kernel32);
    local_persist get_file_size_ex *Func = 0;
    if(!Func)
    {
        Func = (get_file_size_ex *)GetProcAddressA(Kernel32, "GetFileSizeEx");
    }
    Assert(Func);
    Result = Func(File, FileSize);
    
    return Result;
}

internal b32
ReadFile(HANDLE File, void *Buffer, u32 BytesToRead, u32 *BytesRead, OVERLAPPED *Overlapped)
{
    typedef b32 read_file(HANDLE File, void *Buffer, u32 BytesToRead, u32 *BytesRead, OVERLAPPED *Overlapped);
    
    b32 Result;
    Assert(Kernel32);
    local_persist read_file *Func = 0;
    if(!Func)
    {
        Func = (read_file *)GetProcAddressA(Kernel32, "ReadFile");
    }
    Assert(Func);
    Result = Func(File, Buffer, BytesToRead, BytesRead, Overlapped);
    
    return Result;
}

internal b32
CloseHandle(HANDLE Object)
{
    typedef b32 close_handle(HANDLE Object);
    
    b32 Result;
    Assert(Kernel32);
    local_persist close_handle *Func = 0;
    if(!Func)
    {
        Func = (close_handle *)GetProcAddressA(Kernel32, "CloseHandle");
    }
    Assert(Func);
    Result = Func(Object);
    
    return Result;
}

internal void *
VirtualAlloc(void *Address, umm Size, u32 AllocationType, u32 Protect)
{
    typedef void * virtual_alloc(void *Address, umm Size, u32 AllocationType, u32 Protect);
    
    void *Result;
    Assert(Kernel32);
    local_persist virtual_alloc *Func = 0;
    if(!Func)
    {
        Func = (virtual_alloc *)GetProcAddressA(Kernel32, "VirtualAlloc");
    }
    Assert(Func);
    Result = Func(Address, Size, AllocationType, Protect);
    
    return Result;
}

inline string
Win32ReadEntireFile(memory_arena *Arena, char *FilePath)
{
    string Result;
    ZeroStruct(Result);
    
    HANDLE FileHandle = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    LARGE_INTEGER FileSize;
    b32 ReadResult = GetFileSizeEx(FileHandle, &FileSize);
    Assert(ReadResult);
    
    Result.Size = FileSize.QuadPart;
    Result.Data = PushSize(Arena, Result.Size);
    Assert(Result.Data);
    
    u32 BytesRead;
    ReadResult = ReadFile(FileHandle, Result.Data, (u32)Result.Size, &BytesRead, 0);
    Assert(ReadResult);
    Assert(BytesRead == Result.Size);
    
    CloseHandle(FileHandle);
    
    return Result;
}

s32 __stdcall
mainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    char Buffer[] = "Hello, world!\n";
    u32 BufferSize = sizeof(Buffer);
    u32 BytesWritten = 0;
    WriteFile(OutputHandle, Buffer, BufferSize, &BytesWritten, 0);
    Assert(BufferSize == BytesWritten);
    
    
#if KENGINE_INTERNAL
    void *BaseAddress = (void *)Terabytes(2);
#else
    void *BaseAddress = 0;
#endif
    
    u64 MemoryBlockSize = Megabytes(16);
    void *MemoryBlock = VirtualAlloc(BaseAddress, MemoryBlockSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(MemoryBlock);
    
    memory_arena Arena_;
    memory_arena *Arena = &Arena_;
    InitializeArena(Arena, MemoryBlockSize, MemoryBlock);
    
    string TypesSource = Win32ReadEntireFile(Arena, "D:\\kengine\\code\\kengine_types.h");
    WriteFile(OutputHandle, TypesSource.Data, (u32)TypesSource.Size, &BytesWritten, 0);
    Assert(TypesSource.Size == BytesWritten);
    
    ExitProcess(1);
    
    InvalidCodePath;
    
    return 0;
}