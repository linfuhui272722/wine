/*
 * Wine iOS Core - Wine-Compatible API Headers
 */

#ifndef WINE_CORE_H
#define WINE_CORE_H

#include <stdint.h>
#include <stddef.h>

// Wine Core Version
#define WINE_CORE_VERSION "1.0.0"

// Basic Windows Types
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t ULONGLONG;
typedef int64_t LONGLONG;

typedef DWORD *PDWORD;
typedef WORD *PWORD;
typedef BYTE *PBYTE;

typedef int32_t LONG;
typedef int64_t LONGLONG;

typedef uintptr_t UINT_PTR;
typedef intptr_t INT_PTR;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;

typedef int BOOL;
typedef int WINBOOL;
typedef unsigned int UINT;
typedef void *PVOID;
typedef void VOID;
typedef const void *PCVOID;

typedef char CHAR;
typedef wchar_t WCHAR;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;

typedef size_t SIZE_T;
typedef uintptr_t ULONG_PTR;
typedef intptr_t LONG_PTR;
typedef LONG_PTR SSIZE_T;
typedef ULONG_PTR SIZE_T_;

#define MAX_PATH 260

// Handle types
#define DECLARE_HANDLE(name) typedef struct name##__ { int unused; } *name
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HLOCAL);
DECLARE_HANDLE(HGLOBAL);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGLRC);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HCURSOR);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HDWP);
DECLARE_HANDLE(HPROPSHEETPAGE);
DECLARE_HANDLE(HKEY);
typedef HKEY *PHKEY;
DECLARE_HANDLE(HMETAFILE);
DECLARE_HANDLE(HENHMETAFILE);
DECLARE_HANDLE(HWINEVENTHOOK);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HMODULE);

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define MAXUINT8  0xFF
#define MAXUINT16 0xFFFF
#define MAXUINT32 0xFFFFFFFF
#define MAXINT8   0x7F
#define MAXINT16  0x7FFF
#define MAXINT32  0x7FFFFFFF

// Boolean constants
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// Calling conventions
#define WINAPI __attribute__((ms_abi))
#define CALLBACK __attribute__((ms_abi))
#define APIENTRY WINAPI

// Structure definitions
#pragma pack(push, 1)

typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        } DUMMYSTRUCTNAME;
        PVOID Pointer;
    } DUMMYUNIONNAME;
    HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    PVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[MAX_PATH];
    CHAR cAlternateFileName[14];
    DWORD dwFileType;
    DWORD dwCreatorType;
    WORD wFinderFlags;
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA;

typedef WIN32_FIND_DATAA WIN32_FIND_DATA;
typedef PWIN32_FIND_DATAA PWIN32_FIND_DATA;

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME;

typedef struct _MEMORY_BASIC_INFORMATION {
    PVOID BaseAddress;
    PVOID AllocationBase;
    DWORD AllocationProtect;
    SIZE_T RegionSize;
    DWORD State;
    DWORD Protect;
    DWORD Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

#pragma pack(pop)

// Memory constants
#define HEAP_ZERO_MEMORY 0x00000008
#define HEAP_GENERATE_EXCEPTIONS 0x00000004

#define MEM_COMMIT  0x1000
#define MEM_RESERVE 0x2000
#define MEM_DECOMMIT 0x4000
#define MEM_RELEASE  0x8000
#define MEM_FREE     0x10000
#define MEM_PRIVATE  0x20000
#define MEM_MAPPED   0x40000

#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80

// File constants
#define GENERIC_READ           0x80000000
#define GENERIC_WRITE          0x40000000
#define GENERIC_EXECUTE        0x20000000
#define GENERIC_ALL            0x10000000

#define FILE_SHARE_READ         0x00000001
#define FILE_SHARE_WRITE        0x00000002
#define FILE_SHARE_DELETE       0x00000004

#define CREATE_NEW             1
#define CREATE_ALWAYS          2
#define OPEN_EXISTING          3
#define OPEN_ALWAYS            4
#define TRUNCATE_EXISTING      5

#define FILE_ATTRIBUTE_NORMAL   0x00000080
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_HIDDEN  0x00000002
#define FILE_ATTRIBUTE_SYSTEM  0x00000004
#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020

#define INVALID_FILE_SIZE      ((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)

// Thread constants
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);
typedef DWORD (*PTHREAD_START_ROUTINE)(void *);

// Registry
typedef LONG REGSAM;
typedef DWORD ACCESS_MASK;
#define KEY_ALL_ACCESS 0xF003F

// Error codes
#define ERROR_SUCCESS           0
#define ERROR_FILE_NOT_FOUND    2
#define ERROR_PATH_NOT_FOUND    3
#define ERROR_ACCESS_DENIED    5
#define ERROR_INVALID_HANDLE    6
#define ERROR_NOT_ENOUGH_MEMORY 8
#define ERROR_INVALID_PARAMETER 87
#define ERROR_CALL_NOT_IMPLEMENTED 120

// Thread access rights
#define THREAD_ALL_ACCESS 0x1F03FF

// Synchronization
typedef DWORD (WINAPI *LPSECURITY_ATTRIBUTES);

// Wine Core Context
typedef struct {
    const char *prefix;
    const char *version;
    int initialized;
} wine_core_context_t;

// Wine Core API
#ifdef __cplusplus
extern "C" {
#endif

// Memory
void *WINAPI wine_Malloc(size_t size);
void *WINAPI wine_MemAlloc(DWORD flags, size_t size);
void WINAPI wine_MemFree(void *ptr);
void *WINAPI wine_VirtualAlloc(void *addr, SIZE_T size, DWORD type, DWORD protect);
BOOL WINAPI wine_VirtualFree(void *addr, SIZE_T size, DWORD type);
BOOL WINAPI wine_VirtualProtect(void *addr, SIZE_T size, DWORD protect, DWORD *old_protect);
SIZE_T WINAPI wine_VirtualQuery(void *addr, MEMORY_BASIC_INFORMATION *info, SIZE_T len);

// Strings
int WINAPI wine_lstrcmpA(LPCSTR s1, LPCSTR s2);
int WINAPI wine_lstrcmpW(LPCWSTR s1, LPCWSTR s2);
int WINAPI wine_lstrcmpiA(LPCSTR s1, LPCSTR s2);
LPSTR WINAPI wine_lstrcpyA(LPSTR dest, LPCSTR src);
LPWSTR WINAPI wine_lstrcpyW(LPWSTR dest, LPCWSTR src);
LPSTR WINAPI wine_lstrcpynA(LPSTR dest, LPCSTR src, int count);
int WINAPI wine_lstrlenA(LPCSTR s);
int WINAPI wine_lstrlenW(LPCWSTR s);

// Files
HANDLE WINAPI wine_CreateFileA(LPCSTR name, DWORD access, DWORD mode, 
    SECURITY_ATTRIBUTES *sa, DWORD creation, DWORD attrs, HANDLE tmpl);
HANDLE WINAPI wine_CreateFileW(LPCWSTR name, DWORD access, DWORD mode,
    SECURITY_ATTRIBUTES *sa, DWORD creation, DWORD attrs, HANDLE tmpl);
BOOL WINAPI wine_ReadFile(HANDLE h, void *buf, DWORD to_read, DWORD *read, OVERLAPPED *ov);
BOOL WINAPI wine_WriteFile(HANDLE h, const void *buf, DWORD to_write, DWORD *written, OVERLAPPED *ov);
BOOL WINAPI wine_CloseHandle(HANDLE h);
DWORD WINAPI wine_GetFileSize(HANDLE h, DWORD *high);
DWORD WINAPI wine_SetFilePointer(HANDLE h, LONG dist, LONG *high, DWORD method);

// Directories
HANDLE WINAPI wine_FindFirstFileA(LPCSTR spec, WIN32_FIND_DATAA *data);
BOOL WINAPI wine_FindNextFileA(HANDLE h, WIN32_FIND_DATAA *data);
BOOL WINAPI wine_FindClose(HANDLE h);

// Threads
HANDLE WINAPI wine_CreateThread(SECURITY_ATTRIBUTES *sa, SIZE_T stack, 
    LPTHREAD_START_ROUTINE start, void *param, DWORD flags, DWORD *tid);
void WINAPI wine_ExitThread(DWORD code);
void WINAPI wine_ExitProcess(DWORD code);
DWORD WINAPI wine_GetCurrentThreadId(void);
DWORD WINAPI wine_GetCurrentProcessId(void);
HANDLE WINAPI wine_GetCurrentThread(void);
HANDLE WINAPI wine_GetCurrentProcess(void);

// Synchronization
HANDLE WINAPI wine_CreateMutexA(SECURITY_ATTRIBUTES *sa, BOOL owner, LPCSTR name);
BOOL WINAPI wine_ReleaseMutex(HANDLE h);

// Time
void WINAPI wine_GetSystemTime(SYSTEMTIME *st);
void WINAPI wine_GetLocalTime(SYSTEMTIME *st);
DWORD WINAPI wine_GetTickCount(void);

// Modules
HMODULE WINAPI wine_LoadLibraryA(LPCSTR name);
HMODULE WINAPI wine_LoadLibraryW(LPCWSTR name);
FARPROC WINAPI wine_GetProcAddress(HMODULE h, LPCSTR name);
BOOL WINAPI wine_FreeLibrary(HMODULE h);

// Registry
LONG WINAPI wine_RegOpenKeyExA(HKEY hkey, LPCSTR subkey, DWORD options, REGSAM access, PHKEY res);
LONG WINAPI wine_RegCloseKey(HKEY hkey);

// Utilities
BOOL WINAPI wine_IsBadReadPtr(const void *ptr, UINT size);

// Initialization
int wine_core_init(const char *prefix);
void wine_core_cleanup(void);
const char *wine_core_version(void);

#ifdef __cplusplus
}
#endif

#endif // WINE_CORE_H
