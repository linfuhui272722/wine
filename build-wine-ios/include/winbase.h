#ifndef _WINE_WINBASE_H
#define _WINE_WINBASE_H
#include "windef.h"
#include "wine.h"
typedef struct _STARTUPINFOA { DWORD cb; LPSTR lpReserved; LPSTR lpDesktop; LPSTR lpTitle; DWORD dwX; DWORD dwY; DWORD dwXSize; DWORD dwYSize; DWORD dwXCountChars; DWORD dwYCountChars; DWORD dwFillAttribute; DWORD dwFlags; WORD wShowWindow; WORD cbReserved2; BYTE *lpReserved2; HANDLE hStdInput; HANDLE hStdOutput; HANDLE hStdError; } STARTUPINFOA, *LPSTARTUPINFOA;
typedef STARTUPINFOA STARTUPINFO; typedef LPSTARTUPINFOA LPSTARTUPINFO;
typedef struct _PROCESS_INFORMATION { HANDLE hProcess; HANDLE hThread; DWORD dwProcessId; DWORD dwThreadId; } PROCESS_INFORMATION, *LPPROCESS_INFORMATION;
#define CREATE_NEW_CONSOLE 0x00000010
#define CREATE_SUSPENDED 0x00000004
#define STILL_ACTIVE 0x103
#endif
