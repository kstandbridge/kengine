#ifndef WIN32_KENGINE_TYPES_H

typedef void * HMODULE;
typedef void * HANDLE;

typedef __int64 LONG_PTR;
typedef unsigned __int64 ULONG_PTR;

typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;

typedef struct _IMAGE_DOS_HEADER 
{                                    // DOS .EXE header
    u16 e_magic;                     // Magic number
    u16 e_cblp;                      // Bytes on last page of file
    u16 e_cp;                        // Pages in file
    u16 e_crlc;                      // Relocations
    u16 e_cparhdr;                   // Size of header in paragraphs
    u16 e_minalloc;                  // Minimum extra paragraphs needed
    u16 e_maxalloc;                  // Maximum extra paragraphs needed
    u16 e_ss;                        // Initial (relative) SS value
    u16 e_sp;                        // Initial SP value
    u16 e_csum;                      // Checksum
    u16 e_ip;                        // Initial IP value
    u16 e_cs;                        // Initial (relative) CS value
    u16 e_lfarlc;                    // File address of relocation table
    u16 e_ovno;                      // Overlay number
    u16 e_res[4];                    // Reserved words
    u16 e_oemid;                     // OEM identifier (for e_oeminfo)
    u16 e_oeminfo;                   // OEM information; e_oemid specific
    u16 e_res2[10];                  // Reserved words
    s32 e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER 
{
    u16 Machine;
    u16 NumberOfSections;
    u32 TimeDateStamp;
    u32 PointerToSymbolTable;
    u32 NumberOfSymbols;
    u16 SizeOfOptionalHeader;
    u16 Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY 
{
    u32 VirtualAddress;
    u32 Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_OPTIONAL_HEADER 
{
    //
    // Standard fields.
    //
    
    u16 Magic;
    u8  MajorLinkerVersion;
    u8  MinorLinkerVersion;
    u32 SizeOfCode;
    u32 SizeOfInitializedData;
    u32 SizeOfUninitializedData;
    u32 AddressOfEntryPoint;
    u32 BaseOfCode;
    u32 BaseOfData;
    
    //
    // NT additional fields.
    //
    
    u32 ImageBase;
    u32 SectionAlignment;
    u32 FileAlignment;
    u16 MajorOperatingSystemVersion;
    u16 MinorOperatingSystemVersion;
    u16 MajorImageVersion;
    u16 MinorImageVersion;
    u16 MajorSubsystemVersion;
    u16 MinorSubsystemVersion;
    u32 Win32VersionValue;
    u32 SizeOfImage;
    u32 SizeOfHeaders;
    u32 CheckSum;
    u16 Subsystem;
    u16 DllCharacteristics;
    u32 SizeOfStackReserve;
    u32 SizeOfStackCommit;
    u32 SizeOfHeapReserve;
    u32 SizeOfHeapCommit;
    u32 LoaderFlags;
    u32 NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;


typedef struct _IMAGE_OPTIONAL_HEADER64 
{
    u16       Magic;
    u8        MajorLinkerVersion;
    u8        MinorLinkerVersion;
    u32       SizeOfCode;
    u32       SizeOfInitializedData;
    u32       SizeOfUninitializedData;
    u32       AddressOfEntryPoint;
    u32       BaseOfCode;
    ULONGLONG ImageBase;
    u32       SectionAlignment;
    u32       FileAlignment;
    u16       MajorOperatingSystemVersion;
    u16       MinorOperatingSystemVersion;
    u16       MajorImageVersion;
    u16       MinorImageVersion;
    u16       MajorSubsystemVersion;
    u16       MinorSubsystemVersion;
    u32       Win32VersionValue;
    u32       SizeOfImage;
    u32       SizeOfHeaders;
    u32       CheckSum;
    u16       Subsystem;
    u16       DllCharacteristics;
    ULONGLONG SizeOfStackReserve;
    ULONGLONG SizeOfStackCommit;
    ULONGLONG SizeOfHeapReserve;
    ULONGLONG SizeOfHeapCommit;
    u32       LoaderFlags;
    u32       NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64 
{
    u32 Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_EXPORT_DIRECTORY 
{
    u32 Characteristics;
    u32 TimeDateStamp;
    u16 MajorVersion;
    u16 MinorVersion;
    u32 Name;
    u32 Base;
    u32 NumberOfFunctions;
    u32 NumberOfNames;
    u32 AddressOfFunctions;     // RVA from base of image
    u32 AddressOfNames;         // RVA from base of image
    u32 AddressOfNameOrdinals;  // RVA from base of image
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;
#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define STD_INPUT_HANDLE    ((u32)-10)
#define STD_OUTPUT_HANDLE   ((u32)-11)
#define STD_ERROR_HANDLE    ((u32)-12)

typedef struct _OVERLAPPED 
{
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union 
    {
        struct 
        {
            u32 Offset;
            u32 OffsetHigh;
        } DUMMYSTRUCTNAME;
        
        void *Pointer;
    } DUMMYUNIONNAME;
    
    HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES 
{
    u32 nLength;
    void *lpSecurityDescriptor;
    b32 bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef union _LARGE_INTEGER 
{
    struct 
    {
        u32 LowPart;
        s64 HighPart;
    } DUMMYSTRUCTNAME;
    struct 
    {
        u32 LowPart;
        s64 HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER;

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

#define FILE_SHARE_READ                 0x00000001

#define OPEN_EXISTING       3

#define MEM_COMMIT                      0x00001000  
#define MEM_RESERVE                     0x00002000  

#define PAGE_READWRITE          0x04    

introspect(win32, Kernel32);
typedef char * get_command_line_a();
introspect(win32, Kernel32);
typedef void * virtual_alloc(void *Address, umm Size, u32 AllocationType, u32 Protect);
introspect(win32, Kernel32);
typedef HMODULE load_library_a(char *FileName);
introspect(win32, Kernel32);
typedef void exit_process(u32 ExitCode);
introspect(win32, Kernel32);
typedef HANDLE get_std_handle(u32 StdHandle);
introspect(win32, Kernel32);
typedef b32 write_file(HANDLE FileHandle, void *Buffer, u32 BytesToWrite, u32 *BytesWritten, OVERLAPPED *Overlapped);
introspect(win32, Kernel32);
typedef HANDLE create_file_a(char *FileName, u32 DesiredAccess, u32 ShareMode, SECURITY_ATTRIBUTES *SecurityAttributes, u32 CreationDisposition, u32 FlagsAndAttributes, HANDLE TemplateFile);
introspect(win32, Kernel32);
typedef b32 get_file_size_ex(HANDLE File, LARGE_INTEGER *FileSize);
introspect(win32, Kernel32);
typedef b32 read_file(HANDLE File, void *Buffer, u32 BytesToRead, u32 *BytesRead, OVERLAPPED *Overlapped);
introspect(win32, Kernel32);
typedef b32 close_handle(HANDLE Object);

#define WIN32_KENGINE_TYPES_H
#endif //WIN32_KENGINE_TYPES_H
