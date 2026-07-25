/*fresh, the screen cleaner. Copy of cls & clear
 * Use fresh --help/-h for help.
 * Copyright (c) 2026 Alan. All Rights Reserved.
 */
#include "os.h"

int ClearScreen() {
#if OS_IS == WINDOWS
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD count;
  DWORD cellCount;
  COORD homeCoords = {0, 0};
  if (hConsole == INVALID_HANDLE_VALUE)
    return 1;
  if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    return 1;
  cellCount = csbi.dwSize.X * csbi.dwSize.Y;

  FillConsoleOutputCharacter(hConsole, (TCHAR)' ', cellCount, homeCoords,
                             &count);
  FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords,
                             &count);
  SetConsoleCursorPosition(hConsole, homeCoords);
#elif OS_IS == LINUX_UNIX
  printf("\e[1J\e[2J\e[3J\e[H");
  fflush(stdout);
#else
  return 1;
#endif
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    ClearScreen();
    return 0;
  }
  char *Arg = argv[1];
  if (strcmp("--help", Arg) == 0 || strcmp("-h", Arg) == 0) {
    printf("\033[33mfresh %s called\n", Arg);
    printf("--help/-h: Print help and exit.\n");
    printf("--version/-v: Print version info\n\033[0m");
    return 0;
  } else if (strcmp("--version", Arg) == 0 || strcmp("-v", Arg) == 0) {
    printf(
        "\033[33mfresh - stdTools v1.0.0\n"
        "Copyright (C) 2026 Free Software Foundation, Inc.\n"
        "License GPLv3+: GNU GPL version 3 or later <https://gnu.org>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\033[0m\n");
    return 0;
  } else {
    printf("\033[31mFlag: %s not recognized!\n\033[0m", Arg);
    return 1;
  }
  return 0;
}
