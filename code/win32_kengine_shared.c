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

internal void *
GetProcAddressA(HMODULE Module, char *ProcName)
{
    typedef void *get_proc_address(HMODULE Module, char *ProcName);
    
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