#ifndef FUNCTIONS_H
#define FUNCTIONS_H

// we need os.h
#include "os.h"
#if defined(OS_IS) && OS_IS == WINDOWS
static inline int EnableVT100(void) {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) {
    return GetLastError();
  }
  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode)) {
    return GetLastError();
  }
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(hOut, dwMode)) {
    return GetLastError();
  }
  return 0;
}
#endif
#endif
