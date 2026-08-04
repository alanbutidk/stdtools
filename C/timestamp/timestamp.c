#define INCLUDE_SYSHEADERS
#include "os.h"

int Windows(const char *File) {
  struct __stat64 file_stat;
  if (_stat64(File, &file_stat) != 0) {
    perror("\033[31mError reading file timestamps!\033[0m");
    return 1;
  }
  __time64_t ModTime = file_stat.st_mtime;
  __time64_t AccTime = file_stat.st_atime;
  __time64_t CreaTime = file_stat.st_ctime;
  char TimeBuff[512];
  _ctime64_s(TimeBuff, sizeof(TimeBuff), &ModTime);
  printf("\033[33mLast modified: %s\n\033[0m", TimeBuff);
  _ctime64_s(TimeBuff, sizeof(TimeBuff), &AccTime);
  printf("\033[33mLast accessed: %s\n\033[0m", TimeBuff);
  _ctime64_s(TimeBuff, sizeof(TimeBuff), &CreaTime);
  printf("\033[33mCreated: %s\n\033[0m", TimeBuff);
  return 0;
}

int Linux(const char *File) {
  struct stat FileStat;
  if (stat(File, &FileStat) == -1) {
    perror("\033[31mError reading file properties/stats!\033[0m\n");
    return 1;
  }
  time_t ModTime = FileStat.st_mtime;
  time_t AccTime = FileStat.st_atime;
  time_t StatusTime = FileStat.st_ctime;
  printf("\033[33m\033Last modified: %s\033[0m\n", ctime(&ModTime));
  printf("\033[33mLast accessed: %s\033[0m", ctime(&AccTime));
  return 0;
}

int main(int argc, char *argv[]) {

// Enable VT100 for windows.
#if OS_IS == WINDOWS
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE)
    return 1;
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
#else
#endif

  if (argc != 2) {
    printf("No args given! Use --help/-h for help\n");
    return EXIT_FAILURE;
  }
  char *FileName = argv[1];
  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    printf("\033[33mtimestamp %s called:\n", FileName);
    printf("--help/-h: Print this help screen.\n");
    printf("--version/-v: Print the version.\n");
    printf("\nUsage: %s <FILE_NAME>\n\033[0m", argv[0]);
    return 0;
  }

  if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
    printf("\033[33mtimestamp - stdTools v1.0.0\n");
    printf("Copyright (C) 2026 Alan\n");
    printf("License GPLv3+: GNU GPL version 3 or later <https://gnu.org>\n");
    printf(
        "This is free software: you are free to change and redistribute it.\n");
    printf("There is NO WARRANTY, to the extent permitted by law.\n\033[0m");
    return 0;
  }
// Main thing.
#if OS_IS == WINDOWS
  Windows(FileName);
#elif OS_IS == LINUX_UNIX
  Linux(FileName);
#else
  return EXIT_FAILURE;
#endif

  return EXIT_SUCCESS;
}
