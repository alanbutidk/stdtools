/*inside, the clone for cat and type.
 * Copyright (c) 2026 Alan. All Rights Reserved.
 */

#include "basic.h"
// Now the os.h which includes OS-Dependent headers automatically. (and makes
// OS-Dependent functions into 1 macro'd function name)
#include "os.h"

#define CHUNK_SIZE 65536

int InsideFile(const char *path) {
  FILE_HANDLE fh = SYS_OPEN(path);
  if (fh == INVALID_HANDLE) {
    fprintf(stderr, "\033[31minside: %s: %s\n\033[0m", path, strerror(errno));
    return 1;
  }
  char buf[CHUNK_SIZE];
  for (;;) {
    long n = SYS_READ(fh, buf, CHUNK_SIZE);
    if (n < 0) {
      fprintf(stderr, "\033[31minside: read error on %s\n\033[0m", path);
      SYS_CLOSE(fh);
      return 1;
    }
    if (n == 0)
      break; // EOF
    long written = 0;
    while (written < n) {
    }
  }
}
