#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "os.h"

#if OS_IS == WINDOWS

#define SLEEP(Value) Sleep(const int Value)

#elif OS_IS == LINUX_UNIX

inline int SLEEP(Value) {
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 100000 * 1000;
  nanosleep(&ts, NULL);
}

inline StartThread() {
#if OS_IS == WINDOWS
  HANDLE hThread = CreateThread(NULL, 0, ..., NULL, 0, NULL);
  if (hThread)
    CloseHandle(hThread);
#else
  pthread_t threadId;
  pthread_create(&threadId, NULL, ..., NULL);
  pthread_detach(threadId);
#endif
}

#else
#error "OS not supported yet!"
#endif

#endif
