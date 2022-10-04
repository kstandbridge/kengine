typedef enum telemetry_field_type
{
    TelemetryField_String,
    TelemetryField_Number
} telemetry_field_type;

typedef struct telemetry_field
{
    struct telemetry_field *Next;
    
    telemetry_field_type Type;
    
    string Key;
    union {
        string StringValue;
        f64 NumberValue;
    };
    
} telemetry_field;

typedef struct telemetry_message
{
    struct telemetry_message *Next;
    
    date_time DateTime;
    
    telemetry_field *Fields;
    string Payload;
    string Endpoint;
} telemetry_message;

typedef enum telemetry_state_type
{
    TelemetryState_Uninitialized,
    TelemetryState_Initializing,
    TelemetryState_Idle,
    TelemetryState_AddingHost,
    TelemetryState_AddingToQueue,
    TelemetryState_SwappingQueues
} telemetry_state_type;

typedef struct telemetry_host
{
    struct telemetry_host *Next;
    string Hostname;
} telemetry_host;

typedef struct telemetry_queue
{
    memory_arena Arena;
    temporary_memory MemoryFlush;
    
    telemetry_message *Messages;
} telemetry_queue;

typedef struct telemetry_state
{
    telemetry_state_type CurrentState;
    
    telemetry_host *Hosts;
    
    string TeamId;
    string ProductName;
    string MachineName;
    string Username;
    u32 ProcessId;
    
    b32 TelemetryThreadRunning;
    
    telemetry_queue *AddingQueue;
    telemetry_queue *ProcessingQueue;
} telemetry_state;

telemetry_state GlobalTelemetryState_;

internal void
PostTelemetryThread(void *Data)
{
    Assert(!GlobalTelemetryState_.TelemetryThreadRunning);
    GlobalTelemetryState_.TelemetryThreadRunning = true;
    
    for(;;)
    {
        // NOTE(kstandbridge): Swap queues
        for(;;)
        {
            u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState_.CurrentState, TelemetryState_SwappingQueues, TelemetryState_Idle);
            if(StateType == TelemetryState_Idle)
            {
                telemetry_queue *Temp = GlobalTelemetryState_.AddingQueue;
                GlobalTelemetryState_.AddingQueue = GlobalTelemetryState_.ProcessingQueue;
                GlobalTelemetryState_.ProcessingQueue = Temp;
                
                CompletePreviousWritesBeforeFutureWrites;
                GlobalTelemetryState_.CurrentState = TelemetryState_Idle;
                
                break;
            }
            else
            {
                _mm_pause();
            }
        }
        
        telemetry_queue *Queue = GlobalTelemetryState_.ProcessingQueue;
        
        // NOTE(kstandbridge): Post all telemetry messages
        if(GlobalTelemetryState_.ProcessingQueue->Messages)
        {
            for(telemetry_host *Host = GlobalTelemetryState_.Hosts;
                Host;
                Host = Host->Next)
            {
                for(telemetry_message *Message = Queue->Messages;
                    Message;
                    Message = Message->Next)
                {
#if KENGINE_INTERNAL
                    u8 *Buffer = BeginPushSize(&Queue->Arena);
                    umm Size = Win32SendInternetRequest(Host->Hostname, 0, Message->Endpoint, "POST", Message->Payload,
                                                        Buffer, Queue->Arena.Size - Queue->Arena.Used, 
                                                        "Content-Type: application/json;\r\n", 0, 0);
                    EndPushSize(&Queue->Arena, Size);
                    string Response = String_(Size, Buffer);
                    int x = 5;
#else
                    Win32SendInternetRequest(Host->Hostname, 0, Message->Endpoint, "POST", Message->Payload,
                                             0, 0, "Content-Type: application/json;\r\n", 0, 0);
#endif
                }
            }
        }
        
        EndTemporaryMemory(Queue->MemoryFlush);
        CheckArena(&Queue->Arena);
        Assert(Queue->Arena.Used == 0);
        
        Queue->MemoryFlush = BeginTemporaryMemory(&Queue->Arena);
        Queue->Messages = 0;
        
        if(!GlobalTelemetryState_.TelemetryThreadRunning)
        {
            break;
        }
        
        Win32Sleep(500);
    }
}

inline void
EndTelemetryThread()
{
    Assert(GlobalTelemetryState_.TelemetryThreadRunning);
    GlobalTelemetryState_.TelemetryThreadRunning = false;
}

internal void
BeginTelemetryMessage()
{
    for(;;)
    {
        f32 PercentFree = ((f32)GlobalTelemetryState_.AddingQueue->Arena.Used / (f32)GlobalTelemetryState_.AddingQueue->Arena.Size);
        
        if(PercentFree < 0.9f)
        {
            u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState_.CurrentState, TelemetryState_AddingToQueue, TelemetryState_Idle);
            if(StateType == TelemetryState_Idle)
            {
                telemetry_message *Message = GlobalTelemetryState_.AddingQueue->Messages;
                if(Message != 0)
                {
                    Message = PushStruct(&GlobalTelemetryState_.AddingQueue->Arena, telemetry_message);
                    Message->Next = GlobalTelemetryState_.AddingQueue->Messages;
                }
                else
                {
                    Message = PushStruct(&GlobalTelemetryState_.AddingQueue->Arena, telemetry_message);
                }
                GlobalTelemetryState_.AddingQueue->Messages = Message;
                
                Message->DateTime = Win32GetDateTime();
                
                break;
            }
        }
        else
        {
            _mm_pause();
        }
    }
}

internal void
AppendTelemetryMessageNumberField(string Key, f32 Value)
{
    Assert(GlobalTelemetryState_.CurrentState == TelemetryState_AddingToQueue);
    Assert(Key.Data);
    
    telemetry_queue *Queue = GlobalTelemetryState_.AddingQueue;
    
    telemetry_message *Message = Queue->Messages;
    
    telemetry_field *Field = Message->Fields;
    if(Field != 0)
    {
        Field = PushStruct(&Queue->Arena, telemetry_field);
        Field->Next = Message->Fields;
    }
    else
    {
        Field = PushStruct(&Queue->Arena, telemetry_field);
    }
    Message->Fields = Field;
    
    Field->Type = TelemetryField_Number;
    Field->Key = PushString_(&Queue->Arena, Key.Size, Key.Data);
    Field->NumberValue = Value;
}


internal void
AppendTelemetryMessageStringField(string Key, string Value)
{
    Assert(GlobalTelemetryState_.CurrentState == TelemetryState_AddingToQueue);
    Assert(Key.Data);
    Assert(Value.Data);
    
    telemetry_queue *Queue = GlobalTelemetryState_.AddingQueue;
    
    telemetry_message *Message = Queue->Messages;
    
    telemetry_field *Field = Message->Fields;
    if(Field != 0)
    {
        Field = PushStruct(&Queue->Arena, telemetry_field);
        Field->Next = Message->Fields;
    }
    else
    {
        Field = PushStruct(&Queue->Arena, telemetry_field);
    }
    Message->Fields = Field;
    
    Field->Type = TelemetryField_String;
    Field->Key = PushString_(&Queue->Arena, Key.Size, Key.Data);
    Field->StringValue = PushString_(&Queue->Arena, Value.Size, Value.Data);
}

internal void
EndTelemetryMessage()
{
    Assert(GlobalTelemetryState_.CurrentState == TelemetryState_AddingToQueue);
    
    telemetry_message *Message = GlobalTelemetryState_.AddingQueue->Messages;
    
    AppendTelemetryMessageStringField(String("product_name"), GlobalTelemetryState_.ProductName);
    AppendTelemetryMessageStringField(String("machine_name"), GlobalTelemetryState_.MachineName);
    AppendTelemetryMessageStringField(String("user_name"), GlobalTelemetryState_.Username);
    AppendTelemetryMessageNumberField(String("pid"), GlobalTelemetryState_.ProcessId);
    
    format_string_state StringState = BeginFormatString(&GlobalTelemetryState_.AddingQueue->Arena);
    
    AppendStringFormat(&StringState, "{\n    \"@timestamp\": \"%d-%02d-%02dT%02d:%02d:%02d.%03dZ\"",
                       Message->DateTime.Year, Message->DateTime.Month, Message->DateTime.Day,
                       Message->DateTime.Hour, Message->DateTime.Minute, Message->DateTime.Second,
                       Message->DateTime.Milliseconds);
    
    for(telemetry_field *Field = Message->Fields;
        Field;
        Field = Field->Next)
    {
        if((Field->Key.Data) && 
           (Field->Key.Data != '\0'))
        {
            if(Field->Type == TelemetryField_Number)
            {
                AppendStringFormat(&StringState, ",\n    \"%S\": %.03f", Field->Key, Field->NumberValue);
            }
            else
            {
                Assert(Field->Type == TelemetryField_String);
                AppendStringFormat(&StringState, ",\n    \"%S\": \"%S\"", Field->Key, Field->StringValue);
            }
            
        }
        else
        {
            Assert(!"Blank key");
        }
    }
    
    AppendStringFormat(&StringState, "\n}");
    
    Message->Payload = EndFormatString(&StringState);
    
    Message->Endpoint = FormatString(&GlobalTelemetryState_.AddingQueue->Arena,
                                     "/%S_%S_%d-%02d/_doc/%S_%S_%d_%d%02d%02d%02d%02d%02d%03d?pretty",
                                     GlobalTelemetryState_.TeamId,
                                     GlobalTelemetryState_.ProductName,
                                     Message->DateTime.Year, Message->DateTime.Month,
                                     GlobalTelemetryState_.ProductName,
                                     GlobalTelemetryState_.MachineName,
                                     GlobalTelemetryState_.ProcessId,
                                     Message->DateTime.Year, Message->DateTime.Month, Message->DateTime.Day,
                                     Message->DateTime.Hour, Message->DateTime.Minute, Message->DateTime.Second,
                                     Message->DateTime.Milliseconds);
    
    CompletePreviousWritesBeforeFutureWrites;
    GlobalTelemetryState_.CurrentState = TelemetryState_Idle;
}

internal void
AddTelemetryHost(memory_arena *Arena, string Hostname)
{
    
    for(;;)
    {
        u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState_.CurrentState, TelemetryState_AddingHost, TelemetryState_Idle);
        if(StateType == TelemetryState_Idle)
        {
            
            telemetry_host *Host = GlobalTelemetryState_.Hosts;
            if(Host != 0)
            {
                Host = PushStruct(Arena, telemetry_host);
                Host->Next = GlobalTelemetryState_.Hosts;
            }
            else
            {
                Host = PushStruct(Arena, telemetry_host);
            }
            GlobalTelemetryState_.Hosts = Host;
            Host->Hostname = PushString_(Arena, Hostname.Size, Hostname.Data);
            
            CompletePreviousWritesBeforeFutureWrites;
            GlobalTelemetryState_.CurrentState = TelemetryState_Idle;
            break;
        }
        else
        {
            _mm_pause();
        }
    }
}

internal void
InitializeTelemetry(memory_arena *Arena, umm QueueSize, string TeamId, string ProductName, string MachineName, string Username, u32 ProcessId)
{
    if(GlobalTelemetryState_.CurrentState == TelemetryState_Uninitialized)
    {
        for(;;)
        {
            u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState_.CurrentState, TelemetryState_Initializing, TelemetryState_Uninitialized);
            if(StateType == TelemetryState_Uninitialized)
            {
                GlobalTelemetryState_.AddingQueue = PushStruct(Arena, telemetry_queue);
                SubArena(&GlobalTelemetryState_.AddingQueue->Arena, Arena, QueueSize);
                GlobalTelemetryState_.AddingQueue->MemoryFlush = BeginTemporaryMemory(&GlobalTelemetryState_.AddingQueue->Arena);
                
                GlobalTelemetryState_.ProcessingQueue = PushStruct(Arena, telemetry_queue);
                SubArena(&GlobalTelemetryState_.ProcessingQueue->Arena, Arena, QueueSize);
                GlobalTelemetryState_.ProcessingQueue->MemoryFlush = BeginTemporaryMemory(&GlobalTelemetryState_.ProcessingQueue->Arena);
                
                GlobalTelemetryState_.TeamId = PushString_(Arena, TeamId.Size, TeamId.Data);
                GlobalTelemetryState_.ProductName = PushString_(Arena, ProductName.Size, ProductName.Data);
                GlobalTelemetryState_.MachineName = PushString_(Arena, MachineName.Size, MachineName.Data);
                GlobalTelemetryState_.Username = PushString_(Arena, Username.Size, Username.Data);
                GlobalTelemetryState_.ProcessId = ProcessId;
                
                GlobalTelemetryState_.CurrentState = TelemetryState_Idle;
                
                break;
            }
            else
            {
                _mm_pause();
            }
        }
    }
    else
    {
        Assert(!"Invalid telemetry state");
    }
}

inline void
SendHeartbeatTelemetry()
{
    BeginTelemetryMessage();
    AppendTelemetryMessageNumberField(String("cl"), VERSION);
    AppendTelemetryMessageStringField(String("category"), String("/heartbeat"));
    AppendTelemetryMessageStringField(String("message"), String("heartbeat"));
    EndTelemetryMessage();
}

#define SendLogTelemetry___(SourceFilePlusLine, Level, Format, ...) SendLogTelemetry____(String(SourceFilePlusLine), Level, Format, __VA_ARGS__)
#define SendLogTelemetry__(File, Line, Level, Format, ...) SendLogTelemetry___(File "(" #Line ")", Level, Format, __VA_ARGS__)
#define SendLogTelemetry_(File, Line, Level, Format, ...) SendLogTelemetry__(File, Line, Level, Format, __VA_ARGS__)
#define LogVerbose(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, String("verbose"), Format, __VA_ARGS__)
#define LogInfo(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, String("info"), Format, __VA_ARGS__)
#define LogWarning(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, String("waring"), Format, __VA_ARGS__)
#define LogError(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, String("error"), Format, __VA_ARGS__)

internal void
SendLogTelemetry_____(string SourceFilePlusLine, string Level, string Message)
{
    SourceFilePlusLine.Data += SourceFilePlusLine.Size;
    SourceFilePlusLine.Size = 0;
    while(SourceFilePlusLine.Data[-1] != '\\')
    {
        ++SourceFilePlusLine.Size;
        --SourceFilePlusLine.Data;
    }
    
    BeginTelemetryMessage();
    AppendTelemetryMessageNumberField(String("cl"), VERSION);
    AppendTelemetryMessageStringField(String("source_file_plus_line"), SourceFilePlusLine);
    AppendTelemetryMessageStringField(String("category"), String("/log"));
    AppendTelemetryMessageStringField(String("log_level"), Level);
    AppendTelemetryMessageStringField(String("message"), Message);
    EndTelemetryMessage();
}

internal void
SendLogTelemetry____(string FileLine, string Level, char *Format, ...)
{
    u8 Buffer[4096];
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatStringToBuffer(Buffer);
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Message = EndFormatStringToBuffer(&StringState, BufferSize);
    
    SendLogTelemetry_____(FileLine, Level, Message);
}
