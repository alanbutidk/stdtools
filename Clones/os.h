#include "basic.h"

#if OS_IS == WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <io.h>
#include <tlhelp32.h>

// SYS_XXXXX type macro functions.
#define SYS_OPEN(path) CreateFileA(...)
#define SYS_READ(...) win_read(...)
#define SYS_WRITE(...) win_write(...)
#define SYS_CLOSE(fh) CloseHandle(fh)
#define FILE_HANDLE HANDLE
#define INVALID_HANDLE INVALID_HANDLE_VALUE

#elif OS_IS == LINUX_UNIX

#include <fcntl.h>
#include <linux/limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYS_OPEN(path) ((int)syscall(SYS_open, path, O_RDONLY, 0))
#define SYS_READ(...) ((long)syscall(SYS_read, ...))
#define SYS_WRITE(...) ((long)syscall(SYS_write, ...))
#define SYS_CLOSE(fh) ((void)syscall(SYS_close, fh))
#define SYS_WRITE_STDOUT(buf, n) ((long)syscall(SYS_write, 1, (buf), (n)))
#define FILE_HANDLE int
#define INVALID_HANDLE (-1)

#else
return 1;
#endif
// All basic headers.
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
