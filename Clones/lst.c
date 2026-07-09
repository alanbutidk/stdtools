/* lst, List. List contents of a directory, ls clone
 * Use lst --help for help related to lst
 * Copyright (c) 2026 Alan. All Rights Reserved.
 */

#if defined(_WIN32) || defined(_WIN64)

#define UNICODE
#define _UNICODE
#include <fcntl.h>
#include <io.h>
#include <tchar.h>
#include <windows.h>
#include <tlhelp32.h>

#define OS_IS WINDOWS_OS

#elif defined(__unix__) || defined(__unix) || defined(__linux__) ||            \
    defined(__APPLE__)

#include <dirent.h>
#include <fcntl.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

struct LinuxDirent64 {
  ino64_t d_ino;
  off64_t d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[];
};

#define OS_IS LINUX_AND_UNIX

#else

#error "Unknown OS Detected!"

#endif

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#if defined(_WIN32) || defined(_WIN64)
typedef wchar_t OutChar;
#define OUT_STRNCPY wcsncpy
#define OUT_STRCMP wcscmp
#else
typedef char OutChar;
#define OUT_STRNCPY strncpy
#define OUT_STRCMP strcmp
#endif

typedef struct {
  OutChar *Buf;
  size_t Len;
  size_t Cap;
} OutBuf;

static void OutBufInit(OutBuf *Ob) {
  Ob->Cap = 65536;
  Ob->Buf = malloc(Ob->Cap * sizeof(OutChar));
  Ob->Len = 0;
}

static void OutBufAppend(OutBuf *Ob, const OutChar *S, size_t N) {
  if (Ob->Len + N > Ob->Cap) {
    while (Ob->Len + N > Ob->Cap) Ob->Cap *= 2;
    Ob->Buf = realloc(Ob->Buf, Ob->Cap * sizeof(OutChar));
  }
  memcpy(Ob->Buf + Ob->Len, S, N * sizeof(OutChar));
  Ob->Len += N;
}

static void OutBufFlushToNarrowStdout(OutBuf *Ob) {
  /* Narrow-mode stdout: used on Linux, and on Windows only for --help/--version,
     which never touch this buffer. Kept for API symmetry. */
  fwrite(Ob->Buf, sizeof(OutChar), Ob->Len, stdout);
  free(Ob->Buf);
  Ob->Buf = NULL;
  Ob->Len = 0;
  Ob->Cap = 0;
}

static void OutBufFree(OutBuf *Ob) {
  free(Ob->Buf);
  Ob->Buf = NULL;
  Ob->Len = 0;
  Ob->Cap = 0;
}

typedef struct {
  OutChar Name[PATH_MAX];
  int IsDir;
} DirEntryRecord;

static int CompareEntryName(const void *A, const void *B) {
  const DirEntryRecord *Ea = A;
  const DirEntryRecord *Eb = B;
  return OUT_STRCMP(Ea->Name, Eb->Name);
}

static int CompareEntryNameReverse(const void *A, const void *B) {
  return -CompareEntryName(A, B);
}

typedef struct {
  int ShowHidden;
  int ReverseSort;
  int NoColor;
  const char *Target;
} LstOptions;

static int IsTTYStdout(void) {
#if defined(_WIN32) || defined(_WIN64)
  return _isatty(_fileno(stdout));
#else
  return isatty(STDOUT_FILENO);
#endif
}

#if defined(__unix__) || defined(__unix) || defined(__linux__) ||              \
    defined(__APPLE__)

int FindUnixContents(const char *DirName, LstOptions *Opts) {
  int Status = 0;
  unsigned long long FileCount = 0;
  unsigned long long FolderCount = 0;
  int UseColor = !Opts->NoColor && IsTTYStdout();

  int Fd = open(DirName, O_RDONLY | O_DIRECTORY);
  if (Fd == -1) {
    perror("open");
    return 1;
  }

  DirEntryRecord *Entries = NULL;
  size_t EntryCap = 0;
  size_t EntryCount = 0;

  char Buffer[65536];
  long NRead;

  while ((NRead = syscall(SYS_getdents64, Fd, Buffer, sizeof(Buffer))) > 0) {
    long Bpos = 0;
    while (Bpos < NRead) {
      struct LinuxDirent64 *D = (struct LinuxDirent64 *)(Buffer + Bpos);

      int IsDot = strcmp(D->d_name, ".") == 0;
      int IsDotDot = strcmp(D->d_name, "..") == 0;

      if (IsDot || IsDotDot) {
        Bpos += D->d_reclen;
        continue;
      }

      if (!Opts->ShowHidden && D->d_name[0] == '.') {
        Bpos += D->d_reclen;
        continue;
      }

      if (EntryCount >= EntryCap) {
        EntryCap = EntryCap ? EntryCap * 2 : 256;
        Entries = realloc(Entries, EntryCap * sizeof(DirEntryRecord));
      }

      OUT_STRNCPY(Entries[EntryCount].Name, D->d_name, PATH_MAX - 1);
      Entries[EntryCount].Name[PATH_MAX - 1] = '\0';
      Entries[EntryCount].IsDir = (D->d_type == DT_DIR);
      EntryCount++;

      Bpos += D->d_reclen;
    }
  }

  if (NRead == -1) {
    perror("getdents64");
    Status = 1;
    goto cleanup;
  }

  qsort(Entries, EntryCount, sizeof(DirEntryRecord),
        Opts->ReverseSort ? CompareEntryNameReverse : CompareEntryName);

  OutBuf Ob;
  OutBufInit(&Ob);

  char Line[PATH_MAX + 32];
  for (size_t i = 0; i < EntryCount; i++) {
    int N;
    if (Entries[i].IsDir) {
      FolderCount++;
      N = UseColor ? snprintf(Line, sizeof(Line), "\033[33m%s\n\033[0m", Entries[i].Name)
                    : snprintf(Line, sizeof(Line), "%s\n", Entries[i].Name);
    } else {
      FileCount++;
      N = UseColor ? snprintf(Line, sizeof(Line), "\033[36m%s\n\033[0m", Entries[i].Name)
                    : snprintf(Line, sizeof(Line), "%s\n", Entries[i].Name);
    }
    OutBufAppend(&Ob, Line, (size_t)N);
  }

  struct statvfs Vfs;
  unsigned long long UsedBytes = 0;
  unsigned long long FreeBytes = 0;
  if (statvfs(DirName, &Vfs) == 0) {
    FreeBytes = (unsigned long long)Vfs.f_bavail * Vfs.f_frsize;
    UsedBytes = ((unsigned long long)Vfs.f_blocks - Vfs.f_bfree) * Vfs.f_frsize;
  }

  const char *Units[] = {"B", "KB", "MB", "GB", "TB"};
  double UsedVal = (double)UsedBytes;
  int UIdx = 0;
  while (UsedVal >= 1024 && UIdx < 4) {
    UsedVal /= 1024;
    UIdx++;
  }

  double FreeVal = (double)FreeBytes;
  int FIdx = 0;
  while (FreeVal >= 1024 && FIdx < 4) {
    FreeVal /= 1024;
    FIdx++;
  }

  char Summary[512];
  int SLen = snprintf(Summary, sizeof(Summary),
                       "\n%llu of files.\n%llu of folders.\n%.2f %s Used.\n%.2f %s Free.\n",
                       FileCount, FolderCount, UsedVal, Units[UIdx], FreeVal, Units[FIdx]);
  OutBufAppend(&Ob, Summary, (size_t)SLen);

  OutBufFlushToNarrowStdout(&Ob);

cleanup:
  free(Entries);
  if (Fd != -1) {
    close(Fd);
  }
  return Status;
}

#endif /* Linux/Unix */

#if defined(_WIN32) || defined(_WIN64)

int FindWindowsContents(const char *DirName, LstOptions *Opts) {
  int Status = 0;
  HANDLE HFind = INVALID_HANDLE_VALUE;
  unsigned long long FileCount = 0;
  unsigned long long FolderCount = 0;
  int UseColor = !Opts->NoColor && IsTTYStdout();

  if (strcmp(DirName, "...") == 0) {
    fwprintf(stderr, L"Failed to get file! (Invalid relative token)\n");
    return 1;
  }

  wchar_t SzTargetDir[MAX_PATH];
  MultiByteToWideChar(CP_UTF8, 0, DirName, -1, SzTargetDir, MAX_PATH);

  wchar_t SzFullPath[MAX_PATH];
  if (GetFullPathNameW(SzTargetDir, MAX_PATH, SzFullPath, NULL) == 0) {
    fwprintf(stderr, L"Failed to get file path accuracy!\n");
    return 1;
  }

  wchar_t SzSearchPath[MAX_PATH];
  size_t Len = wcslen(SzFullPath);

  if (Len == 0 || SzFullPath[Len - 1] == L'\\') {
    _snwprintf(SzSearchPath, MAX_PATH, L"%s*", SzFullPath);
  } else {
    _snwprintf(SzSearchPath, MAX_PATH, L"%s\\*", SzFullPath);
  }

  WIN32_FIND_DATAW Ffd;
  HFind = FindFirstFileW(SzSearchPath, &Ffd);
  if (HFind == INVALID_HANDLE_VALUE) {
    fwprintf(stderr, L"Failed to get file structure access!\n");
    return 1;
  }

  DirEntryRecord *Entries = NULL;
  size_t EntryCap = 0;
  size_t EntryCount = 0;

  do {
    if (wcscmp(Ffd.cFileName, L".") == 0 || wcscmp(Ffd.cFileName, L"..") == 0) {
      continue;
    }

    if (!Opts->ShowHidden && Ffd.cFileName[0] == L'.') {
      continue;
    }

    if (EntryCount >= EntryCap) {
      EntryCap = EntryCap ? EntryCap * 2 : 256;
      Entries = realloc(Entries, EntryCap * sizeof(DirEntryRecord));
    }

    wcsncpy(Entries[EntryCount].Name, Ffd.cFileName, PATH_MAX - 1);
    Entries[EntryCount].Name[PATH_MAX - 1] = L'\0';
    Entries[EntryCount].IsDir =
        (Ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    EntryCount++;
  } while (FindNextFileW(HFind, &Ffd) != 0);

  if (GetLastError() != ERROR_NO_MORE_FILES) {
    fwprintf(stderr, L"Error encountered during directory evaluation!\n");
    Status = 1;
    goto cleanup;
  }

  qsort(Entries, EntryCount, sizeof(DirEntryRecord),
        Opts->ReverseSort ? CompareEntryNameReverse : CompareEntryName);

  OutBuf Ob;
  OutBufInit(&Ob);

  wchar_t Line[PATH_MAX + 32];
  for (size_t i = 0; i < EntryCount; i++) {
    int N;
    if (Entries[i].IsDir) {
      FolderCount++;
      N = UseColor ? _snwprintf(Line, PATH_MAX + 32, L"\033[33m%s\n\033[0m", Entries[i].Name)
                    : _snwprintf(Line, PATH_MAX + 32, L"%s\n", Entries[i].Name);
    } else {
      FileCount++;
      N = UseColor ? _snwprintf(Line, PATH_MAX + 32, L"\033[36m%s\n\033[0m", Entries[i].Name)
                    : _snwprintf(Line, PATH_MAX + 32, L"%s\n", Entries[i].Name);
    }
    OutBufAppend(&Ob, Line, (size_t)N);
  }

  ULARGE_INTEGER FreeBytesAvail, TotalBytes, TotalFreeBytes;
  unsigned long long UsedBytes = 0;
  TotalFreeBytes.QuadPart = 0;
  if (GetDiskFreeSpaceExW(SzFullPath, &FreeBytesAvail, &TotalBytes, &TotalFreeBytes)) {
    UsedBytes = TotalBytes.QuadPart - TotalFreeBytes.QuadPart;
  }

  const wchar_t *Units[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
  double UsedVal = (double)UsedBytes;
  int UIdx = 0;
  while (UsedVal >= 1024 && UIdx < 4) {
    UsedVal /= 1024;
    UIdx++;
  }

  double FreeVal = (double)TotalFreeBytes.QuadPart;
  int FIdx = 0;
  while (FreeVal >= 1024 && FIdx < 4) {
    FreeVal /= 1024;
    FIdx++;
  }

  wchar_t Summary[512];
  int SLen = _snwprintf(Summary, 512,
                         L"\n%llu of files.\n%llu of folders.\n%.2f %s Used.\n%.2f %s Free.\n",
                         FileCount, FolderCount, UsedVal, Units[UIdx], FreeVal, Units[FIdx]);
  OutBufAppend(&Ob, Summary, (size_t)SLen);

  /* Wide-mode flush: only this function's output path needs UTF-16 stdout,
     since --help/--version print narrow text before this is ever reached. */
  _setmode(_fileno(stdout), _O_U16TEXT);
  fwrite(Ob.Buf, sizeof(wchar_t), Ob.Len, stdout);
  fflush(stdout);
  _setmode(_fileno(stdout), _O_TEXT);
  OutBufFree(&Ob);

cleanup:
  free(Entries);
  if (HFind != INVALID_HANDLE_VALUE) {
    FindClose(HFind);
  }
  return Status;
}

#endif /* Windows */

int ListDir(const char *DirName, LstOptions *Opts) {
#if defined(OS_IS) && OS_IS == WINDOWS_OS
  return FindWindowsContents(DirName, Opts);
#elif defined(OS_IS) && OS_IS == LINUX_AND_UNIX
  return FindUnixContents(DirName, Opts);
#else
  return EXIT_FAILURE;
#endif
}

static void PrintHelp(const char *ProgName) {
  printf("\033[33mlst %s called:\n"
         "  -a            Show hidden (dotfile) entries\n"
         "  -r            Reverse sort order\n"
         "  --no-color    Disable ANSI color output\n"
         "  --help, -h    Print this help and exit\n"
         "  --version, -v Print version and exit\n\033[0m",
         ProgName);
}

static void PrintVersion(void) {
  printf("\033[33mlst - stdTools(CLONES) v1.2.0\n"
         "Copyright (C) 2026 Alan\n"
         "License GPLv3+: GNU GPL version 3 or later <https://gnu.org>.\n"
         "This is free software: you are free to change and redistribute it.\n"
         "There is NO WARRANTY, to the extent permitted by law.\n\033[0m");
}

int main(int argc, char *argv[]) {
#if defined(_WIN32) || defined(_WIN64)
  HANDLE HOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD DwMode = 0;
  if (GetConsoleMode(HOut, &DwMode)) {
    SetConsoleMode(HOut, DwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
  /* stdout stays narrow (_O_TEXT) by default here — --help/--version use it
     directly. FindWindowsContents switches to _O_U16TEXT itself, only around
     its own wide flush, then switches back. */
#endif

  LstOptions Opts = {0};
  Opts.Target = ".";

  for (int i = 1; i < argc; i++) {
    const char *Arg = argv[i];

    if (strcmp(Arg, "--help") == 0 || strcmp(Arg, "-h") == 0) {
      PrintHelp(Arg);
      return EXIT_SUCCESS;
    } else if (strcmp(Arg, "--version") == 0 || strcmp(Arg, "-v") == 0) {
      PrintVersion();
      return EXIT_SUCCESS;
    } else if (strcmp(Arg, "-a") == 0) {
      Opts.ShowHidden = 1;
    } else if (strcmp(Arg, "-r") == 0) {
      Opts.ReverseSort = 1;
    } else if (strcmp(Arg, "--no-color") == 0) {
      Opts.NoColor = 1;
    } else {
      Opts.Target = Arg;
    }
  }

  return ListDir(Opts.Target, &Opts);
}