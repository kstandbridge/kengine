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
    
#if defined(KENGINE_CONSOLE)
    u32 ArgCount;
    char **Args;
#else
    char *CmdLine;
#endif // defined(KENGINE_CONSOLE)
    
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

#define PlatformDeleteDirectory(Path) Win32DeleteDirectory(Path)
b32
Win32DeleteDirectory(string Path);

#define PlatformCreateDirectory(Path) Win32CreateDirectory(Path)
b32
Win32CreateDirectory(string Path);

#define PlatformFileExists(Path) Win32FileExists(Path)
b32
Win32FileExists(string Path);

typedef void platform_execute_process_callback(void *Context, string Message);
#define PlatformExecuteProcessWithOutput(Path, Args, WorkingDirectory, Callback, Context, ProcessHandle) Win32ExecuteProcessWithOutput(Path, Args, WorkingDirectory, Callback, Context, ProcessHandle)
u32
Win32ExecuteProcessWithOutput(string Path, string Args, string WorkingDirectory, platform_execute_process_callback *Callback, void *Context, void **ProcessHandle);

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

#define PlatformConsoleOut_(Format, ArgsList) Win32ConsoleOut_(Format, ArgsList)
internal void
Win32ConsoleOut_(char *Format, va_list ArgsList);

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

#define PlatformRequestCloseProcess(FileName) Win32RequestCloseProcess(FileName)
b32
Win32RequestCloseProcess(string FileName);

#define PlatformKillProcess(FileName) Win32KillProcess(FileName)
b32
Win32KillProcess(string FileName);

#define PlatformCheckForProcess(FileName) Win32CheckForProcess(FileName)
b32
Win32CheckForProcess(string FileName);

#define PlatformExecuteProcess(Path, Args, WorkingDirectory) Win32ExecuteProcess(Path, Args, WorkingDirectory)
void
Win32ExecuteProcess(string Path, string Args, string WorkingDirectory);

#define PlatformMakeWorkQueue(Arena, ThreadCount) Win32MakeWorkQueue(Arena, ThreadCount)
platform_work_queue *
Win32MakeWorkQueue(memory_arena *Arena, u32 ThreadCount);

#define PlatformCompleteAllWork(PlatformQueue) Win32CompleteAllWork(PlatformQueue)
void
Win32CompleteAllWork(platform_work_queue *PlatformQueue);

#define PlatformAddWorkEntry(PlatformQueue, Callback, Context) Win32AddWorkEntry(PlatformQueue, Callback, Context)
void
Win32AddWorkEntry(platform_work_queue *PlatformQueue, platform_work_queue_callback *Callback, void *Context);

typedef platform_http_response http_request_callback(app_memory *Memory, memory_arena *Arena, platform_http_request Request);

#define PlatformBeginHttpServer(Arena, Callback) Win32BeginHttpServer(Arena, Callback)
void *
Win32BeginHttpServer(memory_arena *Arena, http_request_callback *Callback);

#define PlatformEndHttpServer(Handle) Win32EndHttpServer(Handle)
void
Win32EndHttpServer(void *Handle);

#define PlatformSetClipboardText(Buffer, Length) Win32SetClipboardText(Buffer, Length)
b32
Win32SetClipboardText(char *Buffer, s64 Length);

#define PlatformSetWindowSize(Size) Win32SetWindowSize(Size)
b32
Win32SetWindowSize(v2 Size);

// NOTE(kstandbridge): UxTheme.dll
typedef enum IMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
} IMMERSIVE_HC_CACHE_MODE;

typedef enum PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
} PreferredAppMode;

typedef enum WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
} WINDOWCOMPOSITIONATTRIB;

typedef struct WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;

typedef BOOL set_window_composition_attribute(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
typedef b32 should_apps_use_dark_mode(); // ordinal 132
typedef b32 allow_dark_mode_for_window(HWND hWnd, b32 allow); // ordinal 133
typedef b32 allow_dark_mode_for_app(b32 allow); // ordinal 135, in 1809
typedef void refresh_immersive_color_policy_state(); // ordinal 104
typedef b32 is_dark_mode_allowed_for_window(HWND hWnd); // ordinal 137
typedef b32 get_is_immersive_color_using_high_contrast(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
typedef HTHEME open_nc_theme_data(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
typedef PreferredAppMode set_preferred_app_mode(PreferredAppMode appMode); // ordinal 135, in 1903
typedef HTHEME open_theme_data(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT close_theme_data(HTHEME hTheme);
typedef HRESULT set_window_theme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
typedef HRESULT get_theme_color(HTHEME hTheme, s32 iPartId, s32 iStateId, s32 iPropId, COLORREF *pColor);

typedef struct dark_api
{
    HMODULE UxThemeLibrary;
    set_window_composition_attribute *SetWindowCompositionAttribute;
    should_apps_use_dark_mode *ShouldAppsUseDarkMode;
    allow_dark_mode_for_window *AllowDarkModeForWindow;
    allow_dark_mode_for_app *AllowDarkModeForApp;
    refresh_immersive_color_policy_state *RefreshImmersiveColorPolicyState;
    is_dark_mode_allowed_for_window *IsDarkModeAllowedForWindow;
    get_is_immersive_color_using_high_contrast *GetIsImmersiveColorUsingHighContrast;
    open_nc_theme_data *OpenNcThemeData;
    set_preferred_app_mode *SetPreferredAppMode;
    open_theme_data *OpenThemeData;
    close_theme_data *CloseThemeData;
    set_window_theme *SetWindowTheme;
    get_theme_color *GetThemeColor;
    b32 IsDarkModeSupported;
    b32 IsDarkModeEnabled;
} dark_api;

#define KENGINE_WIN32_H
#endif //KENGINE_WIN32_H
