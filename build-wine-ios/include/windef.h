#ifndef _WINE_WINDEF_H
#define _WINE_WINDEF_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t BYTE; typedef uint16_t WORD; typedef uint32_t DWORD; typedef uint64_t QWORD;
typedef int32_t BOOL; typedef void *HANDLE;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif
#define MAX_PATH 260
#define INFINITE 0xFFFFFFFF
typedef DWORD *PDWORD; typedef WORD *PWORD; typedef BYTE *PBYTE; typedef void *PVOID;
typedef const void *LPCVOID; typedef char *LPSTR; typedef const char *LPCSTR;
typedef wchar_t *LPWSTR; typedef const wchar_t *LPCWSTR;
typedef int INT; typedef unsigned int UINT; typedef long LONG; typedef unsigned long ULONG;
typedef long LPARAM; typedef unsigned long WPARAM; typedef long LRESULT;
typedef unsigned long DWORD_PTR; typedef long INT_PTR; typedef unsigned long UINT_PTR;
typedef size_t SIZE_T; typedef unsigned long ULONG_PTR;
typedef struct _FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; } FILETIME, *PFILETIME, *LPFILETIME;
typedef struct _SECURITY_ATTRIBUTES { DWORD nLength; void *lpSecurityDescriptor; BOOL bInheritHandle; } SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
typedef struct _OVERLAPPED { ULONG_PTR Internal; ULONG_PTR InternalHigh; union { struct { DWORD Offset; DWORD OffsetHigh; } DUMMYSTRUCTNAME; PVOID Pointer; } DUMMYUNIONNAME; HANDLE hEvent; } OVERLAPPED, *LPOVERLAPPED;
typedef struct _WIN32_FIND_DATAA { DWORD dwFileAttributes; FILETIME ftCreationTime; FILETIME ftLastAccessTime; FILETIME ftLastWriteTime; DWORD nFileSizeHigh; DWORD nFileSizeLow; DWORD dwReserved0; DWORD dwReserved1; CHAR cFileName[MAX_PATH]; CHAR cAlternateFileName[14]; } WIN32_FIND_DATAA, *PWIN32_FIND_DATAA;
typedef WIN32_FIND_DATAA WIN32_FIND_DATA; typedef PWIN32_FIND_DATAA PWIN32_FIND_DATA;
typedef struct _SYSTEMTIME { WORD wYear; WORD wMonth; WORD wDayOfWeek; WORD wDay; WORD wHour; WORD wMinute; WORD wSecond; WORD wMilliseconds; } SYSTEMTIME, *PSYSTEMTIME;
typedef struct _MEMORY_BASIC_INFORMATION { PVOID BaseAddress; PVOID AllocationBase; DWORD AllocationProtect; SIZE_T RegionSize; DWORD State; DWORD Protect; DWORD Type; } MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define HEAP_ZERO_MEMORY 0x00000008
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_RELEASE 0x8000
#define PAGE_READWRITE 0x04
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define FILE_SHARE_READ 0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define ERROR_SUCCESS 0
#define ERROR_FILE_NOT_FOUND 2
#define CALLBACK __attribute__((ms_abi))
#define WINAPI __attribute__((ms_abi))
#ifndef DECLARE_HANDLE
#define DECLARE_HANDLE(name) typedef struct name##__ { int unused; } *name
#endif
DECLARE_HANDLE(HMODULE); DECLARE_HANDLE(HINSTANCE); DECLARE_HANDLE(HDC); DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HRGN); DECLARE_HANDLE(HBRUSH); DECLARE_HANDLE(HPEN); DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HPALETTE); DECLARE_HANDLE(HFONT); DECLARE_HANDLE(HCURSOR); DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HMENU); DECLARE_HANDLE(HKEY); typedef HKEY *PHKEY;
#endif
