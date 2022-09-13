// TODO(kstandbridge): there should be NO win32 in this file, replace with platform invocations

inline http_client
BeginHttpClient(memory_arena *Arena, string Hostname, string Port)
{
    http_client Result;
    ZeroStruct(Result);
    
    Result.Hostname = Hostname;
    Result.Socket = Win32SslSocketConnect(Arena, Hostname, Port, &Result.Creds, &Result.Context);
    
    return Result;
}

inline void
EndHttpClient(http_client *Client)
{
    Win32SocketClose(Client->Socket);
}

inline http_request
BeginHttpRequest(http_client *Client, http_method_type Method, http_accept_type Accept, string Endpoint)
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
GetSslHttpResponse(http_client *Client, memory_arena *PermArena, memory_arena *TempArena, string Request)
{
    http_response Result;
    ZeroStruct(Result);
    
    if(Win32SendEncryptedMessage(Client->Socket, Request.Data, Request.Size, Client->Context))
    {
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
            
            temporary_memory TempMem = BeginTemporaryMemory(TempArena);
            char *LineStart = 0;
            string Response = BeginPushString(TempMem.Arena);
            umm ResponseSize = Win32RecieveDecryptedMessage(TempMem.Arena, Client->Socket, Client->Creds, Client->Context, Response.Data, TempMem.Arena->Size - TempMem.Arena->Used);
            EndPushString(&Response, TempMem.Arena, ResponseSize);
            
            if(ResponseSize == 0)
            {
                break;
            }
            
            if(ParsingHeader)
            {
                LineStart = (char *)Response.Data;
                char *LineEnd = (char *)Response.Data;
                u32 Index = 0;
                while(Index < ResponseSize)
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
                            Response.Size -= (LineEnd - (char *)Response.Data) + 1;
                            Response.Data = (u8 *)LineEnd + 1;
                            BodyStringState = BeginFormatString(PermArena);
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
                                Result.ContentType = HttpResponseContent_ApplicaitonZip;
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
                ContentRemaining -= Response.Size;
            }
            
            if(Result.TransferEncoding == HttpTransferEncoding_Chunked)
            {            
                if(ChunkRemaing > 0)
                {
                    Assert(Response.Size > ChunkRemaing);
                    
                    string Chunk = String_(ChunkRemaing, Response.Data);
                    
                    AppendStringFormat(&BodyStringState, "%S", Chunk);
                    // NOTE(kstandbridge): \r\n
                    Response.Data += ChunkRemaing + 2; 
                    Response.Size -= ChunkRemaing - 2;
                    ChunkRemaing = 0;
                }
                
                while(Response.Size > 6)
                {
                    char Hex[4];
                    Hex[0] = '0';
                    Hex[1] = '0';
                    Hex[2] = '0';
                    Hex[3] = '0';
                    
                    u32 HexLength = 0;
                    char *Parse = (char *)Response.Data;
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
                    
                    Response.Data += HexLength + 2;
                    Response.Size -= HexLength - 2;
                    
                    if(ChunkSize > Response.Size)
                    {
                        // NOTE(kstandbridge): Ditch null terminators
                        while(Response.Data[Response.Size] == '\0')
                        {
                            --Response.Size;
                        }
                        
                        ChunkRemaing = ChunkSize - Response.Size;
                        ChunkSize -= ChunkRemaing; 
                        ++ChunkSize; // NOTE(kstandbridge): I don't know why
                        --ChunkRemaing; // NOTE(kstandbridge): I don't know why
                    }
                    
                    string Chunk = String_(ChunkSize, Response.Data);
                    AppendStringFormat(&BodyStringState, "%S", Chunk);
                    
                    if(StringContains(String("</HTML>"), Chunk) ||
                       StringContains(String("</html>"), Chunk))
                    {
                        Parsing = false;
                        break;
                    }
                    
                    
                    // NOTE(kstandbridge): additional 2 for \r\n
                    Response.Data += ChunkSize + 2;
                    Response.Size -= ChunkSize - 2;
                }
                
            }
            else
            {
                AppendStringFormat(&BodyStringState, "%S", Response);
                
                if(StringContains(String("</HTML>"), Response) ||
                   StringContains(String("</html>"), Response))
                {
                    Parsing = false;
                    break;
                }
                
            }
            
            EndTemporaryMemory(TempMem);
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
    // TODO(kstandbridge): Actually generate the raw message
    format_string_state StringState = BeginFormatString(Arena);
    AppendStringFormat(&StringState, "%S %S HTTP/1.1\r\n", HttpMethodToString(Request->Method), Request->Endpoint);
    AppendStringFormat(&StringState, "Host: %S\r\n", Request->Client->Hostname);
    AppendStringFormat(&StringState, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n");
    AppendStringFormat(&StringState, "Accept: %S\r\n\r\n", HttpAcceptToString(Request->Accept));
    
    string Result = EndFormatString(&StringState);
    
    return Result;
}

inline http_response
EndHttpRequest(http_request *Request, memory_arena *PermArena, memory_arena *TempArena)
{
    http_client *Client = Request->Client;
    
    Request->Raw = GenerateHttpMessage_(Request, TempArena);
    
    http_response Result = GetSslHttpResponse(Client, PermArena, TempArena, Request->Raw);
    
    
    return Result;
}

