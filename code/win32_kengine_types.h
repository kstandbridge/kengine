#ifndef WIN32_KENGINE_TYPES_H

#define SECURITY_WIN32

#include <WS2tcpip.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <Shlobj.h>
#include <Uxtheme.h>
#include <vssym32.h>
#include <Wininet.h>
#include <sspi.h>
#include <Schnlsp.h>



#include <gl/gl.h>


introspect(win32, Kernel32) typedef BOOL close_handle(HANDLE hObject);
introspect(win32, Kernel32) typedef LONG compare_file_time(const FILETIME *lpFileTime1, const FILETIME *lpFileTime2);
introspect(win32, Kernel32) typedef BOOL copy_file_a(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
introspect(win32, Kernel32) typedef HANDLE create_file_a(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
introspect(win32, Kernel32) typedef HANDLE create_semaphore_ex_a(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName, DWORD dwFlags, DWORD dwDesiredAccess);
introspect(win32, Kernel32) typedef HANDLE create_thread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
introspect(win32, Kernel32) typedef void exit_process(UINT uExitCode);
introspect(win32, Kernel32) typedef BOOL free_library(HMODULE hLibModule);
introspect(win32, Kernel32) typedef LPSTR get_command_line_a();
introspect(win32, Kernel32) typedef DWORD get_current_thread_id();
introspect(win32, Kernel32) typedef BOOL get_file_attributes_ex_a(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
introspect(win32, Kernel32) typedef DWORD get_file_attributes_a(LPCSTR lpFileName);
introspect(win32, Kernel32) typedef HANDLE WINAPI get_std_handle(DWORD nStdHandle);
introspect(win32, Kernel32) typedef HMODULE load_library_a(LPCSTR lpLibFileName);
introspect(win32, Kernel32) typedef BOOL get_file_size_ex(HANDLE hFile, PLARGE_INTEGER lpFileSize);
introspect(win32, Kernel32) typedef BOOL read_file(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
introspect(win32, Kernel32) typedef BOOL release_semaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount);
introspect(win32, Kernel32) typedef BOOL write_file(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
introspect(win32, Kernel32) typedef LPVOID virtual_alloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
introspect(win32, Kernel32) typedef BOOL virtual_free(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
introspect(win32, Kernel32) typedef DWORD wait_for_single_object_ex(HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable);
introspect(win32, Kernel32) typedef HMODULE get_module_handle_a(LPCSTR lpModuleName);
introspect(win32, Kernel32) typedef int mul_div(int nNumber, int nNumerator, int nDenominator);
introspect(win32, Kernel32) typedef BOOL query_performance_counter(LARGE_INTEGER *lpPerformanceCount);
introspect(win32, Kernel32) typedef BOOL query_performance_frequency(LARGE_INTEGER *lpFrequency);
introspect(win32, Kernel32) typedef void sleep(DWORD dwMilliseconds);
introspect(win32, Kernel32) typedef void get_system_info(LPSYSTEM_INFO lpSystemInfo);
introspect(win32, Kernel32) typedef DWORD get_environment_variable_a(LPCTSTR lpName, LPTSTR lpBuffer, DWORD nSize);
introspect(win32, Kernel32) typedef DWORD expand_environment_strings_a(LPCSTR lpSrc, LPSTR lpDst, DWORD nSize);
introspect(win32, Kernel32) typedef void move_memory(PVOID Destination, const VOID *Source, SIZE_T Length);

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

introspect(win32, User32) typedef HWND create_window_ex_a(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
introspect(win32, User32) typedef LRESULT def_window_proc_a(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
introspect(win32, User32) typedef LRESULT dispatch_message_a(const MSG *lpMsg);
introspect(win32, User32) typedef HDC get_d_c(HWND hWnd);
introspect(win32, User32) typedef SHORT get_key_state(int nVirtKey);
introspect(win32, User32) typedef LONG_PTR get_window_long_ptr_a(HWND hWnd, int nIndex);
introspect(win32, User32) typedef BOOL peek_message_a(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
introspect(win32, User32) typedef ATOM register_class_ex_a(const WNDCLASSEXA *unnamedParam1);
introspect(win32, User32) typedef LONG_PTR set_window_long_ptr_a(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
introspect(win32, User32) typedef BOOL show_window(HWND hWnd, int nCmdShow);
introspect(win32, User32) typedef int release_d_c(HWND hWnd, HDC hDC);
introspect(win32, User32) typedef BOOL translate_message(const MSG *lpMsg);
introspect(win32, User32) typedef BOOL get_cursor_pos(LPPOINT lpPoint);
introspect(win32, User32) typedef BOOL screen_to_client(HWND hWnd, LPPOINT lpPoint);
introspect(win32, User32) typedef BOOL get_client_rect(HWND hWnd, LPRECT lpRect);
introspect(win32, User32) typedef BOOL destroy_window(HWND hWnd);

introspect(win32, Winmm, lowerCamelCase) typedef void time_begin_period(UINT uPeriod);

introspect(win32, Opengl32, lowerCamelCase) typedef HGLRC wgl_create_context(HDC unnamedParam1);
introspect(win32, Opengl32, lowerCamelCase) typedef BOOL wgl_make_current(HDC unnamedParam1, HGLRC unnamedParam2);
introspect(win32, Opengl32, lowerCamelCase) typedef PROC wgl_get_proc_address(LPCSTR unnamedParam1);
introspect(win32, Opengl32, lowerCamelCase) typedef BOOL wgl_delete_context(HGLRC unnamedParam1);

introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_enable(GLenum cap);
introspect(win32, Opengl32, lowerCamelCase) typedef GLubyte* gl_get_string(GLenum name);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_begin(GLenum mode);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_color_4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_tex_coord_2f(GLfloat s, GLfloat t);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_vertex_2f(GLfloat x, GLfloat y);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_viewport(GLint x, GLint y, GLsizei width, GLsizei height);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_bind_texture(GLenum target, GLuint texture);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_tex_image_2_d(GLenum target, GLint level, GLint internalformat, 
                                                                                 GLsizei width, GLsizei height, GLint border, GLint format, 
                                                                                 GLenum type, const GLvoid *pixels);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_tex_parameteri(GLenum target, GLenum pname, GLint param);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_tex_envi(GLenum target, GLenum pname, GLint param);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_clear_color(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_clear(GLbitfield mask);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_matrix_mode(GLenum mode);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_load_identity();
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_gen_textures(GLsizei n, GLuint *textures);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_delete_textures(GLsizei n, const GLuint *textures);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_blend_func(GLenum sfactor, GLenum dfactor);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_disable(GLenum cap);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_load_matrixf(const GLfloat *m);
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_end();
introspect(win32, Opengl32, lowerCamelCase) typedef void WINAPI gl_scissor(GLint x, GLint y, GLsizei width, GLsizei height);

introspect(win32, Shell32) typedef int s_h_create_directory_ex_a(HWND hwnd, LPCSTR pszPath, SECURITY_ATTRIBUTES *psa);

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

introspect(win32, Secur32) typedef PSecurityFunctionTableA init_security_interface_a();

#define WIN32_KENGINE_TYPES_H
#endif //WIN32_KENGINE_TYPES_H
