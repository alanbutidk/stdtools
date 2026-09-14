#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#if _MSC_VER
#pragma section(".data", read, write)
__declspec(allocate(".data")) char InjectedMsg[2048] = "StubExec says hi!\n";
#elif defined(__linux__) || defined(__unix__) || defined(__clang__) ||         \
    defined(__GNUC__) || defined(__GNUG__)
#include <unistd.h>
char InjectedMsg[2048] __attribute__((section(".data"), used)) =
    "Hi from stubexec!\n";

#else
#error "Unknown system!"
#endif

int main(void)
{
#if defined(_WIN32) || defined(_WIN64)
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

  WriteConsoleA(hOut, InjectedMsg, strlen(InjectedMsg), NULL, 0);
  WriteConsoleA(hOut, "\n", 2, NULL, 0);
#elif defined(__linux__) || defined(__unix__)
  write(1, InfectedMsg, strlen(InjectedMsg));
  write(1, "\n", 1);
#endif
  return 0;
}
