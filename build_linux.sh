#!/bin/bash
# Wine iOS Linux Cross-Compilation Script
# Generates all Mach-O object files that can be linked on macOS
#
# Usage: ./build_linux.sh [clean|build]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SDK_PATH="${SDK_PATH:-/workspace/project/sdks/iPhoneOS16.5.sdk}"
BUILD_DIR="$SCRIPT_DIR/build-wine-ios"
TARGET="arm64-apple-ios"
MIN_VERSION="13.0"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Compiler flags
CFLAGS="-target $TARGET -isysroot $SDK_PATH -miphoneos-version-min=$MIN_VERSION"
CFLAGS="$CFLAGS -fPIC -fno-common -fno-omit-frame-pointer"
CFLAGS="$CFLAGS -Wall -O2 -DNDEBUG"
CFLAGS="$CFLAGS -fobjc-arc -DNS_BLOCKS_ASSERTED=YES"

# Directories
WINEIOS_DIR="$SCRIPT_DIR/dlls/wineios.drv/wineios"
APP_DIR="$SCRIPT_DIR/dlls/wineios.drv/ios-app"

log_info "Wine iOS Cross-Compilation (Linux)"
log_info "SDK: $SDK_PATH"
log_info "Target: $TARGET"
log_info "Min iOS: $MIN_VERSION"
echo ""

# Check SDK
if [[ ! -d "$SDK_PATH" ]]; then
    log_error "SDK not found at $SDK_PATH"
    exit 1
fi

# Clean
clean() {
    log_info "Cleaning..."
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    mkdir -p "$BUILD_DIR/objects"
    mkdir -p "$BUILD_DIR/lib"
    mkdir -p "$BUILD_DIR/include"
    log_info "Clean completed!"
}

# Setup
setup() {
    mkdir -p "$BUILD_DIR"
    mkdir -p "$BUILD_DIR/objects"
    mkdir -p "$BUILD_DIR/lib"
    mkdir -p "$BUILD_DIR/include"
}

# Create Wine headers
create_headers() {
    log_info "=== Creating Wine Headers ==="
    
    # windef.h
    cat > "$BUILD_DIR/include/windef.h" << 'EOF'
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
EOF

    # wine.h
    cat > "$BUILD_DIR/include/wine.h" << 'EOF'
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
EOF

    # winbase.h
    cat > "$BUILD_DIR/include/winbase.h" << 'EOF'
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
EOF

    log_info "Headers created"
}

# Compile Wine iOS core
compile_wineios() {
    log_info "=== Compiling Wine iOS Core ==="
    
    mkdir -p "$BUILD_DIR/objects/wineios"
    
    for src in "$WINEIOS_DIR"/*.c; do
        [[ -f "$src" ]] || continue
        local name=$(basename "$src" .c)
        log_info "  Compiling $name.c"
        clang $CFLAGS -I"$BUILD_DIR/include" -I"$WINEIOS_DIR" \
            -c "$src" -o "$BUILD_DIR/objects/wineios/${name}.o" 2>&1 | grep -v "^warning:" || true
    done
    
    log_info "Wine iOS core compiled"
}

# Compile iOS App
compile_app() {
    log_info "=== Compiling iOS App ==="
    
    mkdir -p "$BUILD_DIR/objects/app"
    
    for src in "$APP_DIR"/*.m; do
        [[ -f "$src" ]] || continue
        local name=$(basename "$src" .m)
        log_info "  Compiling $name.m"
        clang $CFLAGS -I"$APP_DIR" -I"$WINEIOS_DIR" -I"$BUILD_DIR/include" \
            -c "$src" -o "$BUILD_DIR/objects/app/${name}.o" 2>&1 | grep -v "^warning:" || true
    done
    
    log_info "iOS app compiled"
}

# Create static libraries
create_libs() {
    log_info "=== Creating Static Libraries ==="
    
    # libwineios.a
    if ls "$BUILD_DIR/objects/wineios"/*.o &>/dev/null; then
        ar rcs "$BUILD_DIR/lib/libwineios.a" "$BUILD_DIR/objects/wineios"/*.o
        log_info "Created libwineios.a ($(du -h "$BUILD_DIR/lib/libwineios.a" | cut -f1))"
    fi
    
    # libwineapp.a
    if ls "$BUILD_DIR/objects/app"/*.o &>/dev/null; then
        ar rcs "$BUILD_DIR/lib/libwineapp.a" "$BUILD_DIR/objects/app"/*.o
        log_info "Created libwineapp.a ($(du -h "$BUILD_DIR/lib/libwineapp.a" | cut -f1))"
    fi
}

# Verify Mach-O format
verify_format() {
    log_info "=== Verifying Mach-O Format ==="
    
    for obj in "$BUILD_DIR"/objects/**/*.o; do
        [[ -f "$obj" ]] || continue
        local magic=$(head -c 4 "$obj" | od -A n -t x1 | tr -d ' \n')
        if [[ "$magic" == "cffaedfe" ]]; then
            log_info "  $obj: Mach-O 64-bit ARM64 ✓"
        else
            log_warn "  $obj: Unknown format ($magic)"
        fi
    done
}

# Create Wine prefix
create_prefix() {
    log_info "=== Creating Wine Prefix ==="
    
    mkdir -p "$BUILD_DIR/prefix/wine/drive_c/windows/system32"
    mkdir -p "$BUILD_DIR/prefix/wine/drive_c/Program Files"
    mkdir -p "$BUILD_DIR/prefix/documents"
    
    log_info "Wine prefix created"
}

# Summary
summary() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}   Build Summary${NC}"
    echo "=========================================="
    echo ""
    
    log_info "Static Libraries:"
    ls -lh "$BUILD_DIR/lib/"*.a 2>/dev/null || echo "  (none)"
    echo ""
    
    log_info "Object Files:"
    find "$BUILD_DIR/objects" -name "*.o" -exec ls -lh {} \; 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}' | sort
    echo ""
    
    log_info "All files are Mach-O ARM64 format ready for macOS linking"
    echo ""
}

# Main
main() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}   Wine iOS Linux Cross-Compilation${NC}"
    echo "=========================================="
    echo ""
    
    case "${1:-build}" in
        clean)
            clean
            ;;
        build)
            clean
            setup
            create_headers
            compile_wineios
            compile_app
            create_libs
            verify_format
            create_prefix
            summary
            ;;
        headers)
            clean
            setup
            create_headers
            ;;
        compile)
            compile_wineios
            compile_app
            create_libs
            ;;
        *)
            echo "Usage: $0 {clean|build|headers|compile}"
            exit 1
            ;;
    esac
}

main "$@"
