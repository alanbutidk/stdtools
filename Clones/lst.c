/* lst, List. List contents of a directory, ls clone
 * Use lst --help for help related to lst
 * Copyright (c) 2026 Alan. All Rights Reserved.
 */

#define _GNU_SOURCE

#if defined(_WIN32) || defined(_WIN64)

#define UNICODE
#define _UNICODE

// NOTE: Dont remove the whitespace newline on Line(13)
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <signal.h>
#include <tchar.h>
#include <tlhelp32.h>

#define OS_IS 1

#if defined(_MSC_VER)
#define _CRT_DISABLE_PERF_CRITICAL_LOCKS
#endif

// NoLock fwrite:
#define FWRITE(ptr, size, count, stream)                                       \
  _fwrite_nolock(ptr, size, count, stream)
#define FFLUSH(stream) _fflush_nolock(stream)

#elif defined(__unix__) || defined(__unix) || defined(__linux__) ||            \
    defined(__APPLE__) || defined(__gnu_linux__)

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <unistd.h>

struct LinuxDirent64 {
  ino64_t d_ino;
  off64_t d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[];
};

#define OS_IS 2

// Define fwrite for unlocked or define normal fwrite if no ABI is there.
#if defined(__APPLE__)
#define FWRITE(ptr, size, count, stream) fwrite(ptr, size, count, stream)
#define FFLUSH(ptr, size, count, stream) fflush(ptr, size, count, stream)
#else
#define FWRITE(ptr, size, count, stream)                                       \
  fwrite_unlocked(ptr, size, count, stream)
#define FFLUSH(stream) fflush_unlocked(stream)
#endif

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
#define OUT_STRLEN wcslen
#else
typedef char OutChar;
#define OUT_STRNCPY strncpy
#define OUT_STRCMP strcmp
#define OUT_STRLEN strlen
#endif

volatile sig_atomic_t KeepRunning = 1;

#if OS_IS == 2
static void HandleSig(int Sig) {
  (void)Sig;
  /* Raw write(2), not the buffered FWRITE macro: this runs from a signal
   * handler where touching stdio's internal buffers/locks is unsafe. */
  ssize_t Ignored;
  Ignored = write(STDERR_FILENO, "\033[0m", 4);
  Ignored = write(STDOUT_FILENO, "\033[0m", 4);
  (void)Ignored;
  _exit(EXIT_SUCCESS);
}

#elif OS_IS == 1
BOOL WINAPI WinCtrlHandler(DWORD fdwCtrlType) {

  if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_BREAK_EVENT ||
      fdwCtrlType == CTRL_CLOSE_EVENT || fdwCtrlType == CTRL_LOGOFF_EVENT) {
    _write(2, "\033[0m", 4);
    _write(1, "\033[0m", 4);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
      SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN |
                                        FOREGROUND_BLUE);
    }
    _exit(EXIT_FAILURE);
    return TRUE;
  }
  return FALSE;
}
#endif

/*
 * OutBufReserve lets callers pre-size the buffer once (based on a known
 * upper bound on total bytes) so the hot per-entry write loop never has to
 * check capacity or call realloc. OutBufAppend still exists for the small,
 * one-off writes (help text, summary footer) where growth checks don't
 * matter. */
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

static void OutBufReserve(OutBuf *Ob, size_t MinCap) {
  if (MinCap <= Ob->Cap) {
    return;
  }
  size_t NewCap = Ob->Cap ? Ob->Cap : 65536;
  while (NewCap < MinCap) {
    NewCap *= 2;
  }
  Ob->Buf = realloc(Ob->Buf, NewCap * sizeof(OutChar));
  Ob->Cap = NewCap;
}

static void OutBufAppend(OutBuf *Ob, const OutChar *S, size_t N) {
  if (Ob->Len + N > Ob->Cap) {
    OutBufReserve(Ob, Ob->Len + N);
  }
  memcpy(Ob->Buf + Ob->Len, S, N * sizeof(OutChar));
  Ob->Len += N;
}

/* Unchecked append: caller has already guaranteed capacity via
 * OutBufReserve. Used in the hot per-entry loop to skip the branch and
 * function-call overhead of the checked version. */
static inline void OutBufAppendRaw(OutBuf *Ob, const OutChar *S, size_t N) {
  memcpy(Ob->Buf + Ob->Len, S, N * sizeof(OutChar));
  Ob->Len += N;
}

static void OutBufFlushToNarrowStdout(OutBuf *Ob) {
  /* Raw write(2) straight to fd 1, bypassing stdio's stream layer (locking,
   * buffering, formatting machinery) entirely. We already built the exact
   * bytes we want in Ob->Buf, so a single write() syscall is strictly
   * cheaper than handing it to FWRITE/fflush. Loop to handle short writes,
   * which write() can legally return even for regular pipes/files. */
  size_t Total = Ob->Len * sizeof(OutChar);
  size_t Off = 0;
  const char *Bytes = (const char *)Ob->Buf;
  while (Off < Total) {
    ssize_t N = write(STDOUT_FILENO, Bytes + Off, Total - Off);
    if (N <= 0) {
      break;
    }
    Off += (size_t)N;
  }
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
  size_t NameLen;
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
  int Sort;
  int NoColor;
  int ShowSummary; /* Off by default: statvfs/GetDiskFreeSpaceExW cost a
                       real syscall that dir/ls don't pay in their fast
                       default path. Opt in with -l/--long. */
  int LineByLine;  /* Print each entry as it's found via snprintf/printf,
                       like the original code, instead of buffering
                       everything and dumping it at the end. */
  const char *Target;
} LstOptions;

static int IsTTYStdout(void) {
#if defined(_WIN32) || defined(_WIN64)
  return _isatty(_fileno(stdout));
#else
  return isatty(STDOUT_FILENO);
#endif
}

/* Fixed-width color/format pieces, precomputed once instead of re-derived
 * (or worse, re-parsed by a printf format string) per entry. */
#if defined(_WIN32) || defined(_WIN64)
#define DIR_PRE L"\033[33m"
#define FILE_PRE L"\033[36m"
#define COLOR_POST L"\n\033[0m"
#define PLAIN_POST L"\n"
#else
#define DIR_PRE "\033[33m"
#define FILE_PRE "\033[36m"
#define COLOR_POST "\n\033[0m"
#define PLAIN_POST "\n"
#endif
#define DIR_PRE_LEN 5
#define FILE_PRE_LEN 5
#define COLOR_POST_LEN 5
#define PLAIN_POST_LEN 1

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
  size_t TotalNameBytes = 0;

  char Buffer[65536];
  long NRead;

  while ((NRead = syscall(SYS_getdents64, Fd, Buffer, sizeof(Buffer))) > 0) {
    long Bpos = 0;
    while (Bpos < NRead) {
      struct LinuxDirent64 *D = (struct LinuxDirent64 *)(Buffer + Bpos);

      /* Fold the "." / ".." check and the hidden-file check into a single
       * pass over d_name[0] where possible, avoiding two full strcmp calls
       * per entry for the common case. */
      char C0 = D->d_name[0];
      if (C0 == '.') {
        int IsDot = D->d_name[1] == '\0';
        int IsDotDot = D->d_name[1] == '.' && D->d_name[2] == '\0';
        if (IsDot || IsDotDot) {
          Bpos += D->d_reclen;
          continue;
        }
        if (!Opts->ShowHidden) {
          Bpos += D->d_reclen;
          continue;
        }
      }

      if (EntryCount >= EntryCap) {
        EntryCap = EntryCap ? EntryCap * 2 : 256;
        Entries = realloc(Entries, EntryCap * sizeof(DirEntryRecord));
      }

      size_t NameLen = strlen(D->d_name);
      if (NameLen >= PATH_MAX) {
        NameLen = PATH_MAX - 1;
      }
      memcpy(Entries[EntryCount].Name, D->d_name, NameLen);
      Entries[EntryCount].Name[NameLen] = '\0';
      Entries[EntryCount].NameLen = NameLen;
      Entries[EntryCount].IsDir = (D->d_type == DT_DIR);
      TotalNameBytes += NameLen;
      EntryCount++;

      Bpos += D->d_reclen;
    }
  }

  if (NRead == -1) {
    perror("getdents64");
    Status = 1;
    goto cleanup;
  }
  if (Opts->Sort || Opts->ReverseSort) {
    qsort(Entries, EntryCount, sizeof(DirEntryRecord),
          Opts->ReverseSort ? CompareEntryNameReverse : CompareEntryName);
  }

  OutBuf Ob;
  OutBufInit(&Ob);

  if (Opts->LineByLine) {
    /* Format and print each entry immediately as it's
     * processed, instead of buffering the whole listing. Slower for large
     * dirs (many small writes / printf calls) but streams output as it
     * goes rather than dumping all at once. */
    char Line[PATH_MAX + 32];
    for (size_t i = 0; i < EntryCount; i++) {
      const DirEntryRecord *E = &Entries[i];
      int N;
      if (E->IsDir) {
        FolderCount++;
        N = UseColor
                ? snprintf(Line, sizeof(Line), "\033[33m%s\n\033[0m", E->Name)
                : snprintf(Line, sizeof(Line), "%s\n", E->Name);
      } else {
        FileCount++;
        N = UseColor
                ? snprintf(Line, sizeof(Line), "\033[36m%s\n\033[0m", E->Name)
                : snprintf(Line, sizeof(Line), "%s\n", E->Name);
      }
      FWRITE(Line, 1, (size_t)N, stdout);
    }
    goto summary;
  }

  /* Pre-size once: worst case per entry is prefix + name + colored suffix.
   * This makes the loop below realloc-free regardless of directory size. */
  size_t MaxPerEntry = (UseColor ? DIR_PRE_LEN : 0) +
                       (UseColor ? COLOR_POST_LEN : PLAIN_POST_LEN);
  OutBufReserve(&Ob, TotalNameBytes + EntryCount * MaxPerEntry + 512);

  for (size_t i = 0; i < EntryCount; i++) {
    const DirEntryRecord *E = &Entries[i];
    if (E->IsDir) {
      FolderCount++;
      if (UseColor) {
        OutBufAppendRaw(&Ob, DIR_PRE, DIR_PRE_LEN);
        OutBufAppendRaw(&Ob, E->Name, E->NameLen);
        OutBufAppendRaw(&Ob, COLOR_POST, COLOR_POST_LEN);
      } else {
        OutBufAppendRaw(&Ob, E->Name, E->NameLen);
        OutBufAppendRaw(&Ob, PLAIN_POST, PLAIN_POST_LEN);
      }
    } else {
      FileCount++;
      if (UseColor) {
        OutBufAppendRaw(&Ob, FILE_PRE, FILE_PRE_LEN);
        OutBufAppendRaw(&Ob, E->Name, E->NameLen);
        OutBufAppendRaw(&Ob, COLOR_POST, COLOR_POST_LEN);
      } else {
        OutBufAppendRaw(&Ob, E->Name, E->NameLen);
        OutBufAppendRaw(&Ob, PLAIN_POST, PLAIN_POST_LEN);
      }
    }
  }

summary:

  if (Opts->ShowSummary) {
    struct statvfs Vfs;
    unsigned long long UsedBytes = 0;
    unsigned long long FreeBytes = 0;
    if (statvfs(DirName, &Vfs) == 0) {
      FreeBytes = (unsigned long long)Vfs.f_bavail * Vfs.f_frsize;
      UsedBytes =
          ((unsigned long long)Vfs.f_blocks - Vfs.f_bfree) * Vfs.f_frsize;
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
    int SLen = snprintf(
        Summary, sizeof(Summary),
        "\n%llu of files.\n%llu of folders.\n%.2f %s Used.\n%.2f %s Free.\n",
        FileCount, FolderCount, UsedVal, Units[UIdx], FreeVal, Units[FIdx]);
    OutBufAppend(&Ob, Summary, (size_t)SLen);
  }

  if (Opts->LineByLine) {
    /* Entries already streamed above; only the summary (if any) still lives
     * in Ob and needs flushing. Free Ob's unused capacity either way. */
    if (Ob.Len > 0) {
      OutBufFlushToNarrowStdout(&Ob);
    } else {
      OutBufFree(&Ob);
    }
    FFLUSH(stdout);
  } else {
    OutBufFlushToNarrowStdout(&Ob);
  }

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
  HFind =
      FindFirstFileExW(SzSearchPath, FindExInfoBasic, &Ffd,
                       FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

  if (HFind == INVALID_HANDLE_VALUE) {
    fwprintf(stderr, L"Failed to get file structure access!\n");
    return 1;
  }

  DirEntryRecord *Entries = NULL;
  size_t EntryCap = 0;
  size_t EntryCount = 0;
  size_t TotalNameChars = 0;

  do {
    /* FindExInfoBasic already skips 8.3 short-name generation server-side —
     * that's the main win over a naive FindFirstFile/dir call. */
    wchar_t C0 = Ffd.cFileName[0];
    if (C0 == L'.') {
      int IsDot = Ffd.cFileName[1] == L'\0';
      int IsDotDot = Ffd.cFileName[1] == L'.' && Ffd.cFileName[2] == L'\0';
      if (IsDot || IsDotDot) {
        continue;
      }
      if (!Opts->ShowHidden) {
        continue;
      }
    }

    if (EntryCount >= EntryCap) {
      EntryCap = EntryCap ? EntryCap * 2 : 256;
      Entries = realloc(Entries, EntryCap * sizeof(DirEntryRecord));
    }

    size_t NameLen = wcslen(Ffd.cFileName);
    if (NameLen >= PATH_MAX) {
      NameLen = PATH_MAX - 1;
    }
    memcpy(Entries[EntryCount].Name, Ffd.cFileName, NameLen * sizeof(wchar_t));
    Entries[EntryCount].Name[NameLen] = L'\0';
    Entries[EntryCount].NameLen = NameLen;
    Entries[EntryCount].IsDir =
        (Ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    TotalNameChars += NameLen;
    EntryCount++;
  } while (FindNextFileW(HFind, &Ffd) != 0);

  if (GetLastError() != ERROR_NO_MORE_FILES) {
    fwprintf(stderr, L"Error encountered during directory evaluation!\n");
    Status = 1;
    goto cleanup;
  }

  if (Opts->Sort || Opts->ReverseSort) {
    qsort(Entries, EntryCount, sizeof(DirEntryRecord),
          Opts->ReverseSort ? CompareEntryNameReverse : CompareEntryName);
  }

  OutBuf Ob;
  OutBufInit(&Ob);

  if (Opts->LineByLine) {
    /* Original behavior: format and print each entry immediately, switching
     * stdout to wide mode for the duration since filenames are UTF-16. */
    _setmode(_fileno(stdout), _O_U16TEXT);
    wchar_t Line[PATH_MAX + 32];
    for (size_t i = 0; i < EntryCount; i++) {
      const DirEntryRecord *E = &Entries[i];
      int N;
      if (E->IsDir) {
        FolderCount++;
        N = UseColor ? _snwprintf(Line, PATH_MAX + 32, L"\033[33m%s\n\033[0m",
                                  E->Name)
                     : _snwprintf(Line, PATH_MAX + 32, L"%s\n", E->Name);
      } else {
        FileCount++;
        N = UseColor ? _snwprintf(Line, PATH_MAX + 32, L"\033[36m%s\n\033[0m",
                                  E->Name)
                     : _snwprintf(Line, PATH_MAX + 32, L"%s\n", E->Name);
      }
      fwrite(Line, sizeof(wchar_t), (size_t)N, stdout);
    }
    FFLUSH(stdout);
    _setmode(_fileno(stdout), _O_TEXT);
    goto summary;
  }

  size_t MaxPerEntry = (UseColor ? DIR_PRE_LEN : 0) +
                       (UseColor ? COLOR_POST_LEN : PLAIN_POST_LEN);
  OutBufReserve(&Ob, TotalNameChars + EntryCount * MaxPerEntry + 512);

  for (size_t i = 0; i < EntryCount; i++) {
    const DirEntryRecord *E = &Entries[i];
    if (E->IsDir) {
      FolderCount++;
      if (UseColor) {
        OutBufAppendRaw(&Ob, DIR_PRE, DIR_PRE_LEN);
        OutBufAppendRaw(&Ob, E->Name, E->NameLen);
        OutBufAppendRaw(&Ob, COLOR_POST, COLOR_POST_LEN);
      } else {
        OutBufAppendRaw(&Ob, E->Name, E->NameLen);
        OutBufAppendRaw(&Ob, PLAIN_POST, PLAIN_POST_LEN);
      }
    } else {
      FileCount++;
      if (UseColor) {
        OutBufAppendRaw(&Ob, FILE_PRE, FILE_PRE_LEN);
        OutBufAppendRaw(&Ob, E->Name, E->NameLen);
        OutBufAppendRaw(&Ob, COLOR_POST, COLOR_POST_LEN);
      } else {
        OutBufAppendRaw(&Ob, E->Name, E->NameLen);
        OutBufAppendRaw(&Ob, PLAIN_POST, PLAIN_POST_LEN);
      }
    }
  }

  if (Opts->ShowSummary) {
    ULARGE_INTEGER FreeBytesAvail, TotalBytes, TotalFreeBytes;
    unsigned long long UsedBytes = 0;
    TotalFreeBytes.QuadPart = 0;
    if (GetDiskFreeSpaceExW(SzFullPath, &FreeBytesAvail, &TotalBytes,
                            &TotalFreeBytes)) {
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
    int SLen = _snwprintf(
        Summary, 512,
        L"\n%llu of files.\n%llu of folders.\n%.2f %s Used.\n%.2f %s Free.\n",
        FileCount, FolderCount, UsedVal, Units[UIdx], FreeVal, Units[FIdx]);
    OutBufAppend(&Ob, Summary, (size_t)SLen);
  }

summary:
  /* Wide-mode flush: only this function's output path needs UTF-16 stdout,
     since --help/--version print narrow text before this is ever reached.
     WriteFile bypasses the CRT stdio layer entirely. Cheaper than
     fwrite+fflush for a single big buffered blast. Skipped when LineByLine
     already streamed entries; only a leftover summary (if any) needs it. */

  if (Ob.Len > 0) {

    HANDLE HStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD BytesWritten = 0;
    if (!WriteConsoleW(HStdOut, Ob.Buf, (DWORD)Ob.Len, &BytesWritten, NULL)) {
      WriteFile(HStdOut, Ob.Buf, (DWORD)(Ob.Len * sizeof(wchar_t)),
                &BytesWritten, NULL);
    }
  }
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
#if defined(OS_IS) && OS_IS == 1
  return FindWindowsContents(DirName, Opts);
#elif defined(OS_IS) && OS_IS == 2
  return FindUnixContents(DirName, Opts);
#else
  return EXIT_FAILURE;
#endif
}

static void PrintHelp(const char *ProgName) {
  printf("\033[33mlst %s called:\n"
         "  -a            Show hidden (dotfile) entries\n"
         "  -s            Show sorted order\n"
         "  -r            Reverse sort order\n"
         "  -l, --long    Show file/folder count and disk usage summary\n"
         "                (costs one extra syscall; off by default for speed)\n"
         "  -lbl, --linebyline\n"
         "                Print each entry as it's found instead of\n"
         "                buffering the whole listing and dumping it at the\n"
         "                end (slower for large dirs, streams output)\n"
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
  SetConsoleCtrlHandler(WinCtrlHandler, TRUE);

  HANDLE HOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD DwMode = 0;
  if (GetConsoleMode(HOut, &DwMode)) {
    SetConsoleMode(HOut, DwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
  /* stdout stays narrow (_O_TEXT) by default here: --help/--version use it
     directly. FindWindowsContents switches to _O_U16TEXT itself, only around
     its own wide flush, then switches back. */
#else
  if (signal(SIGINT, HandleSig) == SIG_ERR) {
    printf("Could not register handler!\n");
    return EXIT_FAILURE;
  }
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
    } else if (strcmp(Arg, "-s") == 0) {
      Opts.Sort = 1;
    } else if (strcmp(Arg, "-l") == 0 || strcmp(Arg, "--long") == 0) {
      Opts.ShowSummary = 1;
    } else if (strcmp(Arg, "-lbl") == 0 || strcmp(Arg, "--linebyline") == 0) {
      Opts.LineByLine = 1;
    } else if (strcmp(Arg, "--no-color") == 0) {
      Opts.NoColor = 1;
    } else {
      Opts.Target = Arg;
    }
  }

  return ListDir(Opts.Target, &Opts);
}
