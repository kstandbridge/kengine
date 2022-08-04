#include "kengine_platform.h"
#include "win32_kengine_types.h"

internal HMODULE 
FindModuleBase(void *Ptr)
{
    HMODULE Result = 0;
    
    ULONG_PTR Address = (ULONG_PTR)Ptr;
    Address &= ~0xFFFF;
    u32 *Module = (u32 *)Address;
    while(Module[0] != 0x00905A4D) // NOTE(kstandbridge): MZ Header
    {
        Module -= 0x4000; // NOTE(kstandbridge): 0x10000/4
    }
    
    Result = (HMODULE)Module;
    return Result;
}

#define RELATIVE_PTR(Base, Offset) ( ((u8 *)Base) + Offset )
internal void *
FindGetProcessAddress(HMODULE Module)
{
    void *Result = 0;
    
    IMAGE_DOS_HEADER *ImageDosHeader = (IMAGE_DOS_HEADER *)Module;
    IMAGE_NT_HEADERS64 *ImageNtHeaders = (IMAGE_NT_HEADERS64 *)RELATIVE_PTR(ImageDosHeader, ImageDosHeader->e_lfanew);
    IMAGE_EXPORT_DIRECTORY *ImageExportDirectory = (IMAGE_EXPORT_DIRECTORY *)RELATIVE_PTR(ImageDosHeader, 
                                                                                          ImageNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    u32 *Names = (u32 *)RELATIVE_PTR(ImageDosHeader, ImageExportDirectory->AddressOfNames);
    for(u32 Index = 0;
        Index < ImageExportDirectory->NumberOfNames;
        ++Index)
    {
        u32 *Name32 = (u32 *)RELATIVE_PTR(ImageDosHeader, Names[Index]);
        u16 *Name16 = (u16 *)Name32;
        u8 *Name8 = (u8 *)Name32;
        if((Name32[0]!=0x50746547) || // GetP
           (Name32[1]!=0x41636f72) || // rocA
           (Name32[2]!=0x65726464) || // ddre
           (Name16[6]!=0x7373) ||     // ss
           (Name8[14]!=0x00))         // '\0'
        {
            continue;
        }
        u16 *Ordinals = (u16 *)RELATIVE_PTR(ImageDosHeader, ImageExportDirectory->AddressOfNameOrdinals);
        u32 *Functions = (u32 *)RELATIVE_PTR(ImageDosHeader, ImageExportDirectory->AddressOfFunctions);
        Result = RELATIVE_PTR(ImageDosHeader, Functions[Ordinals[Index]]);
        break;
    }
    
    return Result;
}

global HMODULE Kernel32;

typedef void *get_proc_address(HMODULE Module, char *ProcName);
internal void *
GetProcAddressA(HMODULE Module, char *ProcName)
{
    void *Result = 0;
    
    Assert(Kernel32);
    
    local_persist get_proc_address *Func = 0;
    if(!Func)
    {
        Func = (get_proc_address *)FindGetProcessAddress(Kernel32);
    }
    Assert(Func);
    Result = Func(Module, ProcName);
    
    return Result;
}

typedef void exit_process(u32 ExitCode);
internal void
ExitProcess(u32 ExitCode)
{
    Assert(Kernel32);
    
    local_persist exit_process *Func = 0;
    if(!Func)
    {
        Func = (exit_process *)GetProcAddressA(Kernel32, "ExitProcess");
    }
    Assert(Func);
    Func(ExitCode);
}

typedef HANDLE get_std_handle(u32 StdHandle);
internal HANDLE
GetStdHandle(u32 StdHandle)
{
    void *Result = 0;
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

typedef b32 write_file(HANDLE FileHandle, void *Buffer, u32 BytesToWrite, u32 *BytesWritten, OVERLAPPED *Overlapped);
internal b32
WriteFile(HANDLE FileHandle, void *Buffer, u32 BytesToWrite, u32 *BytesWritten, OVERLAPPED *Overlapped)
{
    b32 Result = false;
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

s32 __stdcall
mainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    char Buffer[] = "Hello, world!";
    u32 BufferSize = sizeof(Buffer);
    u32 BytesWritten = 0;
    WriteFile(OutputHandle, Buffer, BufferSize, &BytesWritten, 0);
    Assert(BufferSize == BytesWritten);
    
    ExitProcess(1);
    
    return 0;
}