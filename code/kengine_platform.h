#ifndef KENGINE_PLATFORM_H

#include <stdarg.h>
#include <immintrin.h>

#define introspect(...)
#include "kengine_types.h"


typedef struct app_button_state
{
    s32 HalfTransitionCount;
    b32 EndedDown;
} app_button_state;

typedef enum
{
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Extended0,
    MouseButton_Extended1,
    
    MouseButton_Count,
} mouse_button_type;

typedef struct app_input
{
    f32 DeltaTime;
    
    app_button_state MouseButtons[MouseButton_Count];
    v2 MouseP;
    f32 MouseZ;
    
    b32 ShiftDown, AltDown, ControlDown;
    b32 FKeyPressed[13];
} app_input;

inline b32 
WasPressed(app_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
    
    return Result;
}

typedef enum platform_memory_block_flags
{
    PlatformMemoryBlockFlag_OverflowCheck = 0x1,
    PlatformMemoryBlockFlag_UnderflowCheck = 0x2
} platform_memory_block_flags;

typedef struct platform_memory_block
{
    u64 Flags;
    u64 Size;
    u8 *Base;
    umm Used;
    
    struct platform_memory_block *Prev;
} platform_memory_block;

typedef void platform_work_queue_callback(void *Data);

typedef struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
} platform_work_queue_entry;

typedef struct platform_work_queue 
{
    platform_work_queue_entry Entries[256];
} platform_work_queue;

typedef platform_work_queue *platform_make_work_queue(struct memory_arena *Arena, u32 ThreadCount);
typedef void platform_add_work_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

#if KENGINE_CONSOLE
typedef void platform_init_console_command_loop(platform_work_queue *Queue);
#endif

typedef platform_memory_block *platform_allocate_memory(umm Size, u64 Flags);
typedef void platform_deallocate_memory(platform_memory_block *Block);

typedef struct platform_memory_stats
{
    u32 BlockCount;
    umm TotalSize;
    umm TotalUsed;
} platform_memory_stats;

typedef platform_memory_stats platform_get_memory_stats();

// TODO(kstandbridge): platform_
typedef loaded_glyph get_glyph_for_code_point(struct memory_arena *Arena, u32 CodePoint);
typedef f32 get_horizontal_advance(u32 PrevCodePoint, u32 CodePoint);
typedef f32 get_verticle_advance();


// TODO(kstandbridge): move this somewhere?
typedef void platform_delete_http_cache_entry(char *Format, ...);
typedef enum http_verb_type
{
    HttpVerb_Post,
    HttpVerb_Get,
    HttpVerb_Put,
    HttpVerb_Patch,
    HttpVerb_Delete,
} http_verb_type;

typedef struct platform_http_client
{
    void *Handle;
    
    string Hostname;
    u32 Port;
    b32 IsHttps;
    b32 NoErrors;
} platform_http_client;

typedef void platform_end_http_client(platform_http_client *PlatformClient);
typedef platform_http_client platform_begin_http_client_with_creds(string Hostname, u32 Port, string Username, string Password);
typedef platform_http_client platform_begin_http_client(string Hostname, u32 Port);
typedef void platform_skip_http_metrics(platform_http_client *PlatformClient);
typedef void platform_set_http_client_timeout(platform_http_client *PlatformClient, u32 TimeoutMs);

typedef struct platform_http_request
{
    void *Handle;
    b32 NoErrors;
    string Payload;
    
    string Hostname;
    u32 Port;
    string Verb;
    string Template;
    string Endpoint;
    umm RequestLength;
    umm ResponseLength;
    u32 StatusCode;
    
} platform_http_request;

typedef void platform_end_http_request(platform_http_request *PlatformRequest);
typedef platform_http_request platform_begin_http_request(platform_http_client *PlatformClient,  http_verb_type Verb, char *Format, ...);

typedef struct platform_http_response
{
    u16 StatusCode;
    string Payload;
} platform_http_response;

typedef void platform_set_http_request_headers(platform_http_request *Request, string Headers);

typedef u32 platform_send_http_request(platform_http_request *Request);
typedef u32 platform_send_http_request_from_file(platform_http_request *PlatformRequest, string File);

typedef umm platform_get_http_response_to_file(platform_http_request *PlatformRequest, string File);
typedef string platform_get_http_response(platform_http_request *PlatformRequest);

#define PlatformNoErrors(Handle) ((Handle).NoErrors)

typedef string platform_read_entire_file(struct memory_arena *Arena, string FilePath);
typedef b32 platform_write_text_to_file(string Text, string FilePath);
typedef void platform_zip_directory(string SourceDirectory, string DestinationZip);
typedef void platform_unzip_to_directory(string SourceZip, string DestFolder);
typedef b32 platform_file_exists(string Path);
typedef b32 platform_permanent_delete_file(string Path);
typedef b32 platform_directory_exists(string Path);
typedef b32 platform_create_directory(string Path);
typedef b32 platform_request_close_process(string Name);
typedef b32 platform_kill_process(string Name);
typedef b32 platform_check_for_process(string Name);
typedef void platform_execute_process(string Path, string Args, string WorkingDirectory);

typedef void platform_execute_process_callback(string Message);
typedef u32 platform_execute_process_with_output(string Path, string Args, string WorkingDirectory, platform_execute_process_callback *Output);

typedef umm platform_get_hostname(u8 *Buffer, umm BufferMaxSize);
typedef umm platform_get_username(u8 *Buffer, umm BufferMaxSize);
typedef umm platform_get_home_directory(u8 *Buffer, umm BufferMaxSize);
typedef umm platform_get_app_config_directory(u8 *Buffer, umm BufferMaxSize);
typedef u32 platform_get_process_id();
typedef u64 platform_get_system_timestamp();
typedef date_time platform_get_date_time_for_timestamp(u64 Timestamp);
typedef void platform_console_out(char *Format, ...);

typedef struct platform_api
{
    platform_make_work_queue *MakeWorkQueue;
    platform_add_work_entry *AddWorkEntry;
    platform_complete_all_work *CompleteAllWork;
    
#if KENGINE_CONSOLE
    platform_init_console_command_loop *InitConsoleCommandLoop;
#endif
    
    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;
    platform_get_memory_stats *GetMemoryStats;
    
    // TODO(kstandbridge): platform_
    get_glyph_for_code_point *GetGlyphForCodePoint;
    get_horizontal_advance *GetHorizontalAdvance;
    get_verticle_advance *GetVerticleAdvance;
    
    platform_delete_http_cache_entry *DeleteHttpCache;
    platform_end_http_client *EndHttpClient;
    platform_begin_http_client_with_creds *BeginHttpClientWithCreds;
    platform_begin_http_client *BeginHttpClient;
    platform_skip_http_metrics *SkipHttpMetrics;
    platform_set_http_client_timeout *SetHttpClientTimeout;
    platform_end_http_request *EndHttpRequest;
    platform_begin_http_request *BeginHttpRequest;
    platform_set_http_request_headers *SetHttpRequestHeaders;
    platform_send_http_request *SendHttpRequest;
    platform_send_http_request_from_file *SendHttpRequestFromFile;
    platform_get_http_response_to_file *GetHttpResponseToFile;
    platform_get_http_response *GetHttpResponse;
    
    platform_read_entire_file *ReadEntireFile;
    platform_write_text_to_file *WriteTextToFile;
    platform_zip_directory *ZipDirectory;
    platform_unzip_to_directory *UnzipToDirectory;
    platform_file_exists *FileExists;
    platform_permanent_delete_file *PermanentDeleteFile;
    platform_directory_exists *DirectoryExists;
    platform_create_directory *CreateDirectory;
    platform_request_close_process *RequestCloseProcess;
    platform_kill_process *KillProcess;
    platform_check_for_process *CheckForProcess;
    platform_execute_process *ExecuteProcess;
    platform_execute_process_with_output *ExecuteProcessWithOutput;
    platform_get_hostname *GetHostname;
    platform_get_username *GetUsername;
    platform_get_home_directory *GetHomeDirectory;
    platform_get_app_config_directory *GetAppConfigDirectory;
    platform_get_process_id *GetProcessId;
    platform_get_system_timestamp *GetSystemTimestamp;
    platform_get_date_time_for_timestamp *GetDateTimeFromTimestamp;
    platform_console_out *ConsoleOut;
    
} platform_api;
extern platform_api Platform;

// TODO(kstandbridge): Rename to platform_state ???
typedef struct app_memory
{
    struct app_state *AppState;
    struct ui_state *UiState;
    
#if KENGINE_INTERNAL
    struct telemetry_state *TelemetryState;
    struct debug_state *DebugState;
    struct debug_event_table *DebugEventTable;
#endif
    u32 ArgCount;
    string *Args;
    b32 IsRunning;
    
    platform_api PlatformAPI;
} app_memory;

typedef void app_handle_command(app_memory *Memory, string Command, u32 ArgCount, string *Args);

#if defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
typedef void app_tick_(app_memory *Memory, f32 dtForFrame);
#else
typedef void app_tick_(app_memory *Memory, struct render_group *RenderGroup, app_input *Input);
#endif

#if KENGINE_HTTP
typedef platform_http_response app_handle_http_request(app_memory *Memory, struct memory_arena *Arena, platform_http_request Request);
#endif

inline date_time
GetDateTime()
{
    u64 Timestamp = Platform.GetSystemTimestamp();
    date_time Result = Platform.GetDateTimeFromTimestamp(Timestamp);
    
    return Result;
}

#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H
