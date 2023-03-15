#ifndef KENGINE_WIN32_H


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
    
    s64 PerfCountFrequency;
    
} win32_state;

global win32_state GlobalWin32State;

#define PlatformAllocateMemory(Size, Flags) Win32AllocateMemory(Size, Flags)
platform_memory_block *
Win32AllocateMemory(umm Size, u64 Flags);

#define PlatformDeallocateMemory(Block) Win32DeallocateMemory(Block)
void
Win32DeallocateMemory(platform_memory_block *Block);

#define PlatformGetAppConfigDirectory(Buffer, BufferMaxSize) Win32GetAppConfigDirectory(Buffer, BufferMaxSize)
umm
Win32GetAppConfigDirectory(u8 *Buffer, umm BufferMaxSize);


#define PlatformGetHomeDirectory(Buffer, BufferMaxSize) Win32GetHomeDirectory(Buffer, BufferMaxSize)
umm
Win32GetHomeDirectory(u8 *Buffer, umm BufferMaxSize);

#define PlatformDirectoryExists(Path) Win32DirectoryExists(Path)
b32
Win32DirectoryExists(string Path);

#define PlatformCreateDirectory(Path) Win32CreateDirectory(Path)
b32
Win32CreateDirectory(string Path);

#define PlatformFileExists(Path) Win32FileExists(Path)
b32
Win32FileExists(string Path);

typedef void platform_execute_process_callback(string Message);
#define PlatformExecuteProcessWithOutput(Path, Args, WorkingDirectory, Callback) Win32ExecuteProcessWithOutput(Path, Args, WorkingDirectory, Callback)
u32
Win32ExecuteProcessWithOutput(string Path, string Args, string WorkingDirectory, platform_execute_process_callback *Callback);

#define PlatformUnzipToDirectory(SourceZip, DestFolder) Win32UnzipToDirectory(SourceZip, DestFolder)
void
Win32UnzipToDirectory(string SourceZip, string DestFolder);

#define PlatformWriteTextToFile(Text, FilePath) Win32WriteTextToFile(Text, FilePath)
b32
Win32WriteTextToFile(string Text, string FilePath);

#define PlatformPermanentDeleteFile(Path) Win32PermanentDeleteFile(Path)
b32
Win32PermanentDeleteFile(string Path);

#define PlatformGetHostname(Buffer, BufferMaxSize) Win32GetHostname(Buffer, BufferMaxSize)
umm
Win32GetHostname(u8 *Buffer, umm BufferMaxSize);

#define PlatformGetUsername(Buffer, BufferMaxSize) Win32GetUsername(Buffer, BufferMaxSize)
umm
Win32GetUsername(u8 *Buffer, umm BufferMaxSize);

#define PlatformGetProcessId() Win32GetProcessId()
u32
Win32GetProcessId();

#define PlatformZipDirectory(SourceDirectory, DestinationZip) Win32ZipDirectory(SourceDirectory, DestinationZip)
void
Win32ZipDirectory(string SourceDirectory, string DestinationZip);

#define PlatformGetSystemTimestamp() Win32GetSystemTimestamp();
u64
Win32GetSystemTimestamp();

#define PlatformGetDateTimeFromTimestamp(Timestamp) Win32GetDateTimeFromTimestamp(Timestamp)
date_time
Win32GetDateTimeFromTimestamp(u64 Timestamp);


#define PlatformReadEntireFile(Arena, FilePath) Win32ReadEntireFile(Arena, FilePath)

#define PlatformConsoleOut(Format, ...) Win32ConsoleOut(Format, __VA_ARGS__)
internal void
Win32ConsoleOut(char *Format, ...);

string
Win32ReadEntireFile(memory_arena *Arena, string FilePath);


#define PlatformDeleteHttpCache(Format, ...) Win32DeleteHttpCache(Format, __VA_ARGS__)
void
Win32DeleteHttpCache(char *Format, ...);


#define PlatformBeginHttpClientWithCreds(Hostname, Port, Username, Password) Win32BeginHttpClientWithCreds(Hostname, Port, Username, Password)
platform_http_client
Win32BeginHttpClientWithCreds(string Hostname, u32 Port, string Username, string Password);

#define PlatformBeginHttpClient(Hostname, Port) Win32BeginHttpClient(Hostname, Port)
inline platform_http_client
Win32BeginHttpClient(string Hostname, u32 Port)
{
    platform_http_client Result = Win32BeginHttpClientWithCreds(Hostname, Port, NullString(), NullString());
    return Result;
}

#define PlatformSkipHttpMetrics(PlatformClient) Win32SkipHttpMetrics(PlatformClient);
internal void
Win32SkipHttpMetrics(platform_http_client *PlatformClient);

#define PlatformSetHttpClientTimeout(PlatformClient, TimeoutMs) Win32SetHttpClientTimeout(PlatformClient, TimeoutMs);
internal void
Win32SetHttpClientTimeout(platform_http_client *PlatformClient, u32 TimeoutMs);

#define PlatformSetHttpRequestHeaders(PlatformRequest, Headers) Win32SetHttpRequestHeaders(PlatformRequest, Headers)
internal void
Win32SetHttpRequestHeaders(platform_http_request *PlatformRequest, string Headers);

#define PlatformBeginHttpRequest(PlatformClient, Verb, Format, ...) Win32BeginHttpRequest(PlatformClient, Verb, Format, __VA_ARGS__)
platform_http_request
Win32BeginHttpRequest(platform_http_client *PlatformClient, http_verb_type Verb, char *Format, ...);

#define PlatformSendHttpRequest(PlatformRequest) Win32SendHttpRequest(PlatformRequest)
u32
Win32SendHttpRequest(platform_http_request *PlatformRequest);

#define PlatformGetHttpResponse(PlatformRequest) Win32GetHttpResponse(PlatformRequest)
string
Win32GetHttpResponse(platform_http_request *PlatformRequest);

#define PlatformEndHttpRequest(PlatformRequest) Win32EndHttpRequest(PlatformRequest)
void
Win32EndHttpRequest(platform_http_request *PlatformRequest);

#define PlatformEndHttpClient(PlatformClient) Win32EndHttpClient(PlatformClient)
void
Win32EndHttpClient(platform_http_client *PlatformClient);

#define PlatformGetHttpResponseToFile(PlatformRequest, File) Win32GetHttpResponseToFile(PlatformRequest, File)
umm
Win32GetHttpResponseToFile(platform_http_request *PlatformRequest, string File);

#define PlatformSendHttpRequestFromFile(PlatformRequest, File) Win32SendHttpRequestFromFile(PlatformRequest, File)
u32
Win32SendHttpRequestFromFile(platform_http_request *PlatformRequest, string File);

#define KENGINE_WIN32_H
#endif //KENGINE_WIN32_H
