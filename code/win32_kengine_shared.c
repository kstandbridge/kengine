
inline void
Win32LogError_(string Function, DWORD ErrorCode)
{
    LPTSTR ErrorBuffer = 0;
    DWORD ErrorLength = Win32FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 
                                            0, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPTSTR)&ErrorBuffer, 0, 0);
    
    if(ErrorLength)
    {
        string Error = String_(ErrorLength, (u8 *)ErrorBuffer);
        LogError("%S failed due to %S", Function, Error);
    }
    else
    {
        LogError("%S failed due to error code %u", Function, ErrorCode);
    }
    
    if(ErrorBuffer)
    {
        Win32LocalFree(ErrorBuffer);
    }
}

inline void
Win32LogError(string Function)
{
    DWORD ErrorCode = Win32GetLastError();
    Win32LogError_(Function, ErrorCode);
}

typedef struct win32_memory_block
{
    platform_memory_block PlatformBlock;
    struct win32_memory_block *Prev;
    struct win32_memory_block *Next;
    u64 Padding;
} win32_memory_block;

typedef struct win32_state
{
    ticket_mutex MemoryMutex;
    win32_memory_block MemorySentinel;
    
    memory_arena Arena;
    
    HWND Window;
    
    s64 PerfCountFrequency;
    
#if KENGINE_INTERNAL    
    char ExeFilePath[MAX_PATH];
    char DllFullFilePath[MAX_PATH];
    char TempDllFullFilePath[MAX_PATH];
    char LockFullFilePath[MAX_PATH];
    FILETIME LastDLLWriteTime;
    HMODULE AppLibrary;
    
    app_tick_ *AppTick_;
    
#if KENGINE_CONSOLE
    app_handle_command *AppHandleCommand;
#endif
    
#if KENGINE_HTTP
    app_handle_http_request *AppHandleHttpRequest;
#endif
    
#endif
    
} win32_state;

global win32_state GlobalWin32State;

platform_api Platform;

internal platform_memory_block *
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
    
    win32_memory_block *Win32Block = (win32_memory_block *)Win32VirtualAlloc(0, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Win32Block);
    Win32Block->PlatformBlock.Base = (u8 *)Win32Block + BaseOffset;
    Assert(Win32Block->PlatformBlock.Used == 0);
    Assert(Win32Block->PlatformBlock.Prev == 0);
    
    if(Flags & (PlatformMemoryBlockFlag_UnderflowCheck|PlatformMemoryBlockFlag_OverflowCheck))
    {
        DWORD OldProtect = 0;
        b32 IsProtected = Win32VirtualProtect((u8 *)Win32Block + ProtectOffset, PageSize, PAGE_NOACCESS, &OldProtect);
        Assert(IsProtected);
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
    
    LogDebug("Allocated %u bytes", Size);
    
    platform_memory_block *Result = &Win32Block->PlatformBlock;
    return Result;
}

internal void
Win32DeallocateMemory(platform_memory_block *Block)
{
    win32_memory_block *Win32Block = (win32_memory_block *)Block;
    
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Win32Block->Prev->Next = Win32Block->Next;
    Win32Block->Next->Prev = Win32Block->Prev;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
    
    LogDebug("Deallocated %u bytes", Block->Size);
    
    b32 IsFreed = Win32VirtualFree(Win32Block, 0, MEM_RELEASE);
    Assert(IsFreed);
}

internal platform_memory_stats
Win32GetMemoryStats()
{
    platform_memory_stats Result;
    ZeroStruct(Result);
    
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    win32_memory_block *Sentinel = &GlobalWin32State.MemorySentinel;
    for(win32_memory_block *Win32Block = Sentinel->Next;
        Win32Block != Sentinel;
        Win32Block = Win32Block->Next)
    {
        Assert(Win32Block->PlatformBlock.Size <= U32Max);
        
        ++Result.BlockCount;
        Result.TotalSize += Win32Block->PlatformBlock.Size;
        Result.TotalUsed += Win32Block->PlatformBlock.Used;
    }
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
    
    return Result;
}

internal b32
Win32DirectoryExists(string Path)
{
    b32 Result;
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    u32 Attributes = Win32GetFileAttributesA(CPath);
    Result = ((Attributes != INVALID_FILE_ATTRIBUTES) &&
              (Attributes & FILE_ATTRIBUTE_DIRECTORY));
    
    return Result;    
}

internal b32
Win32FileExists(string Path)
{
    b32 Result;
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    u32 Attributes = Win32GetFileAttributesA(CPath);
    
    Result = ((Attributes != INVALID_FILE_ATTRIBUTES) && 
              !(Attributes & FILE_ATTRIBUTE_DIRECTORY));
    
    return Result;
}

internal b32
Win32PermanentDeleteFile(string Path)
{
    b32 Result;
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    Result = Win32DeleteFileA(CPath);
    
    return Result;
}

internal void
Win32ConsoleOut(char *Format, ...)
{
    u8 Buffer[4096];
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Text = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    
#if KENGINE_CONSOLE
    HANDLE OutputHandle = Win32GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    DWORD NumberOfBytesWritten;
    Win32WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&NumberOfBytesWritten, 0);
    Assert(NumberOfBytesWritten == Text.Size);
#endif
    
#if KENGINE_INTERNAL
    Buffer[Text.Size] = '\0';
    Win32OutputDebugStringA((char *)Buffer);
#endif
}

internal FILETIME
Win32GetLastWriteTime(char *Filename)
{
    // TODO(kstandbridge): Change param to string?
    
    FILETIME LastWriteTime;
    ZeroStruct(LastWriteTime);
    
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(Win32GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }
    else
    {
        Win32LogError(String("GetFileAttributesEx"));
    }
    
    return LastWriteTime;
}

typedef struct win32_work_queue
{
    platform_work_queue PlatformQueue;
    u32 volatile CompletionGoal;
    u32 volatile CompletionCount;
    
    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;
    
    HANDLE SemaphoreHandle;
} win32_work_queue;

internal void
Win32AddWorkEntry(platform_work_queue *PlatformQueue, platform_work_queue_callback *Callback, void *Data)
{
    win32_work_queue *Win32Queue = (win32_work_queue *)PlatformQueue;
    u32 NewNextEntryToWrite = (Win32Queue->NextEntryToWrite + 1) % ArrayCount(Win32Queue->PlatformQueue.Entries);
    Assert(NewNextEntryToWrite != Win32Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Win32Queue->PlatformQueue.Entries + Win32Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Win32Queue->CompletionGoal;
    _WriteBarrier();
    Win32Queue->NextEntryToWrite = NewNextEntryToWrite;
    Win32ReleaseSemaphore(Win32Queue->SemaphoreHandle, 1, 0);
}

internal b32
Win32DoNextWorkQueueEntry(platform_work_queue *PlatformQueue)
{
    win32_work_queue *Win32Queue = (win32_work_queue *)PlatformQueue;
    
    b32 WeShouldSleep = false;
    
    u32 OriginalNextEntryToRead = Win32Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Win32Queue->PlatformQueue.Entries);
    if(OriginalNextEntryToRead != Win32Queue->NextEntryToWrite)
    {
        u32 Index = AtomicCompareExchangeU32(&Win32Queue->NextEntryToRead,
                                             NewNextEntryToRead,
                                             OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {        
            platform_work_queue_entry Entry = Win32Queue->PlatformQueue.Entries[Index];
            Entry.Callback(Entry.Data);
            AtomicIncrementU32(&Win32Queue->CompletionCount);
        }
    }
    else
    {
        WeShouldSleep = true;
    }
    
    return WeShouldSleep;
}

internal void
Win32CompleteAllWork(platform_work_queue *PlatformQueue)
{
    win32_work_queue *Win32Queue = (win32_work_queue *)PlatformQueue;
    
    while(Win32Queue->CompletionGoal != Win32Queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(PlatformQueue);
    }
    
    Win32Queue->CompletionGoal = 0;
    Win32Queue->CompletionCount = 0;
}

DWORD WINAPI
Win32WorkQueueThread(void *lpParameter)
{
    win32_work_queue *Win32Queue = (win32_work_queue *)lpParameter;
    
    u32 TestThreadId = GetThreadID();
    Assert(TestThreadId == Win32GetCurrentThreadId());
    
    for(;;)
    {
        if(Win32DoNextWorkQueueEntry((platform_work_queue *)Win32Queue))
        {
            Win32WaitForSingleObjectEx(Win32Queue->SemaphoreHandle, INFINITE, false);
        }
    }
}

internal platform_work_queue *
Win32MakeWorkQueue(memory_arena *Arena, u32 ThreadCount)
{
    win32_work_queue *Win32Queue = PushStruct(Arena, win32_work_queue);
    
    Win32Queue->CompletionGoal = 0;
    Win32Queue->CompletionCount = 0;
    
    Win32Queue->NextEntryToWrite = 0;
    Win32Queue->NextEntryToRead = 0;
    
    u32 InitialCount = 0;
    Win32Queue->SemaphoreHandle = Win32CreateSemaphoreExA(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
    {
        u32 ThreadId;
        HANDLE ThreadHandle = Win32CreateThread(0, 0, Win32WorkQueueThread, Win32Queue, 0, (LPDWORD)&ThreadId);
        Win32CloseHandle(ThreadHandle);
    }
    
    platform_work_queue *Result = (platform_work_queue *)Win32Queue;
    return Result;
}

inline f32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End, s64 PerfCountFrequency)
{
    f32 Result = ((f32)(End.QuadPart - Start.QuadPart) /
                  (f32)PerfCountFrequency);
    return Result;
}


inline LARGE_INTEGER
Win32GetWallClock()
{    
    LARGE_INTEGER Result;
    Win32QueryPerformanceCounter(&Result);
    return Result;
}

internal b32
Win32CreateDirectory(string Path)
{
    b32 Result = false;
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    SECURITY_ATTRIBUTES SecurityAttributes;
    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = 0;
    SecurityAttributes.bInheritHandle = true;
    
    switch(Win32SHCreateDirectoryExA(0, CPath, &SecurityAttributes))
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

internal b32
Win32DownloadToFile(string Url, string Path)
{
    b32 Result = false;
    
    char CUrl[MAX_PATH];
    StringToCString(Url, MAX_PATH, CUrl);
    
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    switch(Win32URLDownloadToFileA(0, CUrl, CPath, 0, 0))
    {
        case E_OUTOFMEMORY:
        {
            Assert(!"The buffer length is invalid, or there is insufficient memory to complete the operation.");
        } break;
        
        case INET_E_DOWNLOAD_FAILURE:
        {
            Assert(!"The specified resource or callback interface was invalid.");
        } break;
        
        case S_OK:
        {
            Result = true;
        } break;
        
        InvalidDefaultCase;
    }
    
    
    return Result;
}

internal s32
Win32SocketSendData(SOCKET Socket, void *Input, s32 InputLength)
{
    s32 Length;
    s32 Sum;
    u8 *Ptr = (u8 *)Input;
    
    for(Sum = 0; 
        Sum < InputLength;
        Sum += Length)
    {
        Length = Win32Send(Socket, (char *)&Ptr[Sum], InputLength - Sum, 0);
        if(Length <= 0)
        {
            return -1;
        }
    }
    
    return Sum;
}

internal b32
Win32SendEncryptedMessage(SOCKET Socket, u8 *Input, u32 InputLength, CtxtHandle *SSPICtxtHandle)
{
    b32 Result = false;;
    SecPkgContext_StreamSizes StreamSizes;
    
    SECURITY_STATUS SecurityStatus = Win32SecurityFunctionTable->QueryContextAttributes(SSPICtxtHandle, SECPKG_ATTR_STREAM_SIZES, &StreamSizes);
    if(SEC_E_OK == SecurityStatus)
    {
        char WriteBuffer[2048];
        
        // NOTE(kstandbridge): Put the message in the right place in the buffer
        Copy(InputLength, Input, WriteBuffer + StreamSizes.cbHeader);
        
        SecBuffer Buffers[4];
        // NOTE(kstandbridge): Stream header
        Buffers[0].pvBuffer = WriteBuffer;
        Buffers[0].cbBuffer = StreamSizes.cbHeader;
        Buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
        
        // NOTE(kstandbridge): Stream data
        Buffers[1].pvBuffer = WriteBuffer + StreamSizes.cbHeader;
        Buffers[1].cbBuffer = InputLength;
        Buffers[1].BufferType = SECBUFFER_DATA;
        
        // NOTE(kstandbridge): Stream trailer
        Buffers[2].pvBuffer = WriteBuffer + StreamSizes.cbHeader + InputLength;
        Buffers[2].cbBuffer = StreamSizes.cbTrailer;
        Buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
        
        // NOTE(kstandbridge): Emtpy buffer
        Buffers[3].pvBuffer = 0;
        Buffers[3].cbBuffer = 0;
        Buffers[3].BufferType = SECBUFFER_EMPTY;
        
        SecBufferDesc BufferDesc;
        BufferDesc.ulVersion = SECBUFFER_VERSION;
        BufferDesc.cBuffers = 4;
        BufferDesc.pBuffers = Buffers;
        
        // NOTE(kstandbridge): Encrypt
        SecurityStatus = Win32SecurityFunctionTable->EncryptMessage(SSPICtxtHandle, 0, &BufferDesc, 0);
        
        if(SEC_E_OK == SecurityStatus)
        {
            s32 NewLength = Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer;
            s32 BytesSent = Win32SocketSendData(Socket, WriteBuffer, NewLength);
            Result = (BytesSent > 0);
        }
    }
    
    return Result;
}

// TODO(kstandbridge): Move this?
#define TLS_MAX_BUFSIZ      32768

internal SECURITY_STATUS 
Win32SslClientHandshakeloop(SOCKET Socket, CredHandle *SSPICredHandle, CtxtHandle *SSPICtxtHandle)
{
    SECURITY_STATUS Result = SEC_I_CONTINUE_NEEDED;
    
    DWORD FlagsIn = 
        ISC_REQ_REPLAY_DETECT|
        ISC_REQ_CONFIDENTIALITY|
        ISC_RET_EXTENDED_ERROR|
        ISC_REQ_ALLOCATE_MEMORY|
        ISC_REQ_MANUAL_CRED_VALIDATION;
    
    char Buffer[TLS_MAX_BUFSIZ];
    DWORD BufferMaxLength = TLS_MAX_BUFSIZ;
    DWORD BufferLength = 0;
    s32 BytesRecieved;
    
    SecBuffer ExtraData;
    ZeroStruct(ExtraData);
    
    while((Result == SEC_I_CONTINUE_NEEDED) ||
          (Result == SEC_E_INCOMPLETE_MESSAGE) ||
          (Result == SEC_I_INCOMPLETE_CREDENTIALS))
    {
        if(Result == SEC_E_INCOMPLETE_MESSAGE)
        {
            // NOTE(kstandbridge): Recieve data from server
            BytesRecieved = Win32Recv(Socket, &Buffer[BufferLength], BufferMaxLength - BufferLength, 0);
            
            if(SOCKET_ERROR == BytesRecieved)
            {
                Result = SEC_E_INTERNAL_ERROR;
                Assert(!"Socket error");
                break;
            }
            else if(BytesRecieved == 0)
            {
                Result = SEC_E_INTERNAL_ERROR;
                Assert(!"Server disconnected");
                break;
            }
            
            BufferLength += BytesRecieved;
        }
        
        // NOTE(kstandbridge): Input data
        SecBuffer InputBuffers[2];
        InputBuffers[0].pvBuffer   = Buffer;
        InputBuffers[0].cbBuffer   = BufferLength;
        InputBuffers[0].BufferType = SECBUFFER_TOKEN;
        
        // NOTE(kstandbridge): Empty buffer
        InputBuffers[1].pvBuffer   = 0;
        InputBuffers[1].cbBuffer   = 0;
        InputBuffers[1].BufferType = SECBUFFER_VERSION;
        
        SecBufferDesc InputBufferDesc;
        InputBufferDesc.cBuffers      = 2;
        InputBufferDesc.pBuffers      = InputBuffers;
        InputBufferDesc.ulVersion     = SECBUFFER_VERSION;
        
        // NOTE(kstandbridge): Output from schannel
        SecBuffer OutputBuffers[1];
        OutputBuffers[0].pvBuffer   = 0;
        OutputBuffers[0].cbBuffer   = 0;
        OutputBuffers[0].BufferType = SECBUFFER_VERSION;
        
        SecBufferDesc OutputBufferDesc;
        OutputBufferDesc.cBuffers     = 1;
        OutputBufferDesc.pBuffers     = OutputBuffers;
        OutputBufferDesc.ulVersion    = SECBUFFER_VERSION;
        
        DWORD FlagsOut;
        Result = 
            Win32SecurityFunctionTable->InitializeSecurityContext(SSPICredHandle, SSPICtxtHandle, 0, FlagsIn, 0,
                                                                  SECURITY_NATIVE_DREP, &InputBufferDesc, 0, 0,
                                                                  &OutputBufferDesc, &FlagsOut, 0);
        
        // NOTE(kstandbridge): What have we got so far?
        if((Result == SEC_E_OK) ||
           (Result == SEC_I_CONTINUE_NEEDED) ||
           (FAILED(Result) && (FlagsOut & ISC_RET_EXTENDED_ERROR)))
        {
            // NOTE(kstandbridge): Response from server
            if((OutputBuffers[0].cbBuffer != 0) &&
               (OutputBuffers[0].pvBuffer))
            {
                Win32SocketSendData(Socket, OutputBuffers[0].pvBuffer, OutputBuffers[0].cbBuffer);
                Win32SecurityFunctionTable->FreeContextBuffer(OutputBuffers[0].pvBuffer);
                OutputBuffers[0].pvBuffer = 0;
            }
        }
        
        // NOTE(kstandbridge): Incomplete message, continue reading
        if(Result == SEC_E_INCOMPLETE_MESSAGE)
        {
            continue;
        }
        
        // NOTE(kstandbridge): Completed handsake
        if(Result == SEC_E_OK)
        {
            // NOTE(kstandbridge): The extra buffer could contain encrypted application protocol later stuff, we will decript it later with DecryptMessage
            if(InputBuffers[1].BufferType == SECBUFFER_EXTRA)
            {
                // TODO(kstandbridge): I've yet to hit this case, but we have data to deal with
                __debugbreak();
                
                char ExtraBuffer[TLS_MAX_BUFSIZ];
                ExtraData.pvBuffer = ExtraBuffer;
                
                memmove(ExtraData.pvBuffer,
                        &Buffer[(BufferLength - InputBuffers[1].cbBuffer)],
                        InputBuffers[1].cbBuffer);
                ExtraData.cbBuffer = InputBuffers[1].cbBuffer;
                ExtraData.BufferType = SECBUFFER_TOKEN;
            }
            else
            {
                // NOTE(kstandbridge): No extra data
                ExtraData.pvBuffer = 0;
                ExtraData.cbBuffer = 0;
                ExtraData.BufferType = SECBUFFER_EMPTY;
            }
            
            break;
        }
        
        // NOTE(kstandbridge): Some other error
        if(FAILED(Socket))
        {
            __debugbreak();
            Assert(!"Socket error");
            break;
        }
        
        if(InputBuffers[1].BufferType == SECBUFFER_EXTRA)
        {
            memmove(Buffer,
                    Buffer + (BufferLength - InputBuffers[1].cbBuffer),
                    InputBuffers[1].cbBuffer);
            
            BufferLength = InputBuffers[1].cbBuffer;
        }
        else
        {
            BufferLength = 0;
        }
        
    }
    
    if(FAILED(Socket))
    {
        // TODO(kstandbridge):  m_SecurityFunc.DeleteSecurityContext(phContext);
    }
    
    return Result;
}

internal b32
Win32RecieveDecryptedMessage(SOCKET Socket, CredHandle *SSPICredHandle, CtxtHandle *SSPICtxtHandle, u8 *OutBuffer, umm OutBufferLength)
{
    umm Result = 0;
    SecPkgContext_StreamSizes StreamSizes;
    SECURITY_STATUS SecurityStatus = Win32SecurityFunctionTable->QueryContextAttributes(SSPICtxtHandle, SECPKG_ATTR_STREAM_SIZES, &StreamSizes);
    
    SecBuffer DecryptBuffers[4];
    SecBufferDesc DecryptBufferDesc;
    
    if(SEC_E_OK == SecurityStatus)
    {    
        
        DWORD BufferMaxLength = StreamSizes.cbHeader + StreamSizes.cbMaximumMessage + StreamSizes.cbTrailer;
        
        Assert(BufferMaxLength < TLS_MAX_BUFSIZ);
        char Buffer[TLS_MAX_BUFSIZ];
        ZeroSize(TLS_MAX_BUFSIZ, Buffer);
        DWORD BufferLength = 0;
        s32 BytesRecieved;
        
        SecurityStatus = 0;
        
        // NOTE(kstandbridge): Read from server until we have decripted data or disconnect/session ends
        b32 Continue = true;
        do
        {
            // NOTE(kstandbridge): Is this the first read or last read was incomplete
            if(BufferLength == 0 || SecurityStatus == SEC_E_INCOMPLETE_MESSAGE)
            {
#if 0
                BytesRecieved = Win32Recv(Socket, Buffer + BufferLength, BufferMaxLength - BufferLength, 0);
#else
                BytesRecieved = Win32Recv(Socket, Buffer + BufferLength, 1, 0);
#endif
                if(BytesRecieved < 0)
                {
                    Assert(!"Socket error");
                    break;
                }
                else if(BytesRecieved == 0)
                {
                    // NOTE(kstandbridge): Server disconnected
                    if(BufferLength)
                    {
                        Assert(!"Disconnected while reciving data");
                        break;
                    }
                    // NOTE(kstandbridge): Session ended, we read all the data
                    break;
                } 
                else 
                {
                    BufferLength += BytesRecieved;
                }
            }
            
            // NOTE(kstandbridge): Try decript data
            DecryptBuffers[0].pvBuffer = Buffer;
            DecryptBuffers[0].cbBuffer = BufferLength;
            DecryptBuffers[0].BufferType = SECBUFFER_DATA;
            
            DecryptBuffers[1].BufferType = SECBUFFER_EMPTY;
            DecryptBuffers[2].BufferType = SECBUFFER_EMPTY;
            DecryptBuffers[3].BufferType = SECBUFFER_EMPTY;
            
            DecryptBufferDesc.ulVersion = SECBUFFER_VERSION;
            DecryptBufferDesc.cBuffers = 4;
            DecryptBufferDesc.pBuffers = DecryptBuffers;
            
            SecurityStatus = Win32SecurityFunctionTable->DecryptMessage(SSPICtxtHandle, &DecryptBufferDesc, 0, 0);
            
            if(SecurityStatus == SEC_E_INCOMPLETE_MESSAGE)
            {
                continue;
            }
            
            if(SecurityStatus == SEC_I_CONTEXT_EXPIRED)
            {
                Assert(!"Context expired");
                break;
            }
            
            if((SecurityStatus != SEC_E_OK) &&
               (SecurityStatus != SEC_I_RENEGOTIATE) &&
               (SecurityStatus != SEC_I_CONTEXT_EXPIRED))
            {
                Assert(!"Decryption error");
                break;
            }
            
            SecBuffer *pDataBuffer = 0;
            SecBuffer *pExtraBuffer = 0;
            for(u32 Index = 0;
                Index < 4;
                ++Index)
            {
                if((pDataBuffer == 0) &&
                   DecryptBuffers[Index].BufferType == SECBUFFER_DATA)
                {
                    pDataBuffer = DecryptBuffers + Index;
                }
                
                if((pExtraBuffer == 0) &&
                   DecryptBuffers[Index].BufferType == SECBUFFER_EXTRA)
                {
                    pExtraBuffer = DecryptBuffers + Index;
                }
            }
            
            if(pDataBuffer)
            {
                if((Result + pDataBuffer->cbBuffer) > BufferMaxLength)
                {
                    Assert(!"Buffer not big enough?");
                }
                Copy(pDataBuffer->cbBuffer, pDataBuffer->pvBuffer, OutBuffer + Result);
                
                Result += pDataBuffer->cbBuffer;
                
                
                
            }
            
            if(pExtraBuffer)
            {
                MoveMemory(Buffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
                
                BufferLength = pExtraBuffer->cbBuffer;
                continue;
            }
            else
            {
                BufferLength = 0;
                Continue = false;
            }
            
            if(SecurityStatus == SEC_I_RENEGOTIATE)
            {
                SecurityStatus = Win32SslClientHandshakeloop(Socket, SSPICredHandle, SSPICtxtHandle);
                
                if(SecurityStatus != SEC_E_OK)
                {
                    Assert(!"Failed to renegotiate");
                    break;
                }
                
                if(pExtraBuffer)
                {
                    MoveMemory(Buffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
                    BufferLength = pExtraBuffer->cbBuffer;
                }
            }
            
        } while(Continue);
    }
    
    return Result;
}

internal u32
Win32SslSocketConnect(string Hostname, string Port, CredHandle *SSPICredHandle, CtxtHandle *SSPICtxtHandle)
{
    SOCKET Result = 0;
    
    char CHostname[MAX_PATH];
    StringToCString(Hostname, MAX_PATH, CHostname);
    
    char CPort[MAX_PATH];
    StringToCString(Port, MAX_PATH, CPort);
    
    struct addrinfo Hints;
    ZeroStruct(Hints);
    Hints.ai_family = AF_INET; // NOTE(kstandbridge): IPv4
    Hints.ai_socktype = SOCK_STREAM; // NOTE(kstandbridge): TCP
    Hints.ai_protocol = IPPROTO_TCP;
    
    struct addrinfo *AddressInfos;
    
    if(Win32GetAddrInfo(CHostname, CPort, &Hints, &AddressInfos) == 0)
    {
        for(struct addrinfo *AddressInfo = AddressInfos;
            AddressInfo != 0;
            AddressInfo = AddressInfo->ai_next)
        {
            if(AddressInfo->ai_family == AF_INET) // NOTE(kstandbridge): IPv4
            {
                Result = Win32Socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
                if(Result != INVALID_SOCKET)
                {
                    // NOTE(kstandbridge): Ensure we can reuse same port later
                    s32 On = 1;
                    Win32SetSockOpt(Result, SOL_SOCKET, SO_REUSEADDR, (char *)&On, sizeof(On));
                    
                    // NOTE(kstandbridge): Initialize new TLS session
                    ALG_ID Algs[2] = {0};
                    Algs[0]                            = CALG_RSA_KEYX;
                    SCHANNEL_CRED SChannelCred = {0};
                    SChannelCred.dwVersion             = SCHANNEL_CRED_VERSION;
                    //SChannelCred.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
                    SChannelCred.cSupportedAlgs        = 1;
                    //SChannelCred.palgSupportedAlgs     = Algs;
                    SChannelCred.dwFlags              |= SCH_CRED_NO_DEFAULT_CREDS;
                    SChannelCred.dwFlags              |= SCH_CRED_MANUAL_CRED_VALIDATION;
                    
                    SECURITY_STATUS SecurityStatus = 
                        Win32SecurityFunctionTable->AcquireCredentialsHandleA(0,UNISP_NAME_A, SECPKG_CRED_OUTBOUND,
                                                                              0, &SChannelCred, 0, 0, SSPICredHandle, 0);
                    
                    if(SecurityStatus == SEC_E_OK)
                    {
                        if(SOCKET_ERROR != Win32Connect(Result, AddressInfo->ai_addr, (s32)AddressInfo->ai_addrlen))
                        {
                            DWORD FlagsIn = 
                                ISC_REQ_REPLAY_DETECT|
                                ISC_REQ_CONFIDENTIALITY|
                                ISC_RET_EXTENDED_ERROR|
                                ISC_REQ_ALLOCATE_MEMORY|
                                ISC_REQ_MANUAL_CRED_VALIDATION;
                            
                            SecBuffer Buffers[1];
                            Buffers[0].pvBuffer = 0;
                            Buffers[0].cbBuffer = 0;
                            Buffers[0].BufferType = SECBUFFER_TOKEN;
                            
                            SecBufferDesc BufferDesc;
                            BufferDesc.cBuffers = 1;
                            BufferDesc.pBuffers = Buffers;
                            BufferDesc.ulVersion = SECBUFFER_VERSION;
                            
                            DWORD FlagsOut;
                            SecurityStatus = 
                                Win32SecurityFunctionTable->InitializeSecurityContextA(SSPICredHandle, 0, 0, FlagsIn, 0,
                                                                                       SECURITY_NATIVE_DREP, 0, 0,
                                                                                       SSPICtxtHandle, &BufferDesc, &FlagsOut, 0);
                            
                            if(SEC_I_CONTINUE_NEEDED == SecurityStatus)
                            {
                                if(Buffers[0].cbBuffer != 0)
                                {
                                    Win32SocketSendData(Result, Buffers[0].pvBuffer, Buffers[0].cbBuffer);
                                    
                                    SecurityStatus = Win32SecurityFunctionTable->FreeContextBuffer(Buffers[0].pvBuffer);
                                    
                                    // TODO(kstandbridge): Why no delete?
                                    //Win32SecurityFunctionTable->DeleteSecurityContext(&SSPICredHandle);
                                    
                                    Buffers[0].pvBuffer = 0;
                                }
                                
                                if(SEC_E_OK == SecurityStatus)
                                {
                                    SecurityStatus = Win32SslClientHandshakeloop(Result, SSPICredHandle, SSPICtxtHandle);
                                    
                                    PCCERT_CONTEXT RemoteCertContext;
                                    SecurityStatus = Win32SecurityFunctionTable->QueryContextAttributes(SSPICtxtHandle,
                                                                                                        SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                                                                                        (PVOID)&RemoteCertContext);
                                    
                                    if(SecurityStatus == SEC_E_OK)
                                    {
                                        // TODO(kstandbridge): Win32SslVerifyCertificate
                                        //SecurityStatus = Win32SslVerifyCertificate();
                                        if(SecurityStatus != SEC_E_OK)
                                        {
                                            // TODO(kstandbridge): CertFreeCertificateContext
                                            //CertFreeCertificateContext(RemoteCertContext);
                                        }
                                    }
                                    else
                                    {
                                        Assert(!"QueryContextAttributes error?");
                                    }
                                }
                                else
                                {
                                    Assert(!"Handshake failed sending data");
                                }
                            }
                            else
                            {
                                Assert(!"Handshake failed initalizing security context");
                            }
                            
                        }
                        else
                        {
                            s32 ErrorCode = Win32WSAGetLastError();
                            Assert(!"Connection error");
                        }
                    }
                    else
                    {
                        s32 ErrorCode = Win32WSAGetLastError();
                        Assert(!"Acquire credentials handle failed");
                    }
                }
                else
                {
                    s32 ErrorCode = Win32WSAGetLastError();
                    Assert(!"Could not create socket");
                }
                
                break;
            }
        }
    }
    else
    {
        s32 ErrorCode = Win32WSAGetLastError();
        Assert(!"Hostname look up failed");
    }
    
    Win32FreeAddrInfo(AddressInfos);
    
    return Result;
}

internal u32
Win32SocketConnect(string Hostname, string Port)
{
    SOCKET Result = 0;
    
    char CHostname[MAX_PATH];
    StringToCString(Hostname, MAX_PATH, CHostname);
    char CPort[MAX_PATH];
    StringToCString(Port, MAX_PATH, CPort);
    
    struct addrinfo *AddressInfo;
    struct addrinfo Hints;
    ZeroStruct(Hints);
    Hints.ai_family = AF_INET; // NOTE(kstandbridge): IPv4
    Hints.ai_socktype = SOCK_STREAM; // NOTE(kstandbridge): TCP
    Hints.ai_protocol = IPPROTO_TCP;
    
    if(Win32GetAddrInfo(CHostname, CPort, &Hints, &AddressInfo) == 0)
    {
        Result = Win32Socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
        if(Result != INVALID_SOCKET)
        {
            if(SOCKET_ERROR == Win32Connect(Result, AddressInfo->ai_addr, (s32)AddressInfo->ai_addrlen))
            {
                s32 ErrorCode = Win32WSAGetLastError();
                Assert(!"Connection error");
            }
        }
        else
        {
            s32 ErrorCode = Win32WSAGetLastError();
            Assert(!"Could not create socket");
        }
    }
    else
    {
        s32 ErrorCode = Win32WSAGetLastError();
        Assert(!"Hostname look up failed");
    }
    
    Win32FreeAddrInfo(AddressInfo);
    
    return Result;
}

internal void
Win32SocketClose(SOCKET Socket)
{
    Win32Shutdown(Socket, SD_BOTH);
    Win32CloseSocket(Socket);
}

internal u64
Win32GetTimestamp(s16 Year, s16 Month, s16 Day, s16 Hour, s16 Minute, s16 Second)
{
    u64 Result = 0;
    
    SYSTEMTIME Date;
    Date.wYear = Year;
    Date.wMonth = Month;
    Date.wDay = Day;
    Date.wHour = Hour;
    Date.wMinute = Minute;
    Date.wSecond = Second;
    
    FILETIME Time;
    Win32SystemTimeToFileTime(&Date, &Time);
    
    ULARGE_INTEGER Large;
    Large.LowPart = Time.dwLowDateTime;
    Large.HighPart = Time.dwHighDateTime;
    
    Result = Large.QuadPart;
    
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
    
    HANDLE Snapshot = Win32CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    PROCESSENTRY32 Entry;
    ZeroStruct(Entry);
    Entry.dwSize = sizeof(PROCESSENTRY32);
    b32 ProcessFound = Win32Process32First(Snapshot, &Entry);
    while(ProcessFound)
    {
        if(StringsAreEqual(CStringToString(Entry.szExeFile), Name))
        {
            HANDLE Process = Win32OpenProcess(PROCESS_TERMINATE, false, Entry.th32ProcessID);
            if(Process != 0)
            {
                Result = true;
                if(Op == IterateProcesses_RequestClose)
                {
                    u8 CPipe[MAX_PATH];
                    string Pipe = FormatStringToBuffer(CPipe, sizeof(CPipe),  "\\\\.\\pipe\\close-request-pipe-%d", Entry.th32ProcessID);
                    CPipe[Pipe.Size] = '\0';
                    HANDLE PipeHandle = Win32CreateFileA((char *)CPipe, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
                    
                    // NOTE(kstandbridge): Give it 3 seconds to close
                    Win32Sleep(3000); 
                    Win32CloseHandle(PipeHandle);
                    
                    // NOTE(kstandbridge): Kill it anyway
                    Win32TerminateProcess(Process, 0);
                }
                else if(Op == IterateProcesses_Terminate)
                {                    
                    Win32TerminateProcess(Process, 0);
                }
                else
                {
                    Assert(Op == IterateProcesses_None);
                }
                Win32CloseHandle(Process);
                break;
            }
            else
            {
                Assert(!"Lack PROCESS_TERMINATE access to Firebird.exe");
            }
        }
        ProcessFound = Win32Process32Next(Snapshot, &Entry);
    }
    Win32CloseHandle(Snapshot);
    
    return Result;
}

internal b32
Win32RequestCloseProcess(string FileName)
{
    b32 Result = Win32IterateProcesses_(FileName, IterateProcesses_RequestClose);
    return Result;
}

internal b32
Win32KillProcess(string Name)
{
    b32 Result = Win32IterateProcesses_(Name, IterateProcesses_Terminate);
    return Result;
}

internal b32
Win32CheckForProcess(string Name)
{
    b32 Result = Win32IterateProcesses_(Name, IterateProcesses_None);
    return Result;
}

// TODO(kstandbridge): Pipe outpack to callback?
internal void
Win32ExecuteProcessWithOutput(string Path, string Args, string WorkingDirectory)
{
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
    if(Win32CreatePipe(&InPipeRead, &InPipeWrite, &SecurityAttributes, 0))
    {
        HANDLE OutPipeRead;
        HANDLE OutPipeWrite;
        if(Win32CreatePipe(&OutPipeRead, &OutPipeWrite, &SecurityAttributes, 0))
        {
            STARTUPINFOA StartupInfo;
            ZeroStruct(StartupInfo);
            StartupInfo.cb = sizeof(STARTUPINFOA);
            StartupInfo.dwFlags = STARTF_USESTDHANDLES;
            StartupInfo.hStdError = OutPipeWrite;
            StartupInfo.hStdOutput = OutPipeWrite;
            StartupInfo.hStdInput = InPipeRead;
            
            PROCESS_INFORMATION ProcessInformation;
            ZeroStruct(ProcessInformation);
            if(Win32CreateProcessA(CPath, CArgs, &SecurityAttributes, 0, true, DETACHED_PROCESS, 
                                   0, CWorkingDirectory, &StartupInfo, &ProcessInformation))
            {
#define BUFFER_SIZE 1024
                Win32CloseHandle(OutPipeWrite);
                Win32CloseHandle(InPipeRead);
                
                char ReadingBuffer[BUFFER_SIZE];
                DWORD ReadIndex = 0;
                b32 IsReading = Win32ReadFile(OutPipeRead, ReadingBuffer, BUFFER_SIZE, &ReadIndex, 0);
                while(IsReading)
                {
                    ReadingBuffer[ReadIndex] = '\0';
                    u32 OutputIndex = 0;
                    char OutputBuffer[BUFFER_SIZE];
                    for(u32 Index = 0;
                        Index < ReadIndex;
                        ++Index)
                    {
                        if(ReadingBuffer[Index] == '\0')
                        {
                            OutputBuffer[OutputIndex] = '\0';
                            break;
                        }
                        if(ReadingBuffer[Index] == '\r')
                        {
                            continue;
                        }
                        if(ReadingBuffer[Index] == '\n')
                        {
                            OutputBuffer[OutputIndex] = '\0';
                            if(OutputIndex > 0)
                            {
                                // TODO(kstandbridge): Pass string to callback?
                                Win32OutputDebugStringA(OutputBuffer);
                                Win32OutputDebugStringA("\n");
                            }
                            OutputIndex = 0;
                        }
                        else
                        {
                            OutputBuffer[OutputIndex++] = ReadingBuffer[Index];
                        }
                    }
                    IsReading = Win32ReadFile(OutPipeRead, ReadingBuffer, BUFFER_SIZE, &ReadIndex, 0);
                }
            }
            else
            {
                // TODO(kstandbridge): Return the exit code?
                Win32LogError(String("CreateProcessA"));
            }
            
            Win32CloseHandle(OutPipeRead);
            
            DWORD ExitCode = 0;
            Win32GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode);
        }
        else
        {
            Win32LogError(String("CreatePipe"));
        }
        
        Win32CloseHandle(InPipeWrite);
    }
    else
    {
        Win32LogError(String("CreatePipe"));
    }
}

internal void
Win32ExecuteProcess(string Path, string Args, string WorkingDirectory)
{
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    char CArgs[MAX_PATH];
    StringToCString(Args, MAX_PATH, CArgs);
    
    char CWorkingDirectory[MAX_PATH];
    StringToCString(WorkingDirectory, MAX_PATH, CWorkingDirectory);
    
    Win32ShellExecuteA(0, "open", CPath, CArgs, CWorkingDirectory, 0);
}

internal void
Win32UnzipToDirectory(string SourceZip, string DestFolder)
{
    if(!Win32DirectoryExists(DestFolder))
    {
        Win32CreateDirectory(DestFolder);
    }
    
    wchar_t CSourceZip[MAX_PATH];
    Win32MultiByteToWideChar(CP_UTF8, 0, (char *)SourceZip.Data, SourceZip.Size, CSourceZip, MAX_PATH);
    CSourceZip[SourceZip.Size + 0] = '\0';
    CSourceZip[SourceZip.Size + 1] = '\0';
    
    wchar_t CDestFolder[MAX_PATH];
    Win32MultiByteToWideChar(CP_UTF8, 0, (char *)DestFolder.Data, DestFolder.Size, CDestFolder, MAX_PATH);
    CDestFolder[DestFolder.Size + 0] = '\0';
    CDestFolder[DestFolder.Size + 1] = '\0';
    
    HRESULT HResult;
    IShellDispatch *ShellDispatch;
    Folder *ToFolder = 0;
    VARIANT OutputDirectory, SourceFile, ProgressDialog;
    
    Win32CoInitialize(0);
    
    HResult = Win32CoCreateInstance(&CLSID_Shell, 0, CLSCTX_INPROC_SERVER, &IID_IShellDispatch, (void **)&ShellDispatch);
    
    if (SUCCEEDED(HResult))
    {
        Folder *FromFolder = 0;
        
        Win32VariantInit(&SourceFile);
        SourceFile.vt = VT_BSTR;
        SourceFile.bstrVal = CSourceZip;
        
        Win32VariantInit(&OutputDirectory);
        OutputDirectory.vt = VT_BSTR;
        OutputDirectory.bstrVal = CDestFolder;
        ShellDispatch->lpVtbl->NameSpace(ShellDispatch, OutputDirectory, &ToFolder);
        
        ShellDispatch->lpVtbl->NameSpace(ShellDispatch, SourceFile, &FromFolder);
        FolderItems *FolderItems = 0;
        FromFolder->lpVtbl->Items(FromFolder, &FolderItems);
        
        Win32VariantInit(&ProgressDialog);
        ProgressDialog.vt = VT_I4;
        ProgressDialog.lVal = FOF_NO_UI;
        
        VARIANT ItemsToBeCopied;
        Win32VariantInit(&ItemsToBeCopied);
        ItemsToBeCopied.vt = VT_DISPATCH;
        ItemsToBeCopied.pdispVal = (IDispatch *)FolderItems;
        
        HResult = ToFolder->lpVtbl->CopyHere(ToFolder, ItemsToBeCopied, ProgressDialog);
        
        // NOTE(kstandbridge): As much as I hate sleep, CopyHere creates a separate thread
        // which may not finish if this thread exist before completion. So 1000ms sleep.
        Win32Sleep(1000);
        
        FromFolder->lpVtbl->Release(FromFolder);
        ToFolder->lpVtbl->Release(ToFolder);
    }
    Win32CoUninitialize();
}

internal b32
Win32WriteTextToFile(string Text, string FilePath)
{
    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);
    HANDLE FileHandle = Win32CreateFileA(CFilePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    DWORD BytesWritten = 0;
    Win32WriteFile(FileHandle, Text.Data, (DWORD)Text.Size, &BytesWritten, 0);
    
    b32 Result = (Text.Size == BytesWritten);
    Assert(Result);
    
    Win32CloseHandle(FileHandle);
    
    return Result;
}

internal void
Win32ZipFolder(string SourceFolderPath, string DestPath)
{
    // NOTE(kstandbridge): No Win32 API to create zip file, so lets build our own!!!
    {
        u8 Buffer[] = {80, 75, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        umm BufferSize = sizeof(Buffer);
        Win32WriteTextToFile(String_(BufferSize, Buffer), DestPath);
    }
    
    wchar_t CDestZip[MAX_PATH];
    Win32MultiByteToWideChar(CP_ACP, 0, (char *)DestPath.Data, DestPath.Size, CDestZip, MAX_PATH);
    CDestZip[DestPath.Size + 0] = '\0';
    CDestZip[DestPath.Size + 1] = '\0';
    
    wchar_t CSourceFolder[MAX_PATH];
    Win32MultiByteToWideChar(CP_ACP, 0, (char *)SourceFolderPath.Data, SourceFolderPath.Size, CSourceFolder, MAX_PATH);
    CSourceFolder[SourceFolderPath.Size + 0] = '\0';
    CSourceFolder[SourceFolderPath.Size + 1] = '\0';
    
    HRESULT HResult;
    IShellDispatch *ShellDispatch;
    Folder *ToFolder = 0;
    VARIANT DestZip, SourceFolder, ProgressDialog;
    
    Win32CoInitialize(0);
    
    HResult = Win32CoCreateInstance(&CLSID_Shell, 0, CLSCTX_INPROC_SERVER, &IID_IShellDispatch, (void **)&ShellDispatch);
    
    if (SUCCEEDED(HResult))
    {
        
        Win32VariantInit(&DestZip);
        DestZip.vt = VT_BSTR;
        DestZip.bstrVal = CDestZip;
        HResult = ShellDispatch->lpVtbl->NameSpace(ShellDispatch, DestZip, &ToFolder);
        
        if(SUCCEEDED(HResult))
        {
            
            Win32VariantInit(&SourceFolder);
            SourceFolder.vt = VT_BSTR;
            SourceFolder.bstrVal = CSourceFolder;
            
            Folder *FromFolder = 0;
            ShellDispatch->lpVtbl->NameSpace(ShellDispatch, SourceFolder, &FromFolder);
            FolderItems *FolderItems = 0;
            FromFolder->lpVtbl->Items(FromFolder, &FolderItems);
            
            Win32VariantInit(&ProgressDialog);
            ProgressDialog.vt = VT_I4;
            ProgressDialog.lVal = FOF_NO_UI;
            
            VARIANT ItemsToBeCopied;
            Win32VariantInit(&ItemsToBeCopied);
            ItemsToBeCopied.vt = VT_DISPATCH;
            ItemsToBeCopied.pdispVal = (IDispatch *)FolderItems;
            
            HResult = ToFolder->lpVtbl->CopyHere(ToFolder, ItemsToBeCopied, ProgressDialog);
            
            // NOTE(kstandbridge): As much as I hate sleep, CopyHere creates a separate thread
            // which may not finish if this thread exist before completion. So 1000ms sleep.
            Win32Sleep(1000);
            
            ToFolder->lpVtbl->Release(ToFolder);
            FromFolder->lpVtbl->Release(FromFolder);
        }
        
    }
    Win32CoUninitialize();
}

global HINTERNET GlobalWebSession;
inline void
WebSessionInit()
{
    if(GlobalWebSession == 0)
    {
        GlobalWebSession = Win32InternetOpenA("Default_User_Agent", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
        
        // TODO(kstandbridge): Win32InternetCloseHandle(WebConnect);
        // But not really, because this will be closed automatically when the application closes
    }
    
}

internal string
Win32ReadEntireFile(memory_arena *Arena, string FilePath)
{
    string Result;
    ZeroStruct(Result);
    
    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);
    
    HANDLE FileHandle = Win32CreateFileA(CFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        b32 ReadResult = Win32GetFileSizeEx(FileHandle, &FileSize);
        Assert(ReadResult);
        if(ReadResult)
        {    
            Result.Size = FileSize.QuadPart;
            Result.Data = PushSize(Arena, Result.Size);
            Assert(Result.Data);
            
            if(Result.Data)
            {
                u32 BytesRead;
                ReadResult = Win32ReadFile(FileHandle, Result.Data, (u32)Result.Size, (LPDWORD)&BytesRead, 0);
                Assert(ReadResult);
                Assert(BytesRead == Result.Size);
            }
        }
        
        Win32CloseHandle(FileHandle);
    }
    
    return Result;
}

inline string
Win32ReadInternetResponse(memory_arena *Arena, HINTERNET FileHandle)
{
    string Result;
    ZeroStruct(Result);
    
    DWORD TotalBytesRead = 0;
    DWORD ContentLength = 0;
    DWORD ContentLengthSize = sizeof(ContentLengthSize);
    
    char *TempFilePath = "temp.dat";
    u8 SaveBuffer_[4096];
    b32 SaveToHandle = false;
    HANDLE SaveHandle = 0;
    u8 *SaveBuffer = SaveBuffer_;
    
    if(!Win32HttpQueryInfoA(FileHandle, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, (LPVOID)&ContentLength, &ContentLengthSize, 0))
    {
        DWORD ErrorCode = Win32GetLastError();
        if(ErrorCode != ERROR_HTTP_HEADER_NOT_FOUND)
        {
            Win32LogError_(String("HttpQueryInfoA"), ErrorCode);
        }
    }
    
    
    
    if(ContentLength == 0)
    {
        SaveToHandle = true;
        SaveHandle = Win32CreateFileA(TempFilePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        Assert(SaveHandle != INVALID_HANDLE_VALUE);
    }
    else
    {
        Result.Size = ContentLength;
        Result.Data = PushSize_(Arena, ContentLength, DefaultArenaPushParams());
        SaveBuffer = Result.Data;
    }
    
    DWORD CurrentBytesRead;
    do
    {
        if(Win32InternetReadFile(FileHandle, SaveBuffer, sizeof(SaveBuffer_), &CurrentBytesRead))
        {
            if (CurrentBytesRead == 0)
            {
                break;
            }
            TotalBytesRead += CurrentBytesRead;
        }
        else
        {
            DWORD ErrorCode = Win32GetLastError();
            if (ErrorCode != ERROR_INSUFFICIENT_BUFFER)
            {
                Win32LogError_(String("InternetReadFile"), ErrorCode);
            }
            else
            {
                LogError("InternetReadFile failed due to insufficient buffer");
            }
            break;
        }
        
        if(SaveToHandle)
        {
            DWORD BytesWritten;
            Win32WriteFile(SaveHandle, SaveBuffer, CurrentBytesRead, &BytesWritten, 0);
            Assert(CurrentBytesRead == BytesWritten);
        }
        else
        {
            SaveBuffer += TotalBytesRead;
        }
    }
    while (true);
    
    if(SaveToHandle)
    {
        Win32CloseHandle(SaveHandle);
        Result = Win32ReadEntireFile(Arena, String("temp.dat"));
        Win32DeleteFileA(TempFilePath); 
    }
    else
    {
        if(TotalBytesRead != ContentLength)
        {
            LogError("Was expecting to read %u bytes but got back %u", ContentLength, TotalBytesRead);
        }
    }
    
    return Result;
}

#if 0
internal string
Win32SendHttpRequest____(memory_arena *Arena, string Host, u32 Port, string Endpoint, http_verb_type Verb, string Payload, 
                         string Headers, string Username, string Password)
{
    string Result;
    ZeroStruct(Result);
    
    // TODO(kstandbridge): persistent session?
    HINTERNET WebSession = Win32InternetOpenA("Default_User_Agent", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
    
    char CHost_[2048];
    StringToCString(Host, sizeof(CHost_), CHost_);
    
    INTERNET_PORT InternetPort = 0;
    if(Port == 0)
    {
        InternetPort = INTERNET_DEFAULT_HTTP_PORT;
    }
    DWORD Flags = 0;
    
    char *CHost = CHost_;
    if(StringBeginsWith(String("https"), Host))
    {
        CHost += 8;
        if(Port == 0)
        {
            Port = INTERNET_DEFAULT_HTTPS_PORT;
        }
        Flags = INTERNET_FLAG_SECURE;
    }
    else if(StringBeginsWith(String("http"), Host))
    {
        CHost += 7;
    }
    
    char CEndpoint[2048];
    StringToCString(Endpoint, sizeof(CEndpoint), CEndpoint);
    
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
    
    char CHeaders_[256];
    char *CHeaders = 0;
    if(Headers.Data && Headers.Data[0] != '\0')
    {
        StringToCString(Headers, sizeof(CHeaders_), CHeaders_);
        CHeaders = CHeaders_;
    }
    
    char CUsername_[256];
    char *CUsername = 0;
    if(Username.Data && Username.Data[0] != '\0')
    {
        StringToCString(Username, sizeof(CUsername_), CUsername_);
        CUsername = CUsername_;
    }
    
    char CPassword_[256];
    char *CPassword = 0;
    if(Password.Data && Password.Data[0] != '\0')
    {
        StringToCString(Password, sizeof(CPassword_), CPassword_);
        CPassword = CPassword_;
    }
    
    HINTERNET WebConnect = Win32InternetConnectA(WebSession, CHost, Port, CUsername, CPassword,
                                                 INTERNET_SERVICE_HTTP, 0, 0);
    if(WebConnect)
    {
        HINTERNET WebRequest = Win32HttpOpenRequestA(WebConnect, CVerb, CEndpoint, 0, 0, 0, Flags, 0);
        
        if(CHeaders)
        {
            Win32HttpAddRequestHeadersA(WebRequest, CHeaders, (DWORD) -1, HTTP_ADDREQ_FLAG_ADD);
        }
        
        
        if(Win32HttpSendRequestA(WebRequest, 0, 0, Payload.Data, Payload.Size))
        {
            
#if KENGINE_INTERNAL
            DWORD Status = 404;
            DWORD StatusSize = sizeof(StatusSize);
            Win32HttpQueryInfoA(WebRequest, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, (LPVOID)&Status, &StatusSize, 0);
#endif
            
            Result = Win32ReadInternetResponse(Arena, WebRequest);
        }
        else
        {
            Assert(!"Unable to send request");
        }
        
        Win32HttpEndRequestA(WebRequest, 0, 0, 0);
        Win32InternetCloseHandle(WebConnect);
    }
    else
    {
        Assert(!"Unable to connect");
    }
    
    Win32InternetCloseHandle(WebSession);
    
    return Result;
}
#endif

#if 0
internal umm
Win32GetInternetData(u8 *Buffer, umm BufferMaxSize, string Url)
{
    umm Result = 0;
    
    WebSessionInit();
    
    char CUrl[2048];
    StringToCString(Url, sizeof(CUrl), CUrl);
    
    HINTERNET WebAddress = Win32InternetOpenUrlA(GlobalWebSession, CUrl, 0, 0, 0, 0);
    
    if (WebAddress)
    {
        Result = Win32ReadInternetResponse(WebAddress, Buffer, BufferMaxSize);
        
        Win32InternetCloseHandle(WebAddress);
    }
    else
    {
        Assert(!"Failed to open Url");
    }
    
    
    return Result;
}
#endif

internal string
Win32UploadFileToInternet(memory_arena *Arena, string Host, string Endpoint, char *Verb, string File, char *Username, char *Password)
{
    string Result;
    ZeroStruct(Result);
    
    char CFile[MAX_PATH];
    StringToCString(File, MAX_PATH, CFile);
    
    HANDLE FileHandle = Win32CreateFileA(CFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER LFileSize;
        Win32GetFileSizeEx(FileHandle, &LFileSize);
        umm FileSize = LFileSize.QuadPart;
        
        WebSessionInit();
        
        char CHost_[2048];
        StringToCString(Host, sizeof(CHost_), CHost_);
        
        char CEndpoint[2048];
        StringToCString(Endpoint, sizeof(CEndpoint), CEndpoint);
        
        INTERNET_PORT Port = INTERNET_DEFAULT_HTTP_PORT;
        DWORD Flags = 0;
        
        char *CHost = CHost_;
        if(StringBeginsWith(String("https"), Host))
        {
            CHost += 8;
            Port = INTERNET_DEFAULT_HTTPS_PORT;
            Flags = INTERNET_FLAG_SECURE;
        }
        else if(StringBeginsWith(String("http"), Host))
        {
            CHost += 6;
        }
        
        HINTERNET WebConnect = Win32InternetConnectA(GlobalWebSession, CHost, Port, Username, Password,
                                                     INTERNET_SERVICE_HTTP, 0, 0);
        if(WebConnect)
        {
            HINTERNET WebRequest = Win32HttpOpenRequestA(WebConnect, Verb, CEndpoint, 0, 0, 0, Flags, 0);
            if(WebRequest)
            {
                // NOTE(kstandbridge): HttpSendRequestEx will return 401 initially asking for creds, so just EndRequest and start again
                if((Username != 0) &&
                   (Password != 0))
                {
                    if(Win32HttpSendRequestExA(WebRequest, 0, 0, 0, 0))
                    {
                        Win32HttpEndRequestA(WebRequest, 0, 0, 0);
                        DWORD StatusCode = 0;
                        DWORD StatusCodeLength = sizeof(StatusCode);
                        Win32HttpQueryInfoA(WebRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &StatusCode, &StatusCodeLength, 0);
                        Assert(StatusCode == 401);
                    }
                }
                
                u8 HeaderBuffer[256];
                string Header = FormatStringToBuffer(HeaderBuffer, sizeof(HeaderBuffer), "Content-Length: %u\r\n", FileSize);
                HeaderBuffer[Header.Size] = '\0';
                Win32HttpAddRequestHeadersA(WebRequest, (char *)HeaderBuffer, 
                                            (DWORD)-1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
                
                if(Win32HttpSendRequestExA(WebRequest, 0, 0, HSR_INITIATE, 0))
                {                
                    umm Offset = 0;
                    OVERLAPPED Overlapped;
                    ZeroStruct(Overlapped);
                    u8 Buffer[1024];
                    umm BufferSize = 1024;
                    do
                    {
                        Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
                        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);
                        
                        DWORD BytesRead = 0;
                        if(Win32ReadFile(FileHandle, Buffer, BufferSize, &BytesRead, &Overlapped))
                        {
                            DWORD BytesSent = 0;
                            DWORD TotalBytesSend = 0;
                            do
                            {
                                if(!Win32InternetWriteFile(WebRequest, Buffer + TotalBytesSend, BytesRead, &BytesSent))
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
                    
                    Win32HttpEndRequestA(WebRequest, 0, 0, 0);
                    
                    Result = Win32ReadInternetResponse(Arena, WebRequest);
                }
                else
                {
                    Assert(!"Unable to send request");
                }
            }
            else
            {
                Assert(!"Unable to create request");
            }
        }
        Win32InternetCloseHandle(WebConnect);
        Win32CloseHandle(FileHandle);
    }
    else
    {
        Assert(!"Unable to open file");
    }
    
    return Result;
}

internal u64
Win32GetSystemTimestamp()
{
    u64 Result;
    
    SYSTEMTIME SystemTime;
    Win32GetSystemTime(&SystemTime);
    
    FILETIME FileTime;
    Win32SystemTimeToFileTime(&SystemTime, &FileTime);
    
    ULARGE_INTEGER ULargeInteger;
    ULargeInteger.LowPart = FileTime.dwLowDateTime;
    ULargeInteger.HighPart = FileTime.dwHighDateTime;
    
    Result = ULargeInteger.QuadPart;
    
    return Result;
}

internal date_time
Win32GetDateTimeFromTimestamp(u64 Timestamp)
{
    date_time Result;
    
    ULARGE_INTEGER ULargeInteger;
    ULargeInteger.QuadPart = Timestamp;
    
    FILETIME FileTime;
    FileTime.dwLowDateTime = ULargeInteger.LowPart;
    FileTime.dwHighDateTime = ULargeInteger.HighPart;
    
    SYSTEMTIME SystemTime;
    Win32FileTimeToSystemTime(&FileTime, &SystemTime);
    
    Result.Year = SystemTime.wYear;
    Result.Month = (u8)SystemTime.wMonth;
    Result.Day = (u8)SystemTime.wDay;
    Result.Hour = (u8)SystemTime.wHour;
    Result.Minute = (u8)SystemTime.wMinute;
    Result.Second = (u8)SystemTime.wSecond;
    Result.Milliseconds = SystemTime.wMilliseconds;
    
    return Result;
}
