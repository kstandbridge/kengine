
inline LARGE_INTEGER
Win32GetWallClock()
{    
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline f32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    Assert(GlobalWin32State.PerfCountFrequency);
    
    f32 Result = ((f32)(End.QuadPart - Start.QuadPart) /
                  (f32)GlobalWin32State.PerfCountFrequency);
    return Result;
}

#define Win32LogError(Format, ...) Win32LogError_(GetLastError(), Format, __VA_ARGS__);
inline void
Win32LogError_(DWORD ErrorCode, char *Format, ...)
{
    u8 Buffer[512];
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Message = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    
    LPTSTR ErrorBuffer = 0;
    DWORD ErrorLength = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 
                                       0, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPTSTR)&ErrorBuffer, 0, 0);
    
    if(ErrorLength)
    {
        string Error = String_(ErrorLength, (u8 *)ErrorBuffer);
        LogError("%S. %S", Message, Error);
    }
    else
    {
        LogError("%S. Error code: %u", Message, ErrorCode);
    }
    
    if(ErrorBuffer)
    {
        LocalFree(ErrorBuffer);
    }
}

platform_memory_block *
Win32AllocateMemory(umm Size, u64 Flags)
{
    // NOTE(kstandbridge): Cache align
    Assert(sizeof(win32_memory_block) == 64);
    
    // NOTE(kstandbridge): Raymond Chen on page sizes: https://devblogs.microsoft.com/oldnewthing/20210510-00/?p=105200
    umm PageSize = Kilobytes(4);
    umm TotalSize = Size + sizeof(win32_memory_block);
    umm BaseOffset = sizeof(win32_memory_block);
    umm ProtectOffset = 0;
    if(Flags & PlatformMemoryBlockFlag_UnderflowCheck)
    {
        TotalSize = Size + 2*PageSize;
        BaseOffset = 2*PageSize;
        ProtectOffset = PageSize;
    }
    else if(Flags & PlatformMemoryBlockFlag_OverflowCheck)
    {
        umm SizeRoundedUp = AlignPow2(Size, PageSize);
        TotalSize = SizeRoundedUp + 2*PageSize;
        BaseOffset = PageSize + SizeRoundedUp - Size;
        ProtectOffset = PageSize + SizeRoundedUp;
    }
    
    LogDebug("Allocating %u bytes", TotalSize);
    
    win32_memory_block *Win32Block = (win32_memory_block *)VirtualAlloc(0, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Win32Block);
    Win32Block->PlatformBlock.Base = (u8 *)Win32Block + BaseOffset;
    Assert(Win32Block->PlatformBlock.Used == 0);
    Assert(Win32Block->PlatformBlock.Prev == 0);
    
    if(Flags & (PlatformMemoryBlockFlag_UnderflowCheck|PlatformMemoryBlockFlag_OverflowCheck))
    {
        DWORD OldProtect = 0;
        b32 IsProtected = VirtualProtect((u8 *)Win32Block + ProtectOffset, PageSize, PAGE_NOACCESS, &OldProtect);
        if(!IsProtected)
        {
            InvalidCodePath;
        }
    }
    
    win32_memory_block *Sentinel = &GlobalWin32State.MemorySentinel;
    Win32Block->Next = Sentinel;
    Win32Block->PlatformBlock.Size = Size;
    Win32Block->PlatformBlock.Flags = Flags;
    
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Win32Block->Prev = Sentinel->Prev;
    Win32Block->Prev->Next = Win32Block;
    Win32Block->Next->Prev = Win32Block;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
    
    platform_memory_block *Result = &Win32Block->PlatformBlock;
    return Result;
}

void
Win32DeallocateMemory(platform_memory_block *Block)
{
    win32_memory_block *Win32Block = (win32_memory_block *)Block;
    
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Win32Block->Prev->Next = Win32Block->Next;
    Win32Block->Next->Prev = Win32Block->Prev;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
    
    LogDebug("Freeing %u bytes", Block->Size);
    b32 IsFreed = VirtualFree(Win32Block, 0, MEM_RELEASE);
    if(!IsFreed)
    {
        InvalidCodePath;
    }
}

internal void
Win32ConsoleOut_(char *Format, va_list ArgList)
{
    u8 Buffer[4096] = {0};
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    AppendFormatString_(&StringState, Format, ArgList);
    
    string Text = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    Text;
#ifdef KENGINE_CONSOLE
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    DWORD NumberOfBytesWritten;
    WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&NumberOfBytesWritten, 0);
    Assert(NumberOfBytesWritten == Text.Size);
#endif
    
#if KENGINE_INTERNAL
    Buffer[Text.Size] = '\0';
    OutputDebugStringA((char *)Buffer);
#endif
}

void
Win32ConsoleOut(char *Format, ...)
{
    va_list ArgList;
    va_start(ArgList, Format);
    
    Win32ConsoleOut_(Format, ArgList);
    
    va_end(ArgList);
    
}

umm
Win32GetAppConfigDirectory(u8 *Buffer, umm BufferMaxSize)
{
    DWORD Result = ExpandEnvironmentStringsA("%AppData%", (char *)Buffer, (DWORD)BufferMaxSize);
    
    if(Result == 0)
    {
        Win32LogError("Failed to expand environment strings");
    }
    else
    {
        // NOTE(kstandbridge): Remove the null terminator
        --Result;
    }
    
    return Result;
}

umm
Win32GetHomeDirectory(u8 *Buffer, umm BufferMaxSize)
{
    DWORD Result = ExpandEnvironmentStringsA("%UserProfile%", (char *)Buffer, (DWORD)BufferMaxSize);
    
    if(Result == 0)
    {
        Win32LogError("Failed to expand environment strings");
    }
    else
    {
        // NOTE(kstandbridge): Remove the null terminator
        --Result;
    }
    
    return Result;
}

b32
Win32DirectoryExists(string Path)
{
    b32 Result;
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    u32 Attributes = GetFileAttributesA(CPath);
    Result = ((Attributes != INVALID_FILE_ATTRIBUTES) &&
              (Attributes & FILE_ATTRIBUTE_DIRECTORY));
    
    return Result;    
}

b32
Win32CreateDirectory(string Path)
{
    b32 Result = false;
    
    LogVerbose("Creating directory %S", Path);
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    SECURITY_ATTRIBUTES SecurityAttributes;
    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = 0;
    SecurityAttributes.bInheritHandle = true;
    
    switch(SHCreateDirectoryExA(0, CPath, &SecurityAttributes))
    {
        case ERROR_BAD_PATHNAME:
        {
            Assert(!"The pszPath parameter was set to a relative path.");
        } break;
        case ERROR_FILENAME_EXCED_RANGE:
        {
            Assert(!"The path pointed to by pszPath is too long.");
        } break;
        case ERROR_PATH_NOT_FOUND:
        {
            Assert(!"The system cannot find the path pointed to by pszPath. The path may contain an invalid entry.");
        } break;
        case ERROR_FILE_EXISTS:
        {
            Assert(!"The directory exists.");
        } break;
        case ERROR_ALREADY_EXISTS:
        {
            Assert(!"The directory exists.");
        } break;
        case ERROR_CANCELLED:
        {
            Assert(!"The user canceled the operation.");
        } break;
        case ERROR_SUCCESS:
        {
            Result = true;
        } break;
        
        InvalidDefaultCase;
    }
    
    return Result;
}

b32
Win32FileExists(string Path)
{
    b32 Result;
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    u32 Attributes = GetFileAttributesA(CPath);
    
    Result = ((Attributes != INVALID_FILE_ATTRIBUTES) && 
              !(Attributes & FILE_ATTRIBUTE_DIRECTORY));
    
    return Result;
}

b32
Win32WriteTextToFile(string Text, string FilePath)
{
    LogVerbose("Writing text to file %S", FilePath);
    
    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);
    HANDLE FileHandle = CreateFileA(CFilePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    DWORD BytesWritten = 0;
    WriteFile(FileHandle, Text.Data, (DWORD)Text.Size, &BytesWritten, 0);
    
    b32 Result = (Text.Size == BytesWritten);
    Assert(Result);
    
    CloseHandle(FileHandle);
    
    return Result;
}

b32
Win32PermanentDeleteFile(string Path)
{
    b32 Result;
    
    LogVerbose("Deleting %S", Path);
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    Result = DeleteFileA(CPath);
    
    return Result;
}

void
Win32ZipDirectory(string SourceDirectory, string DestinationZip)
{
    // NOTE(kstandbridge): No Win32 API to create zip file, so lets build our own!!!
    {
        u8 Buffer[] = {80, 75, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        umm BufferSize = sizeof(Buffer);
        Win32WriteTextToFile(String_(BufferSize, Buffer), DestinationZip);
    }
    
    wchar_t CDestZip[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, (char *)DestinationZip.Data, (int)DestinationZip.Size, CDestZip, MAX_PATH);
    CDestZip[DestinationZip.Size + 0] = '\0';
    CDestZip[DestinationZip.Size + 1] = '\0';
    
    wchar_t CSourceFolder[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, (char *)SourceDirectory.Data, (int)SourceDirectory.Size, CSourceFolder, MAX_PATH);
    CSourceFolder[SourceDirectory.Size + 0] = '\0';
    CSourceFolder[SourceDirectory.Size + 1] = '\0';
    
    HRESULT HResult;
    IShellDispatch *ShellDispatch;
    Folder *ToFolder = 0;
    VARIANT DestZip, SourceFolder, ProgressDialog;
    
    CoInitialize(0);
    
    HResult = CoCreateInstance(&CLSID_Shell, 0, CLSCTX_INPROC_SERVER, &IID_IShellDispatch, (void **)&ShellDispatch);
    
    if (SUCCEEDED(HResult))
    {
        VariantInit(&DestZip);
        DestZip.vt = VT_BSTR;
        DestZip.bstrVal = CDestZip;
        HResult = ShellDispatch->lpVtbl->NameSpace(ShellDispatch, DestZip, &ToFolder);
        
        if(SUCCEEDED(HResult))
        {
            VariantInit(&SourceFolder);
            SourceFolder.vt = VT_BSTR;
            SourceFolder.bstrVal = CSourceFolder;
            
            Folder *FromFolder = 0;
            ShellDispatch->lpVtbl->NameSpace(ShellDispatch, SourceFolder, &FromFolder);
            FolderItems *FolderItems = 0;
            FromFolder->lpVtbl->Items(FromFolder, &FolderItems);
            
            VariantInit(&ProgressDialog);
            ProgressDialog.vt = VT_I4;
            ProgressDialog.lVal = FOF_NO_UI;
            
            VARIANT ItemsToBeCopied;
            VariantInit(&ItemsToBeCopied);
            ItemsToBeCopied.vt = VT_DISPATCH;
            ItemsToBeCopied.pdispVal = (IDispatch *)FolderItems;
            
            HResult = ToFolder->lpVtbl->CopyHere(ToFolder, ItemsToBeCopied, ProgressDialog);
            
            // NOTE(kstandbridge): As much as I hate sleep, CopyHere creates a separate thread
            // which may not finish if this thread exist before completion. So 1000ms sleep.
            Sleep(1000);
            
            ToFolder->lpVtbl->Release(ToFolder);
            FromFolder->lpVtbl->Release(FromFolder);
        }
        
    }
    
    CoUninitialize();
}

u64
Win32GetSystemTimestamp()
{
    u64 Result;
    
    SYSTEMTIME SystemTime;
    GetSystemTime(&SystemTime);
    
    FILETIME FileTime;
    SystemTimeToFileTime(&SystemTime, &FileTime);
    
    ULARGE_INTEGER ULargeInteger;
    ULargeInteger.LowPart = FileTime.dwLowDateTime;
    ULargeInteger.HighPart = FileTime.dwHighDateTime;
    
    Result = ULargeInteger.QuadPart;
    
    return Result;
}

date_time
Win32GetDateTimeFromTimestamp(u64 Timestamp)
{
    date_time Result;
    
    ULARGE_INTEGER ULargeInteger;
    ULargeInteger.QuadPart = Timestamp;
    
    FILETIME FileTime;
    FileTime.dwLowDateTime = ULargeInteger.LowPart;
    FileTime.dwHighDateTime = ULargeInteger.HighPart;
    
    SYSTEMTIME SystemTime;
    FileTimeToSystemTime(&FileTime, &SystemTime);
    
    Result.Year = SystemTime.wYear;
    Result.Month = (u8)SystemTime.wMonth;
    Result.Day = (u8)SystemTime.wDay;
    Result.Hour = (u8)SystemTime.wHour;
    Result.Minute = (u8)SystemTime.wMinute;
    Result.Second = (u8)SystemTime.wSecond;
    Result.Milliseconds = SystemTime.wMilliseconds;
    
    return Result;
}

string
Win32ReadEntireFile(memory_arena *Arena, string FilePath)
{
    string Result = {0};
    
    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);
    
    HANDLE FileHandle = CreateFileA(CFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        b32 ReadResult = GetFileSizeEx(FileHandle, &FileSize);
        Assert(ReadResult);
        if(ReadResult)
        {    
            Result.Size = FileSize.QuadPart;
            Result.Data = PushSize(Arena, Result.Size);
            Assert(Result.Data);
            
            if(Result.Data)
            {
                u32 BytesRead;
                ReadResult = ReadFile(FileHandle, Result.Data, (u32)Result.Size, (LPDWORD)&BytesRead, 0);
                Assert(ReadResult);
                Assert(BytesRead == Result.Size);
            }
        }
        
        CloseHandle(FileHandle);
    }
    
    return Result;
}

typedef struct win32_http_client
{
    HINTERNET Session;
    HINTERNET Handle;
    b32 SkipMetrics;
    
    memory_arena Arena;
    DWORD Flags;
    string Hostname;
    string Username;
    string Password;
} win32_http_client;

platform_http_client
Win32BeginHttpClientWithCreds(string Hostname, u32 Port, string Username, string Password)
{
    platform_http_client Result = {0};
    
    char CUsername_[256];
    char *CUsername = 0;
    if(!IsNullString(Username))
    {
        StringToCString(Username, sizeof(CUsername_), CUsername_);
        CUsername = CUsername_;
    }
    
    char CPassword_[256];
    char *CPassword = 0;
    if(!IsNullString(Password))
    {
        StringToCString(Password, sizeof(CPassword_), CPassword_);
        CPassword = CPassword_;
    }
    
    win32_http_client *Win32Client = BootstrapPushStruct(win32_http_client, Arena);
    Result.Handle = Win32Client;
    Win32Client->Username = Username;
    Win32Client->Password = Password;
    Win32Client->Session = InternetOpenA("Default_User_Agent", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
    
    char CHost_[2048];
    StringToCString(Hostname, sizeof(CHost_), CHost_);
    
    INTERNET_PORT InternetPort = 0;
    if(Port == 0)
    {
        InternetPort = INTERNET_DEFAULT_HTTP_PORT;
    }
    
    Win32Client->Hostname = FormatString(&Win32Client->Arena, "%S", Hostname);
    
    char *CHost = CHost_;
    if(StringBeginsWith(String("https"), Hostname))
    {
        Result.IsHttps = true;
        CHost += 8;
        if(Port == 0)
        {
            Port = INTERNET_DEFAULT_HTTPS_PORT;
        }
        Win32Client->Flags = INTERNET_FLAG_SECURE;
    }
    else if(StringBeginsWith(String("http"), Hostname))
    {
        Result.IsHttps = false;
        CHost += 7;
    }
    
    Result.Hostname = PushString_(&Win32Client->Arena, GetNullTerminiatedStringLength(CHost), (u8 *)CHost);
    Result.Port = Port;
    LogDebug("Opening connection to %s:%u", CHost, Port);
    // TODO(kstandbridge): Can I ditch CUsername and CPassword since they are specified as options above?
    // Then we can pass a string instead as the option takes a length
    Win32Client->Handle = InternetConnectA(Win32Client->Session, CHost, (INTERNET_PORT)Port, CUsername, CPassword,
                                           INTERNET_SERVICE_HTTP, 0, 0);
    
    if(Win32Client->Handle)
    {
        Result.NoErrors = true;
    }
    else
    {
        Result.NoErrors = false;
    }
    
    return Result;
}

void
Win32SkipHttpMetrics(platform_http_client *PlatformClient)
{
    if(PlatformClient->Handle)
    {
        win32_http_client *Win32Client = (win32_http_client *)PlatformClient->Handle;
        Win32Client->SkipMetrics = true;
    }
    else
    {
        LogError("No http client handle to set skip metrics on");
    }
}

void
Win32SetHttpClientTimeout(platform_http_client *PlatformClient, u32 TimeoutMs)
{
    if(PlatformClient->Handle)
    {
        win32_http_client *Win32Client = (win32_http_client *)PlatformClient->Handle;
        if(!InternetSetOptionA(Win32Client->Session, INTERNET_OPTION_RECEIVE_TIMEOUT, &TimeoutMs, sizeof(TimeoutMs)))
        {
            Win32LogError("Failed to set internet recieve timeout option");
            PlatformClient->NoErrors = false;
        }
        if(!InternetSetOptionA(Win32Client->Session, INTERNET_OPTION_SEND_TIMEOUT, &TimeoutMs, sizeof(TimeoutMs)))
        {
            Win32LogError("Failed to set internet send timeout option");
            PlatformClient->NoErrors = false;
        }
        if(!InternetSetOptionA(Win32Client->Session, INTERNET_OPTION_CONNECT_TIMEOUT, &TimeoutMs, sizeof(TimeoutMs)))
        {
            Win32LogError("Failed to set internet connect timeout option");
            PlatformClient->NoErrors = false;
        }
    }
    else
    {
        LogError("No http client handle to set timeout on");
    }
}

typedef struct win32_http_request
{
    win32_http_client *Win32Client;
    HINTERNET Handle;
    LARGE_INTEGER BeginCounter;
} win32_http_request;

internal void
Win32SetHttpRequestHeaders(platform_http_request *PlatformRequest, string Headers)
{
    win32_http_request *Win32Request = PlatformRequest->Handle;
    
    // TODO(kstandbridge): Max header size?
    char CHeaders[MAX_URL];
    StringToCString(Headers, sizeof(CHeaders), CHeaders);
    
    if(!HttpAddRequestHeadersA(Win32Request->Handle, CHeaders, (DWORD) -1, HTTP_ADDREQ_FLAG_ADD))
    {
        PlatformRequest->NoErrors = false;
    }
    
}

platform_http_request
Win32BeginHttpRequest(platform_http_client *PlatformClient, http_verb_type Verb, char *Format, ...)
{
    platform_http_request Result = {0};
    
    Result.Hostname = PlatformClient->Hostname;
    Result.Port = PlatformClient->Port;
    
    if(PlatformClient->Handle)
    {
        win32_http_client *Win32Client = (win32_http_client *)PlatformClient->Handle;
        memory_arena *Arena = &Win32Client->Arena;
        win32_http_request *Win32Request = PushStruct(Arena, win32_http_request);
        Win32Request->Win32Client = Win32Client;
        Win32Request->BeginCounter = Win32GetWallClock();
        Result.Handle = Win32Request;
        
        char *CVerb = 0;
        switch(Verb)
        {
            case HttpVerb_Post:   { CVerb = "POST"; } break;
            case HttpVerb_Get:    { CVerb = "GET"; } break;
            case HttpVerb_Put:    { CVerb = "PUT"; } break;
            case HttpVerb_Patch:  { CVerb = "PATCH"; } break;
            case HttpVerb_Delete: { CVerb = "DELETE"; } break;
            
            InvalidDefaultCase;
        }
        
        Result.Verb = PushString_(Arena, GetNullTerminiatedStringLength(CVerb), (u8 *)CVerb);
        Result.Template = PushString_(Arena, GetNullTerminiatedStringLength(Format), (u8 *)Format);
        
        format_string_state StringState = BeginFormatString();
        
        va_list ArgList;
        va_start(ArgList, Format);
        AppendFormatString_(&StringState, Format, ArgList);
        va_end(ArgList);
        
        Result.Endpoint = EndFormatString(&StringState, Arena);
        
        char CEndpoint[MAX_URL];
        StringToCString(Result.Endpoint, sizeof(CEndpoint), CEndpoint);
        
        if(!Win32Client->SkipMetrics)
        {
            LogVerbose("Http %s request to %S:%u%s", CVerb, Win32Client->Hostname, PlatformClient->Port, CEndpoint);
        }
        Win32Request->Handle = HttpOpenRequestA(Win32Client->Handle, CVerb, CEndpoint, 0, 0, 0, Win32Client->Flags, 0);
        if(Win32Request->Handle)
        {
            Result.NoErrors = true;
            
        }
        else
        {
            Result.NoErrors = false;
        }
    }
    else
    {
        Result.NoErrors = false;
        LogError("No http client handle to close");
    }
    
    return Result;
}

u32
Win32SendHttpRequest(platform_http_request *PlatformRequest)
{
    u32 Result = 0;
    
    win32_http_request *Win32Request = PlatformRequest->Handle;
    win32_http_client *Win32Client = Win32Request->Win32Client;
    
    if(!IsNullString(Win32Client->Username) &&
       !IsNullString(Win32Client->Password))
    {
        u8 UserPassBuffer[MAX_URL];
        string UserPass = FormatStringToBuffer(UserPassBuffer, sizeof(UserPassBuffer),
                                               "%S:%S", Win32Client->Username, Win32Client->Password);
        u8 AuthTokenBuffer[MAX_URL];
        string AuthToken = String_(sizeof(AuthTokenBuffer), AuthTokenBuffer);
        if(CryptBinaryToStringA(UserPass.Data, (DWORD)UserPass.Size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                                (LPSTR)AuthToken.Data, (DWORD *)&AuthToken.Size))
        {
            u8 AuthHeaderBuffer[MAX_URL];
            string AuthHeader = FormatStringToBuffer(AuthHeaderBuffer, sizeof(AuthHeaderBuffer),
                                                     "Authorization: Basic %S\r\n", AuthToken);
            if(!HttpAddRequestHeadersA(Win32Request->Handle, (LPCSTR)AuthHeader.Data, (DWORD)AuthHeader.Size, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE))
            {
                Win32LogError("Failed to add basic authorization header");
            }
        }
        else
        {
            Win32LogError("Failed to generate auth token");
        }
    }
    
    PlatformRequest->RequestLength = PlatformRequest->Payload.Size;
    if(HttpSendRequestA(Win32Request->Handle, 0, 0, PlatformRequest->Payload.Data, (DWORD)PlatformRequest->Payload.Size))
    {
        DWORD StatusCode = 0;
        DWORD StatusCodeSize = sizeof(StatusCodeSize);
        if(HttpQueryInfoA(Win32Request->Handle, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, (LPVOID)&StatusCode, &StatusCodeSize, 0))
        {
            LogDebug("Received status code %u on http request", StatusCode);
            Result = StatusCode;
        }
        else
        {
            PlatformRequest->NoErrors = false;
        }
    }
    else
    {
        DWORD ErrorCode = GetLastError();
        if(ErrorCode == ERROR_INTERNET_TIMEOUT)
        {
            LogVerbose("Client timeout on http request");
            Result = 408; // TODO(kstandbridge): status code enum?
            PlatformRequest->NoErrors = false;
        }
    }
    
    PlatformRequest->StatusCode = Result;
    
    return Result;
}

string
Win32GetHttpResponse(platform_http_request *PlatformRequest)
{
    string Result = {0};
    
    win32_http_request *Win32Request = PlatformRequest->Handle;
    win32_http_client *Win32Client = Win32Request->Win32Client;
    memory_arena *Arena = &Win32Client->Arena;
    
    DWORD ContentLength = 0;
    DWORD ContentLengthSize = sizeof(ContentLengthSize);
    
    if(!HttpQueryInfoA(Win32Request->Handle, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, (LPVOID)&ContentLength, &ContentLengthSize, 0))
    {
        DWORD ErrorCode = GetLastError();
        if(ErrorCode != ERROR_HTTP_HEADER_NOT_FOUND)
        {
            PlatformRequest->NoErrors = false;
        }
    }
    
    if(PlatformRequest->NoErrors)
    {
        if(ContentLength == 0)
        {
            ContentLength = (DWORD)(Arena->CurrentBlock->Size - Arena->CurrentBlock->Used);
            if(ContentLength < Kilobytes(1536))
            {
                ContentLength = Kilobytes(1536);
            }
            LogDebug("Content-Length not specified so setting to %u", ContentLength);
        }
        
        Result.Size = ContentLength;
        Result.Data = PushSize_(Arena, ContentLength, DefaultArenaPushParams());
        
        u8 *SaveBuffer = Result.Data;
        
        DWORD TotalBytesRead = 0;
        DWORD CurrentBytesRead;
        LARGE_INTEGER LastCounter = Win32GetWallClock();
        f32 SecondsSinceLastReport = 0.0f;
        for(;;)
        {
            if(InternetReadFile(Win32Request->Handle, SaveBuffer + TotalBytesRead, 4096, &CurrentBytesRead))
            {
                if(CurrentBytesRead == 0)
                {
                    break;
                }
                TotalBytesRead += CurrentBytesRead;
                if(TotalBytesRead == ContentLength)
                {
                    break;
                }
            }
            else
            {
                DWORD ErrorCode = GetLastError();
                if (ErrorCode == ERROR_INSUFFICIENT_BUFFER)
                {
                    LogError("InternetReadFile failed due to insufficent buffer size");
                }
                PlatformRequest->NoErrors = false;
                break;
            }
            LARGE_INTEGER ThisCounter = Win32GetWallClock();
            f32 SecondsElapsed = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
            SecondsSinceLastReport += SecondsElapsed;
            if(SecondsSinceLastReport > 3.0f)
            {
                if(!Win32Client->SkipMetrics)
                {
                    LogDownloadProgress(TotalBytesRead, ContentLength);
                }
                SecondsSinceLastReport = 0.0f;
            }
            LastCounter = ThisCounter;
        }
        
        if(TotalBytesRead > ContentLength)
        {
            LogError("Buffer overflow during download!");
        }
        Result.Size = TotalBytesRead;
        
        if(!Win32Client->SkipMetrics)
        {
            LogDownloadProgress(TotalBytesRead, TotalBytesRead);
        }
    }
    
    PlatformRequest->ResponseLength = Result.Size;
    return Result;
}

void
Win32EndHttpRequest(platform_http_request *PlatformRequest)
{
    if(PlatformRequest->Handle)
    {
        LogDebug("Ending http request");
        win32_http_request *Win32Request = (win32_http_request *)PlatformRequest->Handle;
        HttpEndRequestA(Win32Request->Handle, 0, 0, 0);
        
        if(!Win32Request->Win32Client->SkipMetrics)
        {            
            LARGE_INTEGER EndCounter = Win32GetWallClock();
            f32 MeasuredSeconds = Win32GetSecondsElapsed(Win32Request->BeginCounter, EndCounter);
            SendTimedHttpTelemetry(PlatformRequest->Hostname, PlatformRequest->Port, PlatformRequest->Verb, PlatformRequest->Template, PlatformRequest->Endpoint, PlatformRequest->RequestLength, PlatformRequest->ResponseLength, PlatformRequest->StatusCode,
                                   MeasuredSeconds);
        }
        
    }
    else
    {
        LogError("No http request handle to close");
    }
}

void
Win32EndHttpClient(platform_http_client *PlatformClient)
{
    if(PlatformClient->Handle)
    {
        win32_http_client *Win32Client = (win32_http_client *)PlatformClient->Handle;
        InternetCloseHandle(Win32Client->Handle);
        InternetCloseHandle(Win32Client->Session);
        ClearArena(&Win32Client->Arena);
    }
    else
    {
        LogError("No http client handle to close");
    }
}

u32
Win32SendHttpRequestFromFile(platform_http_request *PlatformRequest, string File)
{
    u32 Result = 0;
    
    win32_http_request *Win32Request = PlatformRequest->Handle;
    
    char CFile[MAX_PATH];
    StringToCString(File, MAX_PATH, CFile);
    
    HANDLE FileHandle = CreateFileA(CFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER LFileSize;
        GetFileSizeEx(FileHandle, &LFileSize);
        umm FileSize = LFileSize.QuadPart;
        PlatformRequest->RequestLength = FileSize;
        
        u8 HeaderBuffer[256];
        string Header = FormatStringToBuffer(HeaderBuffer, sizeof(HeaderBuffer), "Content-Length: %u\r\n", FileSize);
        HeaderBuffer[Header.Size] = '\0';
        HttpAddRequestHeadersA(Win32Request->Handle, (char *)HeaderBuffer, 
                               (DWORD)-1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
        
        b32 RetryOn401 = true;
        for(;;)
        {
            if(HttpSendRequestExA(Win32Request->Handle, 0, 0, HSR_INITIATE, 0))
            {                
                umm Offset = 0;
                OVERLAPPED Overlapped = {0};
                
                u8 Buffer[1024] = {0};
                umm BufferSize = 1024;
                do
                {
                    Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
                    Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);
                    
                    DWORD BytesRead = 0;
                    if(ReadFile(FileHandle, Buffer, (int)BufferSize, &BytesRead, &Overlapped))
                    {
                        DWORD BytesSent = 0;
                        DWORD TotalBytesSend = 0;
                        do
                        {
                            if(!InternetWriteFile(Win32Request->Handle, Buffer + TotalBytesSend, BytesRead, &BytesSent))
                            {
                                FileSize = 0;
                                break;
                            }
                            TotalBytesSend += BytesSent;
                        } while(TotalBytesSend < BytesRead);
                        
                        Offset += BytesRead;
                    }
                    else
                    {
                        Assert(!"Error reading file!");
                    }
                } while(Offset < FileSize);
                
                HttpEndRequestA(Win32Request->Handle, 0, 0, 0);
                
                DWORD StatusCode = 0;
                DWORD StatusCodeSize = sizeof(StatusCodeSize);
                if(HttpQueryInfoA(Win32Request->Handle, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, (LPVOID)&StatusCode, &StatusCodeSize, 0))
                {
                    LogDebug("Received status code %u on http request", StatusCode);
                    
                    // NOTE(kstandbridge): HttpSendRequestEx will return 401 initially asking for creds, so try send the request again
                    if((StatusCode == 401) 
                       && RetryOn401)
                    {
                        RetryOn401 = false;
                    }
                    else
                    {
                        Result = StatusCode;
                        break;
                    }
                }
                else
                {
                    PlatformRequest->NoErrors = false;
                    break;
                }
            }
            else
            {
                PlatformRequest->NoErrors = false;
                break;
            }
        }
        
    }
    else
    {
        LogError("Unable to open file %S", File);
    }
    
    PlatformRequest->StatusCode = Result;
    return Result;
}

u32
Win32ExecuteProcessWithOutput(string Path, string Args, string WorkingDirectory, platform_execute_process_callback *Callback)
{
    u32 Result = 0;
    
    LogVerbose("Working directory set to %S", WorkingDirectory);
    LogVerbose("Executing %S", Args);
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    char CArgs[MAX_PATH];
    StringToCString(Args, MAX_PATH, CArgs);
    
    char CWorkingDirectory[MAX_PATH];
    StringToCString(WorkingDirectory, MAX_PATH, CWorkingDirectory);
    
    SECURITY_ATTRIBUTES SecurityAttributes;
    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = 0;
    SecurityAttributes.bInheritHandle = true;
    
    HANDLE InPipeRead;
    HANDLE InPipeWrite;
    if(CreatePipe(&InPipeRead, &InPipeWrite, &SecurityAttributes, 0))
    {
        HANDLE OutPipeRead;
        HANDLE OutPipeWrite;
        if(CreatePipe(&OutPipeRead, &OutPipeWrite, &SecurityAttributes, 0))
        {
            STARTUPINFOA StartupInfo = {0};
            
            StartupInfo.cb = sizeof(STARTUPINFOA);
            StartupInfo.dwFlags = STARTF_USESTDHANDLES;
            StartupInfo.hStdError = OutPipeWrite;
            StartupInfo.hStdOutput = OutPipeWrite;
            StartupInfo.hStdInput = InPipeRead;
            
            PROCESS_INFORMATION ProcessInformation = {0};
            
            if(CreateProcessA(CPath, CArgs, &SecurityAttributes, 0, true, DETACHED_PROCESS, 
                              0, CWorkingDirectory, &StartupInfo, &ProcessInformation))
            {
#define BUFFER_SIZE 1024
                CloseHandle(OutPipeWrite);
                CloseHandle(InPipeRead);
                
                char ReadingBuffer[BUFFER_SIZE];
                DWORD ReadIndex = 0;
                b32 IsReading = ReadFile(OutPipeRead, ReadingBuffer, BUFFER_SIZE, &ReadIndex, 0);
                while(IsReading)
                {
                    ReadingBuffer[ReadIndex] = '\0';
                    u32 OutputIndex = 0;
                    char OutputBuffer[BUFFER_SIZE];
                    for(u32 Index = 0;
                        Index <= ReadIndex;
                        ++Index)
                    {
                        if((ReadingBuffer[Index] == '\0') ||
                           (ReadingBuffer[Index] == '\r') ||
                           (ReadingBuffer[Index] == '\n'))
                        {
                            OutputBuffer[OutputIndex] = '\0';
                            
                            if(OutputIndex > 0)
                            {
                                string Output = String_(OutputIndex, (u8 *)OutputBuffer);
                                Callback(Output);
                            }
                            OutputIndex = 0;
                        }
                        else
                        {
                            OutputBuffer[OutputIndex++] = ReadingBuffer[Index];
                        }
                    }
                    IsReading = ReadFile(OutPipeRead, ReadingBuffer, BUFFER_SIZE, &ReadIndex, 0);
                }
            }
            else
            {
                // TODO(kstandbridge): Return the exit code?
                Win32LogError("Failed to create process: %s %s", CPath, CArgs);
            }
            
            CloseHandle(OutPipeRead);
            
            DWORD ExitCode = 0;
            GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode);
            Result = ExitCode;
        }
        else
        {
            Win32LogError("Failed to create pipe for process: %s %s", CPath, CArgs);
        }
        
        CloseHandle(InPipeWrite);
    }
    else
    {
        Win32LogError("Failed to create pipe for process: %s %s", CPath, CArgs);
    }
    
    return Result;
}

void
Win32UnzipToDirectory(string SourceZip, string DestFolder)
{
    if(!Win32DirectoryExists(DestFolder))
    {
        Win32CreateDirectory(DestFolder);
    }
    
    LogVerbose("Unzipping %S to %S", SourceZip, DestFolder);
    
    wchar_t CSourceZip[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, (char *)SourceZip.Data, (int)SourceZip.Size, CSourceZip, MAX_PATH);
    CSourceZip[SourceZip.Size + 0] = '\0';
    CSourceZip[SourceZip.Size + 1] = '\0';
    
    wchar_t CDestFolder[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, (char *)DestFolder.Data, (int)DestFolder.Size, CDestFolder, MAX_PATH);
    CDestFolder[DestFolder.Size + 0] = '\0';
    CDestFolder[DestFolder.Size + 1] = '\0';
    
    HRESULT HResult;
    IShellDispatch *ShellDispatch;
    Folder *ToFolder = 0;
    VARIANT OutputDirectory, SourceFile, ProgressDialog;
    
    CoInitialize(0);
    
    HResult = CoCreateInstance(&CLSID_Shell, 0, CLSCTX_INPROC_SERVER, &IID_IShellDispatch, (void **)&ShellDispatch);
    
    if (SUCCEEDED(HResult))
    {
        Folder *FromFolder = 0;
        
        VariantInit(&SourceFile);
        SourceFile.vt = VT_BSTR;
        SourceFile.bstrVal = CSourceZip;
        
        VariantInit(&OutputDirectory);
        OutputDirectory.vt = VT_BSTR;
        OutputDirectory.bstrVal = CDestFolder;
        ShellDispatch->lpVtbl->NameSpace(ShellDispatch, OutputDirectory, &ToFolder);
        
        ShellDispatch->lpVtbl->NameSpace(ShellDispatch, SourceFile, &FromFolder);
        FolderItems *FolderItems = 0;
        FromFolder->lpVtbl->Items(FromFolder, &FolderItems);
        
        VariantInit(&ProgressDialog);
        ProgressDialog.vt = VT_I4;
        ProgressDialog.lVal = FOF_NO_UI;
        //ProgressDialog.lVal = FOF_SIMPLEPROGRESS;
        
        VARIANT ItemsToBeCopied;
        VariantInit(&ItemsToBeCopied);
        ItemsToBeCopied.vt = VT_DISPATCH;
        ItemsToBeCopied.pdispVal = (IDispatch *)FolderItems;
        
        HResult = ToFolder->lpVtbl->CopyHere(ToFolder, ItemsToBeCopied, ProgressDialog);
        
        // NOTE(kstandbridge): As much as I hate sleep, CopyHere creates a separate thread
        // which may not finish if this thread exist before completion. So 1000ms sleep.
        Sleep(1000);
        
        FromFolder->lpVtbl->Release(FromFolder);
        ToFolder->lpVtbl->Release(ToFolder);
    }
    
    CoUninitialize();
}

umm
Win32GetHostname(u8 *Buffer, umm BufferMaxSize)
{
    DWORD Result = (DWORD)BufferMaxSize;
    
    if(!GetComputerNameA((char *)Buffer, &Result))
    {
        Win32LogError("Failed to get computer name");
    }
    
    return Result;
}

umm
Win32GetUsername(u8 *Buffer, umm BufferMaxSize)
{
    umm Result = GetEnvironmentVariableA("Username", (char *)Buffer, (DWORD)BufferMaxSize);
    
    if(Result == 0)
    {
        Win32LogError("Failed to get environment variable");
    }
    
    return Result;
}

u32
Win32GetProcessId()
{
    u32 Result = GetCurrentProcessId();
    
    return Result;
}

void
Win32DeleteHttpCache(char *Format, ...)
{
    format_string_state StringState = BeginFormatString();
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    u8 CUrlName[MAX_URL];
    string UrlName = EndFormatStringToBuffer(&StringState, CUrlName, sizeof(CUrlName));
    CUrlName[UrlName.Size] = 0;
    
    b32 Success = DeleteUrlCacheEntryA((char *)CUrlName);
    if(Success)
    {
        LogDebug("Deleted Url cache entry for %S", UrlName);
    }
    else
    {
        LogDebug("Url cache entry not found %S", UrlName);
    }
}

umm
Win32GetHttpResponseToFile(platform_http_request *PlatformRequest, string File)
{
    umm Result = 0;
    
    win32_http_request *Win32Request = PlatformRequest->Handle;
    win32_http_client *Win32Client = Win32Request->Win32Client;
    
    u64 ContentLength = 0;
    u8 ContentLengthBuffer[MAX_URL];
    DWORD ContentLengthSize = sizeof(ContentLengthBuffer);
    char *Query = "Content-Length";
    Copy(GetNullTerminiatedStringLength(Query) + 1, Query, ContentLengthBuffer);
    
    if(!HttpQueryInfoA(Win32Request->Handle, HTTP_QUERY_CUSTOM, (LPVOID)&ContentLengthBuffer, (LPDWORD)&ContentLengthSize, 0))
    {
        DWORD ErrorCode = GetLastError();
        if(ErrorCode != ERROR_HTTP_HEADER_NOT_FOUND)
        {
            PlatformRequest->NoErrors = false;
        }
    }
    else
    {
        string ContentLengthText = String_(ContentLengthSize, ContentLengthBuffer);
        ParseFromString(ContentLengthText, "%lu", &ContentLength);
    }
    
    if(PlatformRequest->NoErrors)
    {
        u8 CFile[MAX_PATH];
        StringToCString(File, sizeof(CFile), (char *)CFile);
        HANDLE SaveHandle = CreateFileA((char *)CFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);;
        u8 SaveBuffer[4096];
        
        LogVerbose("Downloading to file %s", CFile);
        u64 TotalBytesRead = 0;
        DWORD CurrentBytesRead;
        LARGE_INTEGER LastCounter = Win32GetWallClock();
        f32 SecondsSinceLastReport = 0.0f;
        for(;;)
        {
            if(InternetReadFile(Win32Request->Handle, SaveBuffer, sizeof(SaveBuffer), &CurrentBytesRead))
            {
                TotalBytesRead += CurrentBytesRead;
            }
            else
            {
                DWORD ErrorCode = GetLastError();
                if(ErrorCode == ERROR_INSUFFICIENT_BUFFER)
                {
                    LogError("InternetReadFile failed due to insufficent buffer size");
                }
                PlatformRequest->NoErrors = false;
                break;
            }
            DWORD BytesWritten;
            WriteFile(SaveHandle, SaveBuffer, CurrentBytesRead, &BytesWritten, 0);
            
            LARGE_INTEGER ThisCounter = Win32GetWallClock();
            f32 SecondsElapsed = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
            SecondsSinceLastReport += SecondsElapsed;
            if(SecondsSinceLastReport > 3.0f)
            {
                if(!Win32Client->SkipMetrics)
                {
                    LogDownloadProgress(TotalBytesRead, ContentLength);
                }
                SecondsSinceLastReport = 0.0f;
            }
            if((TotalBytesRead == ContentLength) ||
               (CurrentBytesRead == 0))
            {
                break;
            }
            LastCounter = ThisCounter;
        }
        
        CloseHandle(SaveHandle);
        
        Result = TotalBytesRead;
        
        if(!Win32Client->SkipMetrics)
        {
            LogDownloadProgress(TotalBytesRead, TotalBytesRead);
        }
        
    }
    
    PlatformRequest->ResponseLength = Result;
    return Result;
}


typedef enum iterate_processes_op
{
    IterateProcesses_None,
    IterateProcesses_Terminate,
    IterateProcesses_RequestClose
} iterate_processes_op;

internal b32
Win32IterateProcesses_(string Name, iterate_processes_op Op)
{
    b32 Result = false;
    
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    PROCESSENTRY32 Entry = {0};
    Entry.dwSize = sizeof(PROCESSENTRY32);
    b32 ProcessFound = Process32First(Snapshot, &Entry);
    while(ProcessFound)
    {
        if(StringsAreEqual(CStringToString(Entry.szExeFile), Name))
        {
            HANDLE Process = OpenProcess(PROCESS_TERMINATE, false, Entry.th32ProcessID);
            if(Process != 0)
            {
                Result = true;
                if(Op == IterateProcesses_RequestClose)
                {
                    u8 CPipe[MAX_PATH];
                    string Pipe = FormatStringToBuffer(CPipe, sizeof(CPipe),  "\\\\.\\pipe\\close-request-pipe-%d", Entry.th32ProcessID);
                    CPipe[Pipe.Size] = '\0';
                    HANDLE PipeHandle = CreateFileA((char *)CPipe, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
                    
                    // NOTE(kstandbridge): Give it 3 seconds to close
                    Sleep(3000); 
                    CloseHandle(PipeHandle);
                    
                    // NOTE(kstandbridge): Kill it anyway
                    TerminateProcess(Process, 0);
                }
                else if(Op == IterateProcesses_Terminate)
                {                    
                    TerminateProcess(Process, 0);
                }
                else
                {
                    Assert(Op == IterateProcesses_None);
                }
                CloseHandle(Process);
                break;
            }
            else
            {
                Assert(!"Lack PROCESS_TERMINATE access to Firebird.exe");
            }
        }
        ProcessFound = Process32Next(Snapshot, &Entry);
    }
    CloseHandle(Snapshot);
    
    return Result;
}

b32
Win32RequestCloseProcess(string FileName)
{
    b32 Result = Win32IterateProcesses_(FileName, IterateProcesses_RequestClose);
    return Result;
}

b32
Win32KillProcess(string Name)
{
    b32 Result = Win32IterateProcesses_(Name, IterateProcesses_Terminate);
    return Result;
}

b32
Win32CheckForProcess(string Name)
{
    b32 Result = Win32IterateProcesses_(Name, IterateProcesses_None);
    return Result;
}

void
Win32ExecuteProcess(string Path, string Args, string WorkingDirectory)
{
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    char CArgs[MAX_PATH];
    StringToCString(Args, MAX_PATH, CArgs);
    
    char CWorkingDirectory[MAX_PATH];
    StringToCString(WorkingDirectory, MAX_PATH, CWorkingDirectory);
    
    ShellExecuteA(0, "open", CPath, CArgs, CWorkingDirectory, 0);
}
