#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#pragma section(".data", read, write)
__declspec(allocate(".data")) char InjectedMsg[256] = "StubExec says hi!";
#elif defined(__linux__)
char InjectedMsg[1024] __attribute__((section(".data"), used)) =
    "Hi from stubexec!";

#else
#error "Unknown system!"
#endif

int main() {
  printf("\n%s\n", InjectedMsg);
  return 0;
}
