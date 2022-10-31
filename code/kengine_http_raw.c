
typedef struct http_client
{
    string Hostname;
    
    u32 Socket;
    CredHandle SSPICredHandle;
    CtxtHandle SSPICtxtHandle;
} http_client;

typedef enum http_method_type
{
    HttpMethod_Unknown,
    
    HttpMethod_GET,
} http_method_type;

inline string
HttpMethodToString(http_method_type Method)
{
    string Result;
    ZeroStruct(Result);
    
    switch(Method)
    {
        case HttpMethod_GET: { Result = String("GET"); } break;
        
        case HttpMethod_Unknown:
        InvalidDefaultCase;
    }
    
    return Result;
}

typedef enum http_accept_type
{
    HttpAccept_Unknown, 
    
    HttpAccept_Any
} http_accept_type;

inline string
HttpAcceptToString(http_accept_type Accept)
{
    string Result;
    ZeroStruct(Result);
    
    switch(Accept)
    {
        case HttpAccept_Any: { Result = String("*/*"); } break;
        
        case HttpAccept_Unknown:
        InvalidDefaultCase;
    }
    
    return Result;
}

typedef struct http_request
{
    http_client *Client;
    http_method_type Method;
    string Endpoint;
    http_accept_type Accept;
    
    string Raw;
    string DownloadPath;
    
} http_request;

inline void
HttpRequestDownloadResponseToFile(http_request *Request, string DownloadPath)
{
    Request->DownloadPath = DownloadPath;
}

typedef enum http_response_code_type
{
    HttpResponseCode_SocketError = -1,
    
    HttpResponseCode_NotFound = 404,
} http_response_code_type;

typedef enum http_response_content_type
{
    HttpResponseContent_Unknown,
    
    HttpResponseContent_TextHTML,
    HttpResponseContent_ApplicatonZip,
    HttpResponseContent_ApplicatonOctetStream,
} http_response_content_type;

typedef enum http_transfer_encoding_type
{
    HttpTransferEncoding_Unknown,
    
    HttpTransferEncoding_Chunked,
} http_transfer_encoding_type;

typedef struct http_response
{
    u8 Major;
    u8 Minor;
    
    // TODO(kstandbridge): Make this u16?
    http_response_code_type Code;
    string Message;
    
    http_transfer_encoding_type TransferEncoding;
    
    http_response_content_type ContentType;
    u32 ContentLength;
    string Content;
    
} http_response;

inline http_client
BeginHttpClientRaw(string Hostname, string Port)
{
    http_client Result;
    ZeroStruct(Result);
    
    Result.Hostname = Hostname;
    Result.Socket = Win32SslSocketConnect(Hostname, Port, &Result.SSPICredHandle, &Result.SSPICtxtHandle);
    
    return Result;
}

inline void
EndHttpClientRaw(http_client *Client)
{
    Win32SocketClose(Client->Socket);
}

inline http_request
BeginHttpRequestRaw(http_client *Client, http_method_type Method, http_accept_type Accept, string Endpoint)
{
    http_request Result;
    ZeroStruct(Result);
    
    Result.Client = Client;
    Result.Method = Method;
    Result.Endpoint = Endpoint;
    Result.Accept = Accept;
    
    return Result;
}

internal http_response
GetSslHttpResponse(http_client *Client, memory_arena *PermArena, memory_arena *TempArena, string Request, string DownloadPath)
{
    http_response Result;
    ZeroStruct(Result);
    
    if(Win32SendEncryptedMessage(Client->Socket, Request.Data, Request.Size, &Client->SSPICtxtHandle))
    {
        b32 DownloadResponse = (DownloadPath.Data != 0);
        HANDLE FileHandle = 0;
        if(DownloadResponse)
        {
            char CDownloadPath[MAX_PATH];
            StringToCString(DownloadPath, MAX_PATH, CDownloadPath);
            FileHandle = Win32CreateFileA(CDownloadPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        }
        
        b32 ParsingHeader = true;
        u32 HeaderSize = 0;
        u32 ContentRemaining = U32Max;
        format_string_state BodyStringState;
        ZeroStruct(BodyStringState);
        
        u32 ChunkRemaing = 0;
        b32 Parsing = true;
        while(Parsing)
        {
            if(ContentRemaining == 0)
            {
                break;
            }
            
            u8 Buffer[4096];
            umm BufferMaxSize = sizeof(Buffer);
            char *LineStart = 0;
            umm BufferSize = Win32RecieveDecryptedMessage(Client->Socket, &Client->SSPICredHandle, &Client->SSPICtxtHandle, BufferSize, BufferMaxSize);
            
            if(BufferSize == 0)
            {
                break;
            }
            
            if(ParsingHeader)
            {
                LineStart = (char *)Buffer;
                char *LineEnd = (char *)Buffer;
                u32 Index = 0;
                while(Index < BufferSize)
                {
                    if((*(LineEnd - 1) == '\r') &&
                       (*LineEnd  == '\n'))
                    {
                        string Line = String_(LineEnd - LineStart - 1, (u8 *)LineStart);
                        HeaderSize += LineEnd - LineStart;
                        if(StringBeginsWith(String("\r\n"), Line))
                        {
                            HeaderSize += 2;
                            ParsingHeader = false;
                            BufferSize -= (LineEnd - (char *)Buffer) + 1;
                            Buffer = (u8 *)LineEnd + 1;
                            BodyStringState = BeginFormatString();
                            break;
                        }
                        else if(StringBeginsWith(String("HTTP/"), Line))
                        {
                            string ResponseMessage;
                            ZeroStruct(ResponseMessage);
                            
                            ParseFromString(Line, "HTTP/%d.%d %d %S",
                                            &Result.Major, &Result.Minor, &Result.Code, &ResponseMessage);
                            
                            Result.Message = PushString_(PermArena, ResponseMessage.Size, ResponseMessage.Data);
                        }
                        else if(StringBeginsWith(String("Content-Type"), Line))
                        {
                            string ContentType = String_(LineEnd - LineStart - 15, (u8 *)LineStart + 14);
                            if(StringsAreEqual(ContentType, String("text/html")))
                            {
                                Result.ContentType = HttpResponseContent_TextHTML;
                            }
                            else if(StringsAreEqual(ContentType, String("application/zip")))
                            {
                                Result.ContentType = HttpResponseContent_ApplicatonZip;
                            }
                            else if(StringsAreEqual(ContentType, String("application/octet-stream")))
                            {
                                Result.ContentType = HttpResponseContent_ApplicatonOctetStream;
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else if(StringBeginsWith(String("Content-Length"), Line))
                        {
                            ContentRemaining = U32FromString(LineStart + 16);
                            Result.ContentLength = ContentRemaining;
                        }
                        else if(StringBeginsWith(String("Transfer-Encoding"), Line))
                        {
                            string TransferEncoding = String_(LineEnd - LineStart - 20, (u8 *)LineStart + 19);
                            if(StringsAreEqual(String("chunked"), TransferEncoding))
                            {
                                Result.TransferEncoding = HttpTransferEncoding_Chunked;
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else
                        {
                            // TODO(kstandbridge): Process this line
                            int x = 5;
                        }
                        LineStart = ++LineEnd;
                    }
                    ++LineEnd;
                    ++Index;
                }
            }
            
            if(!ParsingHeader)
            {
                ContentRemaining -= BufferSize;
            }
            
            if(Result.TransferEncoding == HttpTransferEncoding_Chunked)
            {            
                if(ChunkRemaing > 0)
                {
                    Assert(BufferSize > ChunkRemaing);
                    
                    string Chunk = String_(ChunkRemaing, Buffer);
                    
                    AppendStringFormat(&BodyStringState, "%S", Chunk);
                    // NOTE(kstandbridge): \r\n
                    Buffer += ChunkRemaing + 2; 
                    BufferSize -= ChunkRemaing - 2;
                    ChunkRemaing = 0;
                }
                
                while(BufferSize > 6)
                {
                    char Hex[4];
                    Hex[0] = '0';
                    Hex[1] = '0';
                    Hex[2] = '0';
                    Hex[3] = '0';
                    
                    u32 HexLength = 0;
                    char *Parse = (char *)Buffer;
                    while((Parse[0] != '\r') &&
                          (Parse[1] != '\n'))
                    {
                        ++HexLength;
                        ++Parse;
                    }
                    if(HexLength > 4)
                    {
                        Parsing = false;
                        break;
                    }
                    u32 Current = 3;
                    for(u32 Index = 0;
                        Index < HexLength;
                        ++Index)
                    {
                        --Parse;
                        Hex[Current--] = *Parse;
                    }
                    
                    u32 ChunkSize = ((GetHex(Hex[0]) << 12) |
                                     (GetHex(Hex[1]) << 8) |
                                     (GetHex(Hex[2]) << 4) |
                                     (GetHex(Hex[3]) << 0));
                    
                    Buffer += HexLength + 2;
                    BufferSize -= HexLength - 2;
                    
                    if(ChunkSize > BufferSize)
                    {
                        // NOTE(kstandbridge): Ditch null terminators
                        while(Response.Data[BufferSize] == '\0')
                        {
                            --BufferSize;
                        }
                        
                        ChunkRemaing = ChunkSize - BufferSize;
                        ChunkSize -= ChunkRemaing; 
                        ++ChunkSize; // NOTE(kstandbridge): I don't know why
                        --ChunkRemaing; // NOTE(kstandbridge): I don't know why
                    }
                    
                    string Chunk = String_(ChunkSize, Buffer);
                    AppendStringFormat(&BodyStringState, "%S", Chunk);
                    
                    if(StringContains(String("</HTML>"), Chunk) ||
                       StringContains(String("</html>"), Chunk))
                    {
                        Parsing = false;
                        break;
                    }
                    
                    
                    // NOTE(kstandbridge): additional 2 for \r\n
                    Buffer += ChunkSize + 2;
                    BufferSize -= ChunkSize - 2;
                }
                
            }
            else
            {
                if(DownloadResponse)
                {
                    DWORD BytesWritten = 0;
                    Win32WriteFile(FileHandle, Buffer, (DWORD)BufferSize, &BytesWritten, 0);
                    Assert(BufferSize == BytesWritten);
                }
                else
                {                    
                    AppendStringFormat(&BodyStringState, "%S", String_(BufferSize, Buffer));
                    
                    if(StringContains(String("</HTML>"), Buffer) ||
                       StringContains(String("</html>"), Buffer))
                    {
                        Parsing = false;
                        break;
                    }
                }
                
            }
        }
        
        if(DownloadResponse)
        {
            Win32CloseHandle(FileHandle);
        }
        
        Result.Content = EndFormatString(&BodyStringState);
    }
    else
    {
        Result.Code = HttpResponseCode_SocketError;
    }
    
    return Result;
}

inline string
GenerateHttpMessage_(http_request *Request, memory_arena *Arena)
{
    format_string_state StringState = BeginFormatString();
    AppendStringFormat(&StringState, "%S %S HTTP/1.1\r\n", HttpMethodToString(Request->Method), Request->Endpoint);
    AppendStringFormat(&StringState, "Host: %S\r\n", Request->Client->Hostname);
    AppendStringFormat(&StringState, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n");
    AppendStringFormat(&StringState, "Accept: %S\r\n\r\n", HttpAcceptToString(Request->Accept));
    
    string Result = EndFormatString(&StringState, Arena);
    
    return Result;
}

inline http_response
EndHttpRequestRaw(http_request *Request, memory_arena *Arena, memory_arena *TempArena)
{
    http_client *Client = Request->Client;
    
    Request->Raw = GenerateHttpMessage_(Request, TempArena);
    
    http_response Result = GetSslHttpResponse(Client, Arena, TempArena, Request->Raw, Request->DownloadPath);
    
    
    return Result;
}

