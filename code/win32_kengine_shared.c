
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
    
    // TODO(kstandbridge): Can I FreeAddress here or does it break the socket?
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

typedef enum iterate_processes_by_name_op
{
    IterateProcessesByName_None,
    IterateProcessesByName_Terminate
} iterate_processes_by_name_op;

internal b32
Win32IterateProcessesByName_(string FileName, iterate_processes_by_name_op Op)
{
    b32 Result = false;
    
    HANDLE Snapshot = Win32CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    PROCESSENTRY32 Entry;
    ZeroStruct(Entry);
    Entry.dwSize = sizeof(PROCESSENTRY32);
    b32 ProcessFound = Win32Process32First(Snapshot, &Entry);
    while(ProcessFound)
    {
        if(StringsAreEqual(CStringToString(Entry.szExeFile),
                           FileName))
        {
            HANDLE Process = Win32OpenProcess(PROCESS_TERMINATE, false, Entry.th32ProcessID);
            if(Process != 0)
            {
                Result = true;
                if(Op == IterateProcessesByName_Terminate)
                {                    
                    Win32TerminateProcess(Process, 0);
                    Win32CloseHandle(Process);
                }
                else
                {
                    Assert(Op == IterateProcessesByName_None);
                }
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
Win32KillProcessByName(string FileName)
{
    b32 Result = Win32IterateProcessesByName_(FileName, IterateProcessesByName_Terminate);
    return Result;
}

internal b32
Win32CheckForProcessByName(string FileName)
{
    b32 Result = Win32IterateProcessesByName_(FileName, IterateProcessesByName_None);
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
                DWORD ErrorCode = Win32GetLastError();
                Assert(!"Failed to create process");
            }
            
            Win32CloseHandle(OutPipeRead);
            
            DWORD ExitCode = 0;
            Win32GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode);
        }
        else
        {
            DWORD ErrorCode = Win32GetLastError();
            Assert(!"Failed to create out pipe");
        }
        
        Win32CloseHandle(InPipeWrite);
    }
    else
    {
        DWORD ErrorCode = Win32GetLastError();
        Assert(!"Failed to create in pipe");
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
Win32UnzipToFolder(string SourceZip, string DestFolder)
{
    if(!Win32DirectoryExists(DestFolder))
    {
        Win32CreateDirectory(DestFolder);
    }
    
    wchar_t CSourceZip[MAX_PATH];
    Win32MultiByteToWideChar(CP_UTF8, 0, (char *)SourceZip.Data, SourceZip.Size, CSourceZip, MAX_PATH);
    
    wchar_t CDestFolder[MAX_PATH];
    Win32MultiByteToWideChar(CP_UTF8, 0, (char *)DestFolder.Data, DestFolder.Size, CDestFolder, MAX_PATH);
    
    CSourceZip[SourceZip.Size] = '\0';
    CSourceZip[SourceZip.Size + 1] = '\0';
    
    CDestFolder[DestFolder.Size] = '\0';
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
        
        Win32Sleep(1000);
        
        FromFolder->lpVtbl->Release(FromFolder);
        ToFolder->lpVtbl->Release(ToFolder);
    }
    Win32CoUninitialize();
}

internal umm
Win32GetInternetData(HINTERNET WebConnect, u8 *Buffer, umm BufferSize, string Url)
{
    char CUrl[2048];
    StringToCString(Url, sizeof(CUrl), CUrl);
    umm Result = 0;
    
    HINTERNET WebAddress = Win32InternetOpenUrlA(WebConnect, CUrl, 0, 0, 0, 0);
    
    if (WebAddress)
    {
        DWORD CurrentBytesRead;
        do
        {
            if (Win32InternetReadFile(WebAddress, Buffer + Result, BufferSize, &CurrentBytesRead))
            {
                if (CurrentBytesRead == 0)
                {
                    break;
                }
                Result += CurrentBytesRead;
            }
            else
            {
                if (Win32GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                {
                    Assert(!"Read error");
                }
                else
                {
                    Assert(!"Insufficient buffer");
                }
            }
        }
        while (true);
        
    }
    else
    {
        Assert(!"Failed to open Url");
    }
    
    Win32InternetCloseHandle(WebAddress);
    
    return Result;
}
