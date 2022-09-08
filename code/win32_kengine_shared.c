
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
Win32ConsoleOut_(string Text)
{
    u32 Result = 0;
    
    HANDLE OutputHandle = Win32GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    Win32WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&Result, 0);
    Assert(Result == Text.Size);
    
    return Result;
}


internal b32
Win32ConsoleOut(memory_arena *Arena, char *Format, ...)
{
    format_string_state StringState = BeginFormatString(Arena);
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Text = EndFormatString(&StringState);
    
    b32 Result = Win32ConsoleOut_(Text);
    return Result;
}

internal FILETIME
Win32GetLastWriteTime(char *Filename)
{
    // TODO(kstandbridge): Switch this accept string?
    
    FILETIME LastWriteTime;
    ZeroStruct(LastWriteTime);
    
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(Win32GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }
    else
    {
        // TODO(kstandbridge): Error failed to GetFileAttributesExA
        InvalidCodePath;
    }
    
    return LastWriteTime;
}

typedef struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
} platform_work_queue_entry;

typedef struct platform_work_queue
{
    u32 volatile CompletionGoal;
    u32 volatile CompletionCount;
    
    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;
    HANDLE SemaphoreHandle;
    
    platform_work_queue_entry Entries[256];
} platform_work_queue;

internal void
Win32AddWorkEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
{
    u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Queue->CompletionGoal;
    _WriteBarrier();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    Win32ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal b32
Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
{
    b32 WeShouldSleep = false;
    
    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        u32 Index = AtomicCompareExchangeU32(&Queue->NextEntryToRead,
                                             NewNextEntryToRead,
                                             OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {        
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Entry.Data);
            AtomicIncrementU32(&Queue->CompletionCount);
        }
    }
    else
    {
        WeShouldSleep = true;
    }
    
    return WeShouldSleep;
}

internal void
Win32CompleteAllWork(platform_work_queue *Queue)
{
    while(Queue->CompletionGoal != Queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(Queue);
    }
    
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

DWORD WINAPI
Win32WorkQueueThread(void *lpParameter)
{
    platform_work_queue *Queue = (platform_work_queue *)lpParameter;
    
    u32 TestThreadId = GetThreadID();
    Assert(TestThreadId == Win32GetCurrentThreadId());
    
    for(;;)
    {
        if(Win32DoNextWorkQueueEntry(Queue))
        {
            Win32WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, false);
        }
    }
}

internal void
Win32MakeQueue(platform_work_queue *Queue, u32 ThreadCount)
{
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
    
    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;
    
    u32 InitialCount = 0;
    Queue->SemaphoreHandle = Win32CreateSemaphoreExA(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
    {
        u32 ThreadId;
        HANDLE ThreadHandle = Win32CreateThread(0, 0, Win32WorkQueueThread, Queue, 0, (LPDWORD)&ThreadId);
        Win32CloseHandle(ThreadHandle);
    }
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

internal s32
Win32TlsEncrypt(SOCKET Socket, u8 *Input, u32 InputLength, CtxtHandle *SSPICtxtHandle)
{
    SECURITY_STATUS Result;
    SecPkgContext_StreamSizes StreamSizes;
    
    Result = Win32SecurityFunctionTable->QueryContextAttributesA(SSPICtxtHandle, SECPKG_ATTR_STREAM_SIZES, &StreamSizes);
    if(SEC_E_OK == Result)
    {
        
        char WriteBuffer[2048];
        
        // TODO(kstandbridge): Really try to figure out why this worked
        // NOTE(kstandbridge): Put the message in the right place in the buffer
        //memcpy_s(WriteBuffer + StreamSizes.cbHeader, sizeof(WriteBuffer) - StreamSizes.cbHeader - StreamSizes.cbTrailer, Input, InputLength);
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
        Result = Win32SecurityFunctionTable->EncryptMessage(SSPICtxtHandle, 0, &BufferDesc, 0);
        
        if(SEC_E_OK == Result)
        {
            s32 NewLength = Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer;
            Win32SocketSendData(Socket, WriteBuffer, NewLength);
        }
    }
    
    return Result;
}

// TODO(kstandbridge): Move this?
#define TLS_MAX_BUFSIZ      32768

internal SECURITY_STATUS 
Win32SslClientHandshakeloop(memory_arena *Arena, SOCKET Socket, CredHandle *SSPICredHandle, CtxtHandle *SSPICtxtHandle)
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
            Win32SecurityFunctionTable->InitializeSecurityContextA(SSPICredHandle, SSPICtxtHandle, 0, FlagsIn, 0,
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
                
                ExtraData.pvBuffer = PushSize(Arena, InputBuffers[1].cbBuffer);
                
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
Win32TlsDecrypt(memory_arena *Arena, SOCKET Socket, CredHandle *SSPICredHandle, CtxtHandle *SSPICtxtHandle, u8 *OutBuffer, umm OutBufferLength)
{
    umm Result = 0;
    
    char Buffer[TLS_MAX_BUFSIZ];
    ZeroSize(TLS_MAX_BUFSIZ, Buffer);
    DWORD BufferMaxLength = TLS_MAX_BUFSIZ;
    DWORD BufferLength = 0;
    s32 BytesRecieved;
    
    SECURITY_STATUS SecurityStatus = 0;
    
    // NOTE(kstandbridge): Read from server until we have decripted data or disconnect/session ends
    b32 Continue = true;
    do
    {
        // NOTE(kstandbridge): Is this the first read or last read was incomplete
        if(BufferLength == 0 || SecurityStatus == SEC_E_INCOMPLETE_MESSAGE)
        {
            BytesRecieved = Win32Recv(Socket, &Buffer[BufferLength], BufferMaxLength - BufferLength, 0);
            if(BytesRecieved < 0)
            {
                Assert(!"Socket error");
                break;
            }
            
            if(BytesRecieved == 0)
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
            
            BufferLength += BytesRecieved;
        }
        
        // NOTE(kstandbridge): Try decript data
        SecBuffer DecryptBuffers[4];
        DecryptBuffers[0].pvBuffer = Buffer;
        DecryptBuffers[0].cbBuffer = BufferLength;
        DecryptBuffers[0].BufferType = SECBUFFER_DATA;
        
        DecryptBuffers[1].BufferType = SECBUFFER_EMPTY;
        DecryptBuffers[2].BufferType = SECBUFFER_EMPTY;
        DecryptBuffers[3].BufferType = SECBUFFER_EMPTY;
        
        SecBufferDesc DecryptBufferDesc;
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
            if((Result + pDataBuffer->cbBuffer) < OutBufferLength)
            {
                Copy(pDataBuffer->cbBuffer, pDataBuffer->pvBuffer, OutBuffer + Result);
                Result += pDataBuffer->cbBuffer;
            }
            else
            {
                Assert(!"Ran out of buffer space");
            }
            
            
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
            SecurityStatus = Win32SslClientHandshakeloop(Arena, Socket, SSPICredHandle, SSPICtxtHandle);
            
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
    
    return Result;
}

internal b32
Win32HttpClientSendData(http_client *Client, void *Input, s32 InputLength)
{
    b32 Result = false;
    
    Result = (SEC_E_OK == Win32TlsEncrypt(Client->Socket, Input, InputLength, Client->Context));
    
    return true;
}

internal u32
Win32SslSocketConnect(memory_arena *Arena, string Hostname, void **Creds, void **Context)
{
    SOCKET Result = 0;
    
    char CHostname[MAX_PATH];
    StringToCString(Hostname, MAX_PATH, CHostname);
    
    struct addrinfo Hints;
    ZeroStruct(Hints);
    Hints.ai_family = AF_INET; // NOTE(kstandbridge): IPv4
    Hints.ai_socktype = SOCK_STREAM; // NOTE(kstandbridge): TCP
    Hints.ai_protocol = IPPROTO_TCP;
    
    struct addrinfo *AddressInfos;
    
    if(Win32GetAddrInfo(CHostname, "443", &Hints, &AddressInfos) == 0)
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
                    CredHandle *SSPICredHandle = PushStruct(Arena, CredHandle);;
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
                            
                            CtxtHandle *SSPICtxtHandle = PushStruct(Arena, CtxtHandle);
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
                                    SecurityStatus = Win32SslClientHandshakeloop(Arena, Result, SSPICredHandle, SSPICtxtHandle);
                                    
                                    PCCERT_CONTEXT RemoteCertContext;
                                    SecurityStatus = Win32SecurityFunctionTable->QueryContextAttributes(SSPICtxtHandle,
                                                                                                        SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                                                                                        (PVOID)&RemoteCertContext);
                                    
                                    *Creds = (void *)SSPICredHandle;
                                    *Context = (void *)SSPICtxtHandle;
                                    
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
    
    // TODO(kstandbridge): Can I FreeAddress here or does it break the socket?
    Win32FreeAddrInfo(AddressInfos);
    
    return Result;
}

internal u32
Win32SocketConnect(string Hostname)
{
    SOCKET Result = 0;
    
    char CHostname[MAX_PATH];
    StringToCString(Hostname, MAX_PATH, CHostname);
    
    struct addrinfo *AddressInfo;
    struct addrinfo Hints;
    ZeroStruct(Hints);
    Hints.ai_family = AF_INET; // NOTE(kstandbridge): IPv4
    Hints.ai_socktype = SOCK_STREAM; // NOTE(kstandbridge): TCP
    Hints.ai_protocol = IPPROTO_TCP;
    
    if(Win32GetAddrInfo(CHostname, "http", &Hints, &AddressInfo) == 0)
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
    
    // TODO(kstandbridge): Can I FreeAddress here or does it break the socket?
    Win32FreeAddrInfo(AddressInfo);
    
    return Result;
}

internal void
Win32SocketClose(SOCKET Socket)
{
    // TODO(kstandbridge): I would maybe need to free the address here?
    Win32CloseSocket(Socket);
}

internal string
Win32SendHttpRequest(memory_arena *Arena, char *Url, char *Request, s32 Length)
{
    // TODO(kstandbridge): This shouldn't be transient, we can keep the socket and make multiple requests
    
    string Result;
    
    struct addrinfo *AddressInfo;
    struct addrinfo Hints;
    ZeroStruct(Hints);
    Hints.ai_family = AF_INET; // NOTE(kstandbridge): IPv4
    Hints.ai_socktype = SOCK_STREAM; // NOTE(kstandbridge): TCP
    Hints.ai_protocol = IPPROTO_TCP;
    
    if(Win32GetAddrInfo(Url, "http", &Hints, &AddressInfo) != 0)
    {
        
        Result = FormatString(Arena, "Hostname look up failed: %d", Win32WSAGetLastError());
    }
    else
    {
        SOCKET Socket = Win32Socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
        if(INVALID_SOCKET == Socket)
        {
            Result = FormatString(Arena, "Could not create socket: %d", Win32WSAGetLastError());
        }
        else
        {
            if(SOCKET_ERROR == Win32Connect(Socket, AddressInfo->ai_addr, (s32)AddressInfo->ai_addrlen))
            {
                Result = FormatString(Arena, "Connect error: %d", Win32WSAGetLastError());
            }
            else
            {
                DWORD TimeoutMs =  3000;
                if(SOCKET_ERROR == Win32SetSockOpt(Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&TimeoutMs, sizeof(TimeoutMs)))
                {
                    Result = FormatString(Arena, "Error setting socket timeout: %d", Win32WSAGetLastError());
                }
                else
                {
                    if(Win32Send(Socket, Request, Length, 0) < 0)
                    {
                        Result = FormatString(Arena, "Send failed!");
                    }
                    else
                    {
                        s32 RecieveSize;
                        char ServerReply[512];
                        
                        format_string_state StringState = BeginFormatString(Arena);
                        
                        while((RecieveSize = Win32Recv(Socket, ServerReply, sizeof(ServerReply) - 1, 0)) > 0)
                        {
                            ServerReply[RecieveSize] = '\0';
                            AppendStringFormat(&StringState, "%s", ServerReply);
                        }
                        Result = EndFormatString(&StringState);
                    }
                }
            }
            
            Win32CloseSocket(Socket);
            Win32FreeAddrInfo(AddressInfo);
        }
    }
    
    return Result;
}


internal string
Win32SendHttpsRequest(memory_arena *Arena, char *Url, char *Request, s32 Length)
{
    string Result = {0};
    
    char *Port = "443";
    
    struct addrinfo Hints;
    ZeroStruct(Hints);
    
#if 1
    s32 Family = AF_INET; // NOTE(kstandbridge): IPv4
#else
    s32 Family = AF_INET6; // NOTE(kstandbridge): IPv6
#endif
    
    Hints.ai_family   = Family;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    
    // NOTE(kstandbridge): Resolve network address for host
    struct addrinfo *AddressInfos;
    if(Win32GetAddrInfo(Url, Port, &Hints, &AddressInfos))
    {
        Result = PushString(Arena, "Hostname look up failed");
    }
    else
    {
        
        // NOTE(kstandbridge): Traverse list of network addresses
        SOCKET Socket;
        
        for(struct addrinfo *AddressInfo = AddressInfos;
            AddressInfo != 0;
            AddressInfo = AddressInfo->ai_next)
        {
            if(AddressInfo->ai_family == Family)
            {                
                // NOTE(kstandbridge): Create a socket for event signalling
                Socket = Win32Socket(AddressInfo->ai_family, SOCK_STREAM, IPPROTO_TCP);
                if(SOCKET_ERROR != Socket)
                {
                    // NOTE(kstandbridge): Ensure we can reuse same port later
                    s32 On = 1;
                    Win32SetSockOpt(Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&On, sizeof(On));
                }
                
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
                CredHandle SSPICredHandle;
                SECURITY_STATUS SecurityStatus = 
                    Win32SecurityFunctionTable->AcquireCredentialsHandleA(0,UNISP_NAME_A, SECPKG_CRED_OUTBOUND,
                                                                          0, &SChannelCred, 0, 0, &SSPICredHandle, 0);
                
                // NOTE(kstandbridge): Open connection to remote host
                if(SOCKET_ERROR == Win32Connect(Socket, AddressInfo->ai_addr, (s32)AddressInfo->ai_addrlen))
                {
                    Result = FormatString(Arena, "Connect error: %d", Win32WSAGetLastError());
                }
                else
                {
                    
                    // NOTE(kstandbridge): Send initial hello
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
                    
                    CtxtHandle SSPICtxtHandle;
                    DWORD FlagsOut;
                    SecurityStatus = 
                        Win32SecurityFunctionTable->InitializeSecurityContextA(&SSPICredHandle, 0, 0, FlagsIn, 0,
                                                                               SECURITY_NATIVE_DREP, 0, 0,
                                                                               &SSPICtxtHandle, &BufferDesc, &FlagsOut, 0);
                    
                    if(SEC_I_CONTINUE_NEEDED != SecurityStatus)
                    {
                        Result = PushString(Arena, "Handshake failed initalizing security context");
                    }
                    
                    Assert(SEC_I_CONTINUE_NEEDED == SecurityStatus);
                    
                    //else if(SEC_I_CONTINUE_NEEDED == SecurityStatus)
                    {
                        
                        // NOTE(kstandbridge): Send data
                        if(Buffers[0].cbBuffer != 0)
                        {
                            Win32SocketSendData(Socket, Buffers[0].pvBuffer, Buffers[0].cbBuffer);
                            
                            SecurityStatus = Win32SecurityFunctionTable->FreeContextBuffer(Buffers[0].pvBuffer);
                            
                            Buffers[0].pvBuffer = 0;
                        }
                        
                        if(SEC_E_OK != SecurityStatus)
                        {
                            Result = PushString(Arena, "Handshake failed sending data");
                        }
                        else
                        {
                            SecurityStatus = SEC_I_CONTINUE_NEEDED;
                            
#define TLS_MAX_BUFSIZ      32768
                            char Buffer[TLS_MAX_BUFSIZ];
                            DWORD BufferMaxLength = TLS_MAX_BUFSIZ;
                            DWORD BufferLength = 0;
                            s32 BytesRecieved;
                            
                            SecBuffer ExtraData;
                            ZeroStruct(ExtraData);
                            
                            while((SecurityStatus == SEC_I_CONTINUE_NEEDED) ||
                                  (SecurityStatus == SEC_E_INCOMPLETE_MESSAGE) ||
                                  (SecurityStatus == SEC_I_INCOMPLETE_CREDENTIALS))
                            {
                                if(SecurityStatus == SEC_E_INCOMPLETE_MESSAGE)
                                {
                                    // NOTE(kstandbridge): Recieve data from server
                                    BytesRecieved = Win32Recv(Socket, &Buffer[BufferLength], BufferMaxLength - BufferLength, 0);
                                    
                                    if(SOCKET_ERROR == BytesRecieved)
                                    {
                                        SecurityStatus = SEC_E_INTERNAL_ERROR;
                                        Result = PushString(Arena, "Socket error");
                                        break;
                                    }
                                    else if(BytesRecieved == 0)
                                    {
                                        SecurityStatus = SEC_E_INTERNAL_ERROR;
                                        Result = PushString(Arena, "Server disconnected");
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
                                
                                SecurityStatus = 
                                    Win32SecurityFunctionTable->InitializeSecurityContextA(&SSPICredHandle, &SSPICtxtHandle, 0, FlagsIn, 0,
                                                                                           SECURITY_NATIVE_DREP, &InputBufferDesc, 0, 0,
                                                                                           &OutputBufferDesc, &FlagsOut, 0);
                                
                                // NOTE(kstandbridge): What have we got so far?
                                if((SecurityStatus == SEC_E_OK) ||
                                   (SecurityStatus == SEC_I_CONTINUE_NEEDED) ||
                                   (FAILED(SecurityStatus) && (FlagsOut & ISC_RET_EXTENDED_ERROR)))
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
                                if(SecurityStatus == SEC_E_INCOMPLETE_MESSAGE)
                                {
                                    continue;
                                }
                                
                                // NOTE(kstandbridge): Completed handsake
                                if(SecurityStatus == SEC_E_OK)
                                {
                                    // NOTE(kstandbridge): The extra buffer could contain encrypted application protocol later stuff, we will decript it later with DecryptMessage
                                    if(InputBuffers[1].BufferType == SECBUFFER_EXTRA)
                                    {
                                        // TODO(kstandbridge): I've yet to hit this case, but we have data to deal with
                                        __debugbreak();
                                        
                                        ExtraData.pvBuffer = PushSize(Arena, InputBuffers[1].cbBuffer);
                                        
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
                                    Result = PushString(Arena, "Socket error");
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
                            
                            if(SEC_E_OK != SecurityStatus)
                            {
                                __debugbreak();
                                Result = PushString(Arena, "Socket error");
                            }
                            else
                            {
                                SecurityStatus = Win32TlsEncrypt(Socket, (u8 *)Request, Length, &SSPICtxtHandle);
                                if(SEC_E_OK == SecurityStatus)
                                {
                                    
                                    // TODO(kstandbridge): Move to TlsDecrypt
                                    {
#if 0                                        
                                        s32 BytesRecieved;
                                        char Buffer[TLS_MAX_BUFSIZ];
                                        DWORD BufferMaxLength = TLS_MAX_BUFSIZ;
                                        DWORD BufferLength = 0;
#else
                                        ZeroSize(TLS_MAX_BUFSIZ, Buffer);
                                        BufferMaxLength = TLS_MAX_BUFSIZ;
                                        BufferLength = 0;
#endif
                                        
                                        SecurityStatus = 0;
                                        
                                        wchar_t *At = 0;
                                        format_string_state StringState = BeginFormatString(Arena);
                                        
                                        
                                        // NOTE(kstandbridge): Read from server until we have decripted data or disconnect/session ends
                                        for(;;)
                                        {
                                            // NOTE(kstandbridge): Is this the first read or last read was incomplete
                                            if(BufferLength == 0 || SecurityStatus == SEC_E_INCOMPLETE_MESSAGE)
                                            {
                                                BytesRecieved = Win32Recv(Socket, &Buffer[BufferLength], BufferMaxLength - BufferLength, 0);
                                                
                                                if(BytesRecieved < 0)
                                                {
                                                    Result = PushString(Arena, "Socket error");
                                                    break;
                                                }
                                                
                                                if(BytesRecieved == 0)
                                                {
                                                    // NOTE(kstandbridge): Server disconnected
                                                    if(BufferLength)
                                                    {
                                                        Result = PushString(Arena, "Disconnected while reciving data");
                                                        break;
                                                    }
                                                    // NOTE(kstandbridge): Session ended, we read all the data
                                                    break;
                                                }
                                                
                                                BufferLength += BytesRecieved;
                                            }
                                            
                                            // NOTE(kstandbridge): Try decript data
                                            SecBuffer DecryptBuffers[4];
                                            DecryptBuffers[0].pvBuffer = Buffer;
                                            DecryptBuffers[0].cbBuffer = BufferLength;
                                            DecryptBuffers[0].BufferType = SECBUFFER_DATA;
                                            
                                            DecryptBuffers[1].BufferType = SECBUFFER_EMPTY;
                                            DecryptBuffers[2].BufferType = SECBUFFER_EMPTY;
                                            DecryptBuffers[3].BufferType = SECBUFFER_EMPTY;
                                            
                                            SecBufferDesc DecryptBufferDesc;
                                            DecryptBufferDesc.ulVersion = SECBUFFER_VERSION;
                                            DecryptBufferDesc.cBuffers = 4;
                                            DecryptBufferDesc.pBuffers = DecryptBuffers;
                                            
                                            SecurityStatus = Win32SecurityFunctionTable->DecryptMessage(&SSPICtxtHandle, &DecryptBufferDesc, 0, 0);
                                            
                                            if(SecurityStatus == SEC_E_INCOMPLETE_MESSAGE)
                                            {
                                                continue;
                                            }
                                            
                                            if(SecurityStatus == SEC_I_CONTEXT_EXPIRED)
                                            {
                                                Result = PushString(Arena, "Context expired");
                                                break;
                                            }
                                            
                                            if((SecurityStatus != SEC_E_OK) &&
                                               (SecurityStatus != SEC_I_RENEGOTIATE) &&
                                               (SecurityStatus != SEC_I_CONTEXT_EXPIRED))
                                            {
                                                Result = PushString(Arena, "Decryption error");
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
                                                __debugbreak();
                                                char *EndPointer = (char *)pDataBuffer->pvBuffer + pDataBuffer->cbBuffer;
                                                EndPointer[0] = '\0';
                                                AppendStringFormat(&StringState, "%s", (char *)pDataBuffer->pvBuffer);
                                            }
                                            
                                            if(pExtraBuffer)
                                            {
                                                MoveMemory(Buffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
                                                BufferLength = pExtraBuffer->cbBuffer;
                                                continue;
                                            }
                                            else
                                            {
                                                __debugbreak();
                                                // NOTE(kstandbridge): This means we're done?
                                            }
                                            
                                            if(SecurityStatus == SEC_I_RENEGOTIATE)
                                            {
                                                __debugbreak();
                                                // TODO(kstandbridge): We need to connect again!
                                            }
                                            
#if 0                                            
                                            char *EndPointer = (char *)DecryptBuffers[1].pvBuffer + DecryptBuffers[1].cbBuffer;
                                            EndPointer[0] = '\0';
                                            AppendStringFormat(&StringState, "%s", (char *)DecryptBuffers[1].pvBuffer);
                                            break;
#endif
                                        }
                                        
                                        Result = EndFormatString(&StringState);
                                    }
                                    
                                }
                                else
                                {
                                    Result = PushString(Arena, "Failed to send encrypted data");
                                }
                                
                            }
                        }
                    }
                    
                    
                    Win32Shutdown(Socket, SD_BOTH);
                    Win32CloseSocket(Socket);
                    Win32SecurityFunctionTable->DeleteSecurityContext(&SSPICtxtHandle);
                    
                }
                
                break;
                
            }
        }
        
        Win32FreeAddrInfo(AddressInfos);
        
        
    }
    
#if 0    
    DebugLog(L"Data sent!");
    char ServerReply[512];
    wchar_t *At = 0;
    Result = BeginPushString(Arena, &At);
    DebugLog(L"Begin Recv");
    while(1)
    {
        s32 RecieveSize = BIO_read(Bio, ServerReply, sizeof(ServerReply));
        DebugLog(L"Recieved %d data!", RecieveSize);
        ServerReply[RecieveSize] = '\0';
        ContinuePushString(Arena, &Result, ServerReply, &At);
        if (RecieveSize <= 0) break; /* 0 is end-of-stream, < 0 is an error */
    }
    EndPushString(Arena, &Result);
#endif
    
    return Result;
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