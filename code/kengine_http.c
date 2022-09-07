// TODO(kstandbridge): there should be NO win32 in this file, replace with platform invocations

inline http_client
BeginHttpClient(memory_arena *Arena, string Hostname, b32 IsSsl)
{
    http_client Result;
    ZeroStruct(Result);
    
    Result.Hostname = Hostname;
    Result.IsSsl = IsSsl;
    Result.Socket = (IsSsl) ? Win32SslSocketConnect(Arena, Hostname, &Result.Creds, &Result.Context) : Win32SocketConnect(Hostname);
    
    return Result;
}

inline void
EndHttpClient(http_client *Client)
{
    Win32SocketClose(Client->Socket);
}

inline http_request
BeginHttpRequest(http_client *Client, http_method_type Method, string Endpoint, http_accept_type Accept)
{
    http_request Result;
    ZeroStruct(Result);
    
    Result.Client = Client;
    Result.Method = Method;
    Result.Endpoint;
    Result.Accept;
    
    return Result;
}

internal http_response
GetHttpResponse(http_client *Client, memory_arena *Arena, string Request)
{
    http_response Result;
    ZeroStruct(Result);
    
    if(Win32HttpClientSendData(Client, Request.Data, Request.Size))
    {
        // TODO(kstandbridge): Make buffer larger, this is just stress testing
        char Buffer[64];
        u32 BufferSize = sizeof(Buffer);
        s32 At = 0;
        for(;;)
        {
            s32 Recieved = Win32Recv(Client->Socket, Buffer + At, 1, 0);
            if(Recieved == -1)
            {
                // TODO(kstandbridge): Socket error
                Result.Code = HttpResponseCode_SocketError;
                break;
            }
            else if(Recieved == 0)
            {
                // TODO(kstandbridge): Server closed socket
                Result.Code = HttpResponseCode_SocketError;
                break;
            }
            else
            {
                if((Buffer[At - 1] == '\r') &&
                   (Buffer[At] == '\n'))
                {
                    if((Buffer[0] == '\r') &&
                       (Buffer[1] == '\n'))
                    {
                        // NOTE(kstandbridge): Header is finished.
                        u32 LengthRemaining = Result.ContentLength;
                        format_string_state StringState = BeginFormatString(Arena);
                        while(LengthRemaining > 0)
                        {
                            u32 LengthToGet = (LengthRemaining > BufferSize) ? BufferSize : LengthRemaining;
                            Recieved = Win32Recv(Client->Socket, Buffer, LengthToGet, 0);
                            if(Recieved == -1)
                            {
                                // TODO(kstandbridge): Socket error
                                Result.Code = HttpResponseCode_SocketError;
                                break;
                            }
                            else if(Recieved == 0)
                            {
                                // TODO(kstandbridge): Server closed socket
                                Result.Code = HttpResponseCode_SocketError;
                                break;
                            }
                            else
                            {
                                Buffer[Recieved] = '\0';
                                AppendStringFormat(&StringState, "%s", Buffer);
                                LengthRemaining-= Recieved;
                            }
                        }
                        Result.Content = EndFormatString(&StringState);
                        break;
                    }
                    else
                    {
                        Buffer[At - 1] = '\0';
                        // NOTE(kstandbridge): We recieved a line
                        if(StringBeginsWith(String_(At, (u8 *)Buffer), String("HTTP/")))
                        {
                            string ResponseMessage;
                            ZeroStruct(ResponseMessage);
                            ParseFromString(Buffer, "HTTP/%d.%d %d %S",
                                            &Result.Major, &Result.Minor, &Result.Code, &ResponseMessage);
                            
                            Result.Message = PushString_(Arena, ResponseMessage.Size, ResponseMessage.Data);
                        }
                        else if(StringBeginsWith(String_(At, (u8 *)Buffer), String("Content-Type")))
                        {
                            string ContentType = String_(At - 15, (u8 *)(Buffer + 14));
                            if(StringsAreEqual(ContentType, String("text/html")))
                            {
                                Result.ContentType = HttpResponseContent_HTML;
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else if(StringBeginsWith(String_(At, (u8 *)Buffer), String("Content-Length")))
                        {
                            Result.ContentLength = U32FromString(Buffer + 16);
                        }
                        else
                        {
                            // TODO(kstandbridge): Process this line
                            int x = 5;
                        }
                    }
                    At = 0;
                }
                else
                {
                    ++At;
                }
            }
        }
    }
    else
    {
        Result.Code = HttpResponseCode_SocketError;
    }
    
    return Result;
}

internal http_response
GetSslHttpResponse(http_client *Client, memory_arena *Arena, string Request)
{
    http_response Result;
    ZeroStruct(Result);
    
    if(Win32HttpClientSendData(Client, Request.Data, Request.Size))
    {
        char Buffer[20480];
        u32 BufferSize = sizeof(Buffer);
        
        // TODO(kstandbridge): So when using Tls/ssl we recieve data in chunks that are decrypted and should be able to just read the entire site at once
        // without we need to know in advance the size to read so we are pulling from the headers etc
        if(Win32TlsDecrypt(Arena, Client->Socket, Client->Creds, Client->Context, Buffer, BufferSize))
        {
            int x = 5;
        }
        else
        {
            int x = 5;
        }
        
#if 0
        char Buffer[64];
        u32 BufferSize = sizeof(Buffer);
        s32 At = 0;
        for(;;)
        {
            s32 Recieved = Win32Recv(Client->Socket, Buffer + At, 1, 0);
            if(Recieved == -1)
            {
                // TODO(kstandbridge): Socket error
                Result.Code = HttpResponseCode_SocketError;
                break;
            }
            else if(Recieved == 0)
            {
                // TODO(kstandbridge): Server closed socket
                Result.Code = HttpResponseCode_SocketError;
                break;
            }
            else
            {
                if((Buffer[At - 1] == '\r') &&
                   (Buffer[At] == '\n'))
                {
                    if((Buffer[0] == '\r') &&
                       (Buffer[1] == '\n'))
                    {
                        // NOTE(kstandbridge): Header is finished.
                        u32 LengthRemaining = Result.ContentLength;
                        format_string_state StringState = BeginFormatString(Arena);
                        while(LengthRemaining > 0)
                        {
                            u32 LengthToGet = (LengthRemaining > BufferSize) ? BufferSize : LengthRemaining;
                            Recieved = Win32Recv(Client->Socket, Buffer, LengthToGet, 0);
                            if(Recieved == -1)
                            {
                                // TODO(kstandbridge): Socket error
                                Result.Code = HttpResponseCode_SocketError;
                                break;
                            }
                            else if(Recieved == 0)
                            {
                                // TODO(kstandbridge): Server closed socket
                                Result.Code = HttpResponseCode_SocketError;
                                break;
                            }
                            else
                            {
                                Buffer[Recieved] = '\0';
                                AppendStringFormat(&StringState, "%s", Buffer);
                                LengthRemaining-= Recieved;
                            }
                        }
                        Result.Content = EndFormatString(&StringState);
                        break;
                    }
                    else
                    {
                        Buffer[At - 1] = '\0';
                        // NOTE(kstandbridge): We recieved a line
                        if(StringBeginsWith(String_(At, (u8 *)Buffer), String("HTTP/")))
                        {
                            string ResponseMessage;
                            ZeroStruct(ResponseMessage);
                            ParseFromString(Buffer, "HTTP/%d.%d %d %S",
                                            &Result.Major, &Result.Minor, &Result.Code, &ResponseMessage);
                            
                            Result.Message = PushString_(Arena, ResponseMessage.Size, ResponseMessage.Data);
                        }
                        else if(StringBeginsWith(String_(At, (u8 *)Buffer), String("Content-Type")))
                        {
                            string ContentType = String_(At - 15, (u8 *)(Buffer + 14));
                            if(StringsAreEqual(ContentType, String("text/html")))
                            {
                                Result.ContentType = HttpResponseContent_HTML;
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else if(StringBeginsWith(String_(At, (u8 *)Buffer), String("Content-Length")))
                        {
                            Result.ContentLength = U32FromString(Buffer + 16);
                        }
                        else
                        {
                            // TODO(kstandbridge): Process this line
                            int x = 5;
                        }
                    }
                    At = 0;
                }
                else
                {
                    ++At;
                }
            }
        }
#endif
        
    }
    else
    {
        Result.Code = HttpResponseCode_SocketError;
    }
    
    return Result;
}

inline http_response
EndHttpRequest(http_request *Request, memory_arena *Arena)
{
    http_client *Client = Request->Client;
    
    Request->Raw = GenerateHttpMessage_(Request);
    
    http_response Result = Client->IsSsl ? GetSslHttpResponse(Client, Arena, Request->Raw) : GetHttpResponse(Client, Arena, Request->Raw);
    
    
    return Result;
}

