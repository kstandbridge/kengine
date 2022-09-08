// TODO(kstandbridge): there should be NO win32 in this file, replace with platform invocations

inline http_client
BeginHttpClient(memory_arena *Arena, string Hostname, b32 IsSsl)
{
    http_client Result;
    ZeroStruct(Result);
    
    Result.Hostname = Hostname;
    Result.Socket = Win32SslSocketConnect(Arena, Hostname, &Result.Creds, &Result.Context);
    
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
GetSslHttpResponse(http_client *Client, memory_arena *PermArena, memory_arena *TempArena, string Request)
{
    http_response Result;
    ZeroStruct(Result);
    
    if(Win32HttpClientSendData(Client, Request.Data, Request.Size))
    {
        
        b32 ParsingHeader = true;
        
        format_string_state BodyStringState = BeginFormatString(PermArena);
        
        for(;;)
        {
            char *LineStart = 0;
            string Response = BeginPushString(TempArena);
            umm ResponseSize = Win32TlsDecrypt(TempArena, Client->Socket, Client->Creds, Client->Context, Response.Data, TempArena->Size - TempArena->Used);
            EndPushString(&Response, TempArena, ResponseSize);
            
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
                        if(StringBeginsWith(Line, String("\r\n")))
                        {
                            ParsingHeader = false;
                            Response.Size -= LineEnd - (char *)Response.Data;
                            Response.Data = (u8 *)LineEnd + 1;
                            break;
                        }
                        else if(StringBeginsWith(Line, String("HTTP/")))
                        {
                            string ResponseMessage;
                            ZeroStruct(ResponseMessage);
                            
                            // TODO(kstandbridge): Parse from string
#if 0                                
                            ParseFromString(Buffer, "HTTP/%d.%d %d %S",
                                            &Result.Major, &Result.Minor, &Result.Code, &ResponseMessage);
#endif
                            
                            Result.Message = PushString_(PermArena, ResponseMessage.Size, ResponseMessage.Data);
                        }
                        else if(StringBeginsWith(Line, String("Content-Type")))
                        {
                            string ContentType = String_(LineEnd - LineStart - 15, (u8 *)LineStart + 14);
                            if(StringsAreEqual(ContentType, String("text/html")))
                            {
                                Result.ContentType = HttpResponseContent_HTML;
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else if(StringBeginsWith(Line, String("Content-Length")))
                        {
                            Result.ContentLength = U32FromString(LineStart + 16);
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
            
            AppendStringFormat(&BodyStringState, "%S", Response);
            if(StringContains(String("</HTML>"), Response) ||
               StringContains(String("</html>"), Response))
            {
                break;
            }
        }
        
        Result.Content = EndFormatString(&BodyStringState);
    }
    else
    {
        Result.Code = HttpResponseCode_SocketError;
    }
    
    return Result;
}

inline http_response
EndHttpRequest(http_request *Request, memory_arena *PermArena, memory_arena *TempArena)
{
    http_client *Client = Request->Client;
    
    Request->Raw = GenerateHttpMessage_(Request);
    
    http_response Result = GetSslHttpResponse(Client, PermArena, TempArena, Request->Raw);
    
    
    return Result;
}

