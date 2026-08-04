/* fresh, doesnt use CRT at all. Copy of cls and clear.
 * CC Command:

 * Windows: gcc -Os -s -nostdlib -fno-builtin ffresh.c -o ffresh.exe -e
 mainEntry -lkernel32

 * Linux: gcc -Os -s -nostdlib -fno-builtin ffresh.c -o ffresh Use --help/-h for
 usage

*/

int MiniStrCmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int MiniStrLen(const char *str) {
  int len = 0;
  while (str[len])
    len++;
  return len;
}

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void WinPrintf(HANDLE hOut, const char *msg) {
  DWORD written;
  WriteConsoleA(hOut, msg, MiniStrLen(msg), &written, NULL);
}

int ClearScreen(HANDLE hConsole) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD count, cellCount;
  COORD homeCoords = {0, 0};

  if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    return 1;

  cellCount = csbi.dwSize.X * csbi.dwSize.Y;
  FillConsoleOutputCharacterA(hConsole, ' ', cellCount, homeCoords, &count);
  FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords,
                             &count);
  SetConsoleCursorPosition(hConsole, homeCoords);
  return 0;
}

void mainEntry() {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hConsole == INVALID_HANDLE_VALUE)
    ExitProcess(1);

  DWORD dwMode = 0;

  if (GetConsoleMode(hConsole, &dwMode)) {
    dwMode |= 0x0004;
    SetConsoleMode(hConsole, dwMode);
  }

  LPSTR cmdLine = GetCommandLineA();
  char *arg = cmdLine;
  if (*arg == '"') {
    arg++;
    while (*arg && *arg != '"')
      arg++;
    if (*arg == '"')
      arg++;
  } else {
    while (*arg && *arg != ' ')
      arg++;
  }
  while (*arg == ' ')
    arg++;

  if (*arg == '\0') {
    ExitProcess(ClearScreen(hConsole));
  }

  if (MiniStrCmp(arg, "--help") == 0 || MiniStrCmp(arg, "-h") == 0) {
    WinPrintf(hConsole, "\033[33mfresh ");
    WinPrintf(hConsole, arg);
    WinPrintf(hConsole, " called\n--help/-h: Print help and "
                        "exit.\n--version/-v: Print version info\n\033[0m");
    ExitProcess(0);
  } else if (MiniStrCmp(arg, "--version") == 0 || MiniStrCmp(arg, "-v") == 0) {
    WinPrintf(
        hConsole,
        "\033[33mfresh - stdTools v1.0.0\n"
        "Copyright (C) 2026 Free Software Foundation, Inc.\n"
        "License GPLv3+: GNU GPL version 3 or later <https://gnu.org>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\033[0m\n");
    ExitProcess(0);
  } else {
    WinPrintf(hConsole, "\033[31mFlag: ");
    WinPrintf(hConsole, arg);
    WinPrintf(hConsole, " not recognized!\n\033[0m");
    ExitProcess(1);
  }
}

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define SYS_WRITE 1
#define SYS_EXIT 60
#define STDOUT_FILENO 1

static inline long syscall3(long number, long arg1, long arg2, long arg3) {
  long ret;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3)
                       : "rcx", "r11", "memory");
  return ret;
}

static inline void syscall1(long number, long arg1) {
  __asm__ __volatile__("syscall"
                       :
                       : "a"(number), "D"(arg1)
                       : "rcx", "r11", "memory");
}

void Printf(const char *msg) {
  syscall3(SYS_WRITE, STDOUT_FILENO, (long)msg, MiniStrLen(msg));
}

void _start() {
  long *stack_ptr;
  __asm__("mov %%rsp, %0" : "=r"(stack_ptr));

  long argc = *stack_ptr;
  if (argc == 1) {
    Printf("\e[1J\e[2J\e[3J\e[H"); // Fixed function name here
    syscall1(SYS_EXIT, 0);
  }

  char *arg = (char *)*(stack_ptr + 2);

  if (MiniStrCmp(arg, "--help") == 0 || MiniStrCmp(arg, "-h") == 0) {
    Printf("\033[33mfresh ");
    Printf(arg);
    Printf(" called\n--help/-h: Print help and exit.\n--version/-v: Print "
           "version info\n\033[0m");
    syscall1(SYS_EXIT, 0);
  } else if (MiniStrCmp(arg, "--version") == 0 || MiniStrCmp(arg, "-v") == 0) {
    Printf(
        "\033[33mfresh (v2.0.0) - stdTools v1.2.1\n"
        "Copyright (C) 2026 Free Software Foundation, Inc.\n"
        "License GPLv3+: GNU GPL version 3 or later <https://gnu.org>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\033[0m\n");
    syscall1(SYS_EXIT, 0);
  } else {
    Printf("\033[31mFlag: ");
    Printf(arg);
    Printf(" not recognized!\n\033[0m");
    syscall1(SYS_EXIT, 1);
  }
}

#else
#error                                                                         \
    "Target platform architecture not currently supported by standalone fresh module."
#endif
