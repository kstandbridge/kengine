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
GetSslHttpResponse(http_client *Client, memory_arena *PermArena, memory_arena *TempArena, string Request)
{
    http_response Result;
    ZeroStruct(Result);
    
    if(Win32SendEncryptedMessage(Client->Socket, Request.Data, Request.Size, Client->Context))
    {
        b32 ParsingHeader = true;
        
        format_string_state BodyStringState = BeginFormatString(PermArena);
        
        for(;;)
        {
            char *LineStart = 0;
            string Response = BeginPushString(TempArena);
            umm ResponseSize = Win32RecieveDecryptedMessage(TempArena, Client->Socket, Client->Creds, Client->Context, Response.Data, TempArena->Size - TempArena->Used);
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
                        if(StringBeginsWith(String("\r\n"), Line))
                        {
                            ParsingHeader = false;
                            Response.Size -= LineEnd - (char *)Response.Data;
                            Response.Data = (u8 *)LineEnd + 1;
                            break;
                        }
                        else if(StringBeginsWith(String("HTTP/"), Line))
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
                        else if(StringBeginsWith(String("Content-Type"), Line))
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
                        else if(StringBeginsWith(String("Content-Length"), Line))
                        {
                            Result.ContentLength = U32FromString(LineStart + 16);
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
            
            AppendStringFormat(&BodyStringState, "%S", Response);
            if(StringContains(String("</HTML>"), Response) ||
               StringContains(String("</html>"), Response))
            {
                break;
            }
        }
        
        Result.Content = EndFormatString(&BodyStringState);
        
        if(Result.TransferEncoding == HttpTransferEncoding_Chunked)
        {
            string Content = Result.Content;
            
            // NOTE(kstandbridge): I will never understand why HTML randomly has null terminators within it
            {            
                for(umm Index = 0;
                    Index < Content.Size;
                    ++Index)
                {
                    if(Content.Data[Index] == '\0')
                    {
                        StringDeleteCharactersAt(&Content, Index, 1);
                    }
                }
            }
            
            u32 ChunkSize = ((GetHex(Content.Data[0]) << 12) |
                             (GetHex(Content.Data[1]) << 8) |
                             (GetHex(Content.Data[2]) << 4) |
                             (GetHex(Content.Data[3]) << 0));
            StringDeleteCharactersAt(&Content, 0, 6);
            
            u32 At = 0;
            while(ChunkSize)
            {
                At += ChunkSize;
                while((Content.Data[At + 0] != '\r') &&
                      (Content.Data[At + 1] != '\n'))
                {
                    ++At;
                }
                StringDeleteCharactersAt(&Content, At, 2);
                
                char Hex[4];
                Hex[0] = '0';
                Hex[1] = '0';
                Hex[2] = '0';
                Hex[3] = '0';
                
                u32 HexLength = 0;
                char *Parse = (char *)Content.Data + At;
                while((Parse[0] != '\r') &&
                      (Parse[1] != '\n'))
                {
                    ++HexLength;
                    ++Parse;
                }
                if(HexLength > 4)
                {
                    HexLength = 4;
                }
                u32 Current = 3;
                for(u32 Index = 0;
                    Index < HexLength;
                    ++Index)
                {
                    --Parse;
                    Hex[Current--] = *Parse;
                }
                
                ChunkSize = ((GetHex(Hex[0]) << 12) |
                             (GetHex(Hex[1]) << 8) |
                             (GetHex(Hex[2]) << 4) |
                             (GetHex(Hex[3]) << 0));
                
                
                StringDeleteCharactersAt(&Content, At, HexLength);
                Assert(Content.Data[At + 0] == '\r');
                Assert(Content.Data[At + 1] == '\n');
                StringDeleteCharactersAt(&Content, At, 2);
            }
            
            Result.Content = Content;
            Result.ContentLength = Result.Content.Size;
            
            
#if 1 
            for(umm Index = 0;
                Index < Content.Size;
                ++Index)
            {
                if(Content.Data[Index] == '\0')
                {
                    __debugbreak();
                }
                
                char C[2];
                C[0] = Content.Data[Index];
                C[1] = '\0';
                
                Win32OutputDebugStringA(C);
            }
#endif
            
        }
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

