#ifndef WIN32_KENGINE_TYPES_H

#define SECURITY_WIN32

#define COBJMACROS
#include <WS2tcpip.h>
#include <Windows.h>
#include <http.h>
#include <CommCtrl.h>
#include <Shlobj.h>
#include <Uxtheme.h>
#include <vssym32.h>
#include <Wininet.h>
#include <sspi.h>
#include <Schnlsp.h>
#include <tlhelp32.h>

#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

introspect(win32, Gdi32) typedef int add_font_resource_ex_a(LPCSTR name, DWORD fl, PVOID res);
introspect(win32, Gdi32) typedef HDC create_compatible_d_c(HDC hdc);
introspect(win32, Gdi32) typedef HBITMAP create_d_i_b_section(HDC hdc, const BITMAPINFO *pbmi, UINT usage, VOID **ppvBits, HANDLE hSection,DWORD offset);
introspect(win32, Gdi32) typedef HFONT create_font_a(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName);
introspect(win32, Gdi32) typedef BOOL get_char_width_32_w(HDC hdc, UINT iFirst, UINT iLast, LPINT lpBuffer);
introspect(win32, Gdi32) typedef int get_device_caps(HDC hdc, int index);
introspect(win32, Gdi32) typedef DWORD get_kerning_pairs_w(HDC hdc, DWORD nPairs, LPKERNINGPAIR lpKernPair);
introspect(win32, Gdi32) typedef BOOL get_text_extent_point_32_w(HDC hdc, LPCWSTR lpString, int c, LPSIZE psizl);
introspect(win32, Gdi32) typedef BOOL get_text_metrics_a(HDC hdc, LPTEXTMETRICA lptm);
introspect(win32, Gdi32) typedef COLORREF set_bk_color(HDC hdc, COLORREF color);
introspect(win32, Gdi32) typedef int set_bk_mode(HDC hdc, int mode);
introspect(win32, Gdi32) typedef COLORREF set_text_color(HDC hdc, COLORREF color);
introspect(win32, Gdi32) typedef BOOL text_out_w(HDC hdc, int x, int y, LPCWSTR lpString, int c);
introspect(win32, Gdi32) typedef HGDIOBJ select_object(HDC hdc, HGDIOBJ h);
introspect(win32, Gdi32) typedef int stretch_d_i_bits(HDC hdc, int xDest, int yDest, int DestWidth, int DestHeight, int xSrc, int ySrc, int SrcWidth, int SrcHeight, const VOID *lpBits, const BITMAPINFO *lpbmi, UINT iUsage, DWORD rop);
introspect(win32, Gdi32) typedef COLORREF get_pixel(HDC hdc, int x, int y);
introspect(win32, Gdi32) typedef int choose_pixel_format(HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd);
introspect(win32, Gdi32) typedef int describe_pixel_format(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd);
introspect(win32, Gdi32) typedef BOOL set_pixel_format(HDC hdc, int format, const PIXELFORMATDESCRIPTOR *ppfd);
introspect(win32, Gdi32) typedef BOOL swap_buffers(HDC unnamedParam1);

introspect(win32, User32) typedef HWND create_window_ex_a(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, 
                                                          int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, 
                                                          HINSTANCE hInstance, LPVOID lpParam);
introspect(win32, User32) typedef HWND create_window_ex_w(DWORD     dwExStyle,
                                                          LPCWSTR   lpClassName,
                                                          LPCWSTR   lpWindowName,
                                                          DWORD     dwStyle,
                                                          int       X,
                                                          int       Y,
                                                          int       nWidth,
                                                          int       nHeight,
                                                          HWND      hWndParent,
                                                          HMENU     hMenu,
                                                          HINSTANCE hInstance,
                                                          LPVOID    lpParam);
introspect(win32, User32) typedef LRESULT def_window_proc_a(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
introspect(win32, User32) typedef LRESULT def_window_proc_w(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
introspect(win32, User32) typedef LRESULT dispatch_message_a(const MSG *lpMsg);
introspect(win32, User32) typedef LRESULT dispatch_message_w(const MSG *lpMsg);
introspect(win32, User32) typedef BOOL set_window_text_a(HWND hWnd, LPCSTR lpString );
introspect(win32, User32) typedef HWND set_capture(HWND hWnd);
introspect(win32, User32) typedef BOOL release_capture();
introspect(win32, User32) typedef HDC get_d_c(HWND hWnd);
introspect(win32, User32) typedef SHORT get_key_state(int nVirtKey);
introspect(win32, User32) typedef LONG_PTR get_window_long_ptr_a(HWND hWnd, int nIndex);
introspect(win32, User32) typedef LONG_PTR get_window_long_ptr_w(HWND hWnd, int nIndex);
introspect(win32, User32) typedef BOOL get_message_a(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
introspect(win32, User32) typedef BOOL peek_message_a(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
introspect(win32, User32) typedef BOOL peek_message_w(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
introspect(win32, User32) typedef ATOM register_class_ex_a(const WNDCLASSEXA *unnamedParam1);
introspect(win32, User32) typedef ATOM register_class_ex_w(const WNDCLASSEXW *unnamedParam1);
introspect(win32, User32) typedef LONG_PTR set_window_long_ptr_a(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
introspect(win32, User32) typedef LONG_PTR set_window_long_ptr_w(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
introspect(win32, User32) typedef BOOL show_window(HWND hWnd, int nCmdShow);
introspect(win32, User32) typedef int release_d_c(HWND hWnd, HDC hDC);
introspect(win32, User32) typedef BOOL translate_message(const MSG *lpMsg);
introspect(win32, User32) typedef BOOL get_cursor_pos(LPPOINT lpPoint);
introspect(win32, User32) typedef BOOL screen_to_client(HWND hWnd, LPPOINT lpPoint);
introspect(win32, User32) typedef BOOL get_client_rect(HWND hWnd, LPRECT lpRect);
introspect(win32, User32) typedef BOOL destroy_window(HWND hWnd);
introspect(win32, User32) typedef HICON load_icon_a(HINSTANCE hInstance, LPCSTR lpIconName);
introspect(win32, User32) typedef HCURSOR load_cursor_a(HINSTANCE hInstance, LPCSTR lpCursorName);
introspect(win32, User32) typedef BOOL adjust_window_rect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
introspect(win32, User32) typedef BOOL adjust_window_rect_ex(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
introspect(win32, User32) typedef void post_quit_message(int nExitCode);
introspect(win32, User32) typedef BOOL unregister_class_w(LPCWSTR lpClassName,HINSTANCE hInstance);
introspect(win32, User32) typedef int message_box_w(HWND    hWnd,
                                                    LPCWSTR lpText,
                                                    LPCWSTR lpCaption,
                                                    UINT    uType);


introspect(win32, Ole32) typedef HRESULT co_initialize(LPVOID pvReserved);
introspect(win32, Ole32) typedef HRESULT co_create_instance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
introspect(win32, Ole32) typedef void co_uninitialize();

introspect(win32, OleAut32) typedef void variant_init(VARIANTARG *pvarg);

introspect(win32, Secur32) typedef PSecurityFunctionTableA init_security_interface_a();

introspect(win32, Shell32) typedef int s_h_create_directory_ex_a(HWND hwnd, LPCSTR pszPath, SECURITY_ATTRIBUTES *psa);
introspect(win32, Shell32) typedef HINSTANCE shell_execute_a(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, 
                                                             LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);

introspect(win32, Urlmon) typedef HRESULT u_r_l_download_to_file_a(LPUNKNOWN pCaller, LPCTSTR szURL, LPCTSTR szFileName, DWORD dwReserved,LPBINDSTATUSCALLBACK lpfnCB);

introspect(win32, Ws2_32, lowercase) typedef INT WSAAPI wsa_get_addr_info(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA *pHints, PADDRINFOA *ppResult);

introspect(win32, Ws2_32) typedef int w_s_a_get_last_error();
introspect(win32, Ws2_32, lowercase) typedef SOCKET WSAAPI wsa_socket(int af, int type, int protocol);
introspect(win32, Ws2_32, lowercase) typedef int WSAAPI wsa_connect(SOCKET s, struct sockaddr *name, int namelen);
introspect(win32, Ws2_32, lowercase) typedef int WSAAPI wsa_set_sock_opt(SOCKET s, int level, int optname, const char *optval, int optlen);
introspect(win32, Ws2_32, lowercase) typedef int WSAAPI wsa_send(SOCKET s, const char *buf, int len, int flags);
introspect(win32, Ws2_32, lowercase) typedef int WSAAPI wsa_recv(SOCKET s, char *buf, int len, int flags);
introspect(win32, Ws2_32, lowercase) typedef int WSAAPI wsa_close_socket(SOCKET s);
introspect(win32, Ws2_32, lowercase) typedef void WSAAPI wsa_free_addr_info(PADDRINFOA pAddrInfo);
introspect(win32, Ws2_32, lowercase) typedef int WSAAPI wsa_shutdown(SOCKET s, int how);
introspect(win32, Ws2_32) typedef int w_s_a_startup(WORD wVersionRequired, LPWSADATA lpWSAData);
introspect(win32, Ws2_32) typedef int w_s_a_cleanup();

introspect(win32, Winmm, lowerCamelCase) typedef void time_begin_period(UINT uPeriod);

introspect(win32, Advapi32) typedef BOOL crypt_acquire_context_a(HCRYPTPROV *phProv, LPCSTR szContainer, 
                                                                 LPCSTR szProvider, DWORD dwProvType, DWORD dwFlags);
introspect(win32, Advapi32) typedef BOOL crypt_gen_random(HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer);
introspect(win32, Advapi32) typedef BOOL crypt_release_context(HCRYPTPROV hProv, DWORD dwFlags);

introspect(win32, Crypt32) typedef BOOL crypt_binary_to_string_a(const BYTE *pbBinary,
                                                                 DWORD      cbBinary,
                                                                 DWORD      dwFlags,
                                                                 LPSTR      pszString,
                                                                 DWORD      *pcchString);

introspect(win32, Wininet) typedef BOOL delete_url_cache_entry_a(LPCSTR lpszUrlName);

introspect(win32, Wininet) typedef BOOL internet_read_file(HINTERNET hFile, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead);
introspect(win32, Wininet) typedef HINTERNET internet_open_a(LPCSTR lpszAgent, DWORD dwAccessType, LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags);
introspect(win32, Wininet) typedef BOOL internet_set_option_a(HINTERNET hInternet,
                                                              DWORD     dwOption,
                                                              LPVOID    lpBuffer,
                                                              DWORD     dwBufferLength);
introspect(win32, Wininet) typedef HINTERNET internet_open_url_a(HINTERNET hInternet, LPCSTR lpszUrl, LPCSTR lpszHeaders, 
                                                                 DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext);
introspect(win32, Wininet) typedef BOOL internet_close_handle(HINTERNET hInternet);
introspect(win32, Wininet) typedef HINTERNET internet_connect_a(HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName,
                                                                LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext);
introspect(win32, Wininet) typedef HINTERNET http_open_request_a(HINTERNET hConnect, LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
                                                                 LPCSTR lpszReferrer, LPCSTR *lplpszAcceptTypes, DWORD dwFlags, DWORD_PTR dwContext);
introspect(win32, Wininet) typedef BOOL http_send_request_a(HINTERNET hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, 
                                                            LPVOID lpOptional, DWORD dwOptionalLength);
introspect(win32, Wininet) typedef BOOL internet_write_file(HINTERNET hFile, LPCVOID lpBuffer, DWORD dwNumberOfBytesToWrite, 
                                                            LPDWORD lpdwNumberOfBytesWritten);
introspect(win32, Wininet) typedef BOOL http_end_request_a(HINTERNET hRequest, LPINTERNET_BUFFERSA lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext);
introspect(win32, Wininet) typedef BOOL http_send_request_ex_a(HINTERNET hRequest, LPINTERNET_BUFFERSA lpBuffersIn, LPINTERNET_BUFFERSA lpBuffersOut,
                                                               DWORD dwFlags, DWORD_PTR dwContext);
introspect(win32, Wininet) typedef BOOL http_query_info_a(HINTERNET hRequest, DWORD dwInfoLevel, LPVOID lpBuffer, 
                                                          LPDWORD lpdwBufferLength,LPDWORD lpdwIndex);
introspect(win32, Wininet) typedef BOOL http_add_request_headers_a(HINTERNET hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwModifiers);
introspect(win32, Wininet) typedef BOOL internet_get_last_response_info_a(LPDWORD lpdwError, LPSTR lpszBuffer, LPDWORD lpdwBufferLength);

introspect(win32, Httpapi) typedef ULONG http_initialize(HTTPAPI_VERSION Version, ULONG Flags, PVOID pReserved);
introspect(win32, Httpapi) typedef ULONG http_create_server_session(HTTPAPI_VERSION Version, PHTTP_SERVER_SESSION_ID ServerSessionId, ULONG Reserved);
introspect(win32, Httpapi) typedef ULONG http_create_url_group(HTTP_SERVER_SESSION_ID ServerSessionId, PHTTP_URL_GROUP_ID pUrlGroupId, ULONG Reserved);
introspect(win32, Httpapi) typedef ULONG http_create_request_queue(HTTPAPI_VERSION      Version,
                                                                   PCWSTR               Name,
                                                                   PSECURITY_ATTRIBUTES SecurityAttributes,
                                                                   ULONG                Flags,
                                                                   PHANDLE              RequestQueueHandle);
introspect(win32, Httpapi) typedef ULONG http_set_url_group_property(HTTP_URL_GROUP_ID    UrlGroupId,
                                                                     HTTP_SERVER_PROPERTY Property,
                                                                     PVOID                PropertyInformation,
                                                                     ULONG                PropertyInformationLength);
introspect(win32, Httpapi) typedef ULONG http_add_url_to_url_group(HTTP_URL_GROUP_ID UrlGroupId,
                                                                   PCWSTR            pFullyQualifiedUrl,
                                                                   HTTP_URL_CONTEXT  UrlContext,
                                                                   ULONG             Reserved);
introspect(win32, Httpapi) typedef ULONG http_remove_url_from_url_group(HTTP_URL_GROUP_ID UrlGroupId,
                                                                        PCWSTR            pFullyQualifiedUrl,
                                                                        ULONG             Flags);
introspect(win32, Httpapi) typedef ULONG http_close_url_group(HTTP_URL_GROUP_ID UrlGroupId);
introspect(win32, Httpapi) typedef ULONG http_close_server_session(HTTP_SERVER_SESSION_ID ServerSessionId);
introspect(win32, Httpapi) typedef ULONG http_close_request_queue(HANDLE RequestQueueHandle);
introspect(win32, Httpapi) typedef ULONG http_terminate(ULONG Flags, PVOID pReserved);
introspect(win32, Httpapi) typedef ULONG http_receive_http_request(HANDLE          RequestQueueHandle,
                                                                   HTTP_REQUEST_ID RequestId,
                                                                   ULONG           Flags,
                                                                   PHTTP_REQUEST   RequestBuffer,
                                                                   ULONG           RequestBufferLength,
                                                                   PULONG          BytesReturned,
                                                                   LPOVERLAPPED    Overlapped);
introspect(win32, Httpapi) typedef ULONG http_send_http_response(HANDLE             RequestQueueHandle,
                                                                 HTTP_REQUEST_ID    RequestId,
                                                                 ULONG              Flags,
                                                                 PHTTP_RESPONSE     HttpResponse,
                                                                 PHTTP_CACHE_POLICY CachePolicy,
                                                                 PULONG             BytesSent,
                                                                 PVOID              Reserved1,
                                                                 ULONG              Reserved2,
                                                                 LPOVERLAPPED       Overlapped,
                                                                 PHTTP_LOG_DATA     LogData);
introspect(win32, Httpapi) typedef ULONG http_shutdown_request_queue(HANDLE RequestQueueHandle);


introspect(win32, DXGI) typedef HRESULT create_d_x_g_i_factory(REFIID riid, void **ppFactory);

introspect(win32, D3DCompiler_47) typedef HRESULT d_3_d_compile_from_file(LPCWSTR                pFileName,
                                                                          D3D_SHADER_MACRO *pDefines,
                                                                          ID3DInclude            *pInclude,
                                                                          LPCSTR                 pEntrypoint,
                                                                          LPCSTR                 pTarget,
                                                                          UINT                   Flags1,
                                                                          UINT                   Flags2,
                                                                          ID3DBlob               **ppCode,
                                                                          ID3DBlob               **ppErrorMsgs);

introspect(win32, D3D11) typedef HRESULT d_3_d_11_create_device_and_swap_chain(IDXGIAdapter               *pAdapter,
                                                                               D3D_DRIVER_TYPE            DriverType,
                                                                               HMODULE                    Software,
                                                                               UINT                       Flags,
                                                                               D3D_FEATURE_LEVEL    *pFeatureLevels,
                                                                               UINT                       FeatureLevels,
                                                                               UINT                       SDKVersion,
                                                                               DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
                                                                               IDXGISwapChain             **ppSwapChain,
                                                                               ID3D11Device               **ppDevice,
                                                                               D3D_FEATURE_LEVEL          *pFeatureLevel,
                                                                               ID3D11DeviceContext        **ppImmediateContext);
introspect(win32, D3D11) typedef HRESULT d_3_d_11_create_device(IDXGIAdapter            *pAdapter,
                                                                D3D_DRIVER_TYPE         DriverType,
                                                                HMODULE                 Software,
                                                                UINT                    Flags,
                                                                D3D_FEATURE_LEVEL *pFeatureLevels,
                                                                UINT                    FeatureLevels,
                                                                UINT                    SDKVersion,
                                                                ID3D11Device            **ppDevice,
                                                                D3D_FEATURE_LEVEL       *pFeatureLevel,
                                                                ID3D11DeviceContext     **ppImmediateContext);


#define WIN32_KENGINE_TYPES_H
#endif //WIN32_KENGINE_TYPES_H
