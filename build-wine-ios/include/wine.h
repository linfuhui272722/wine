#ifndef _WINE_H
#define _WINE_H
#include "windef.h"

/* Memory */
void *WINAPI VirtualAlloc(void *addr, SIZE_T size, DWORD type, DWORD protect);
BOOL WINAPI VirtualFree(void *addr, SIZE_T size, DWORD type);
void *WINAPI HeapAlloc(HANDLE heap, DWORD flags, SIZE_T size);
BOOL WINAPI HeapFree(HANDLE heap, DWORD flags, void *ptr);
HANDLE WINAPI GetProcessHeap(void);

/* Strings */
int WINAPI lstrcmpA(LPCSTR s1, LPCSTR s2);
LPSTR WINAPI lstrcpyA(LPSTR dest, LPCSTR src);
LPWSTR WINAPI lstrcpyW(LPWSTR dest, LPCWSTR src);
int WINAPI lstrlenA(LPCSTR s);
int WINAPI lstrlenW(LPCWSTR s);

/* Files */
HANDLE WINAPI CreateFileA(LPCSTR name, DWORD access, DWORD mode, LPSECURITY_ATTRIBUTES sa, DWORD creation, DWORD attrs, HANDLE tmpl);
HANDLE WINAPI CreateFileW(LPCWSTR name, DWORD access, DWORD mode, LPSECURITY_ATTRIBUTES sa, DWORD creation, DWORD attrs, HANDLE tmpl);
BOOL WINAPI ReadFile(HANDLE h, void *buf, DWORD to_read, DWORD *read, LPOVERLAPPED ov);
BOOL WINAPI WriteFile(HANDLE h, const void *buf, DWORD to_write, DWORD *written, LPOVERLAPPED ov);
BOOL WINAPI CloseHandle(HANDLE h);
DWORD WINAPI GetFileSize(HANDLE h, DWORD *high);

/* Directories */
HANDLE WINAPI FindFirstFileA(LPCSTR spec, LPWIN32_FIND_DATAA data);
BOOL WINAPI FindNextFileA(HANDLE h, LPWIN32_FIND_DATAA data);
BOOL WINAPI FindClose(HANDLE h);
BOOL WINAPI CreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sa);

/* Threads */
HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES sa, SIZE_T stack, LPTHREAD_START_ROUTINE start, void *param, DWORD flags, DWORD *tid);
void WINAPI ExitThread(DWORD code);
void WINAPI ExitProcess(DWORD code);
DWORD WINAPI GetCurrentThreadId(void);
DWORD WINAPI GetCurrentProcessId(void);
HANDLE WINAPI GetCurrentThread(void);
HANDLE WINAPI GetCurrentProcess(void);
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);

/* Sync */
HANDLE WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES sa, BOOL owner, LPCSTR name);
BOOL WINAPI ReleaseMutex(HANDLE h);

/* Time */
void WINAPI GetSystemTime(LPSYSTEMTIME st);
DWORD WINAPI GetTickCount(void);

/* Modules */
HMODULE WINAPI LoadLibraryA(LPCSTR name);
HMODULE WINAPI LoadLibraryW(LPCWSTR name);
FARPROC WINAPI GetProcAddress(HMODULE h, LPCSTR name);
BOOL WINAPI FreeLibrary(HMODULE h);
DWORD WINAPI GetModuleFileNameA(HMODULE h, LPSTR name, DWORD size);
HMODULE WINAPI GetModuleHandleA(LPCSTR name);

/* Registry */
LONG WINAPI RegOpenKeyExA(HKEY hkey, LPCSTR subkey, DWORD options, DWORD access, PHKEY res);
LONG WINAPI RegCloseKey(HKEY hkey);

/* Error */
DWORD WINAPI GetLastError(void);
void WINAPI SetLastError(DWORD error);

#endif
