#ifndef OS_H
#define OS_H

#define _GNU_SOURCE

// Contains the OS_IS macro:
#include "basic.h"

#if OS_IS == 1

// Strip out not needed libs/headers

#ifdef STRIP_UNUSED_WIN_HEADERS
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

// A blank new line above to not make formatters rearrange headers in alphabatic
// order (breaks compilation).
#include <io.h>
#include <tlhelp32.h>

#if defined(_MSC_VER) && !defined(__clang__)
#error "Compiling via MSVC is not supported."
#endif

// regex.h is avaliable on windows but only for cygwin, mingw64
#include <regex.h>

// If NEED_WINSHELLAPI is defined.
#if defined(NEED_WINSHELLAPI)
#include <shellapi.h>
#endif

// Check if INCLUDE_SYSHEADERS is there and include the following headers:
#ifdef INCLUDE_SYSHEADERS
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <unistd.h>
#endif

// SYS_XXXXX type macro functions.
/* // Uncomment this if you dare to survive GCC with the errors.
#define SYS_OPEN(path) CreateFileA(...)
#define SYS_READ(...) win_read(...)
#define SYS_WRITE(...) win_write(...)
#define SYS_CLOSE(fh) CloseHandle(fh)
#define FILE_HANDLE HANDLE
#define INVALID_HANDLE INVALID_HANDLE_VALUE
*/

#elif OS_IS == 2

#include <fcntl.h>
#include <linux/limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef INCLUDE_REGEX_HEADER
#include <regex.h>
#endif

/* // Uncomment this if you dare to survive GCC with the errors.
#define SYS_OPEN(path) ((int)syscall(SYS_open, path, O_RDONLY, 0))
#define SYS_READ(...) ((long)syscall(SYS_read, ...))
#define SYS_WRITE(...) ((long)syscall(SYS_write, ...))
#define SYS_CLOSE(fh) ((void)syscall(SYS_close, fh))
#define SYS_WRITE_STDOUT(buf, n) ((long)syscall(SYS_write, 1, (buf), (n)))
#define FILE_HANDLE int
#define INVALID_HANDLE (-1)
*/

#else
#error "Unknown OS detected, Couldn't include headers."
#endif

// All basic headers.
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#endif
