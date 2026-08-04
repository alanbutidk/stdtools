/* inside, the clone of cat anc type
 * Copyright (c) 2026 Alan All Rights Reserved.
 * Use --help/-h for usage.
 */

/* --help/-h usage:
 * --help/-h: Print help and exit.
 * --version/-v: Print version and exit.
 * --number/-n: Print till number of line on every line.
*/

#include "Headers/os.h"

int LineNumber = 1;

int PrintStream(FILE *Stream, int NumberLines) {
  int ch;
  int StartOfLine = 1;

  while ((ch = fgetc(Stream)) != EOF) {
    if (NumberLines && StartOfLine) {
      printf("%6d\t", LineNumber++);
      StartOfLine = 0;
    }
    putchar(ch);
    if (ch == '\n') {
      StartOfLine = 1;
    }
  }

  return 0;
}

int PrintHelp(const char *Arg) {
  printf("\033[33minside %s called\n"
         "Look inside a file/Concatenate a file to stdout.\n"
         "      -h | --help: Print Help and Exit\n"
         "      -v | --version: Print version and exit.\n"
         "      -n | --number: Print line numbers on EVERY line.\n"
         "(FEATURE ON NO ARGS): Take stdin and repeat it.\n"
         "(NOTE): -n/--number only works for argv[2]\033[0m",
         Arg);
  return 0;
}

int PrintVer(void) {
  printf("\033[33minside - stdTools v1.2.0\n"
         "Copyright (C) 2026 Alan\n"
         "License GPLv3+: GNU GPL version 3 or later <https://gnu.org>\n"
         "This is free software; you are free to change and redistribute it.\n"
         "There is NO WARRANTY, to the extent permitted by law.\n\033[0m");
  return 0;
}

int main(int argc, char *argv[]) {
  int NumberLines = 0;

  if (argc == 1) {
    // No args: repeat stdin
    return PrintStream(stdin, 0);
  }

  char *Arg = argv[1];

  if (strcmp(Arg, "--help") == 0 || strcmp(Arg, "-h") == 0) {
    PrintHelp(Arg);
    return 0;
  } else if (strcmp(Arg, "--version") == 0 || strcmp(Arg, "-v") == 0) {
    PrintVer();
    return 0;
  } else if (argc > 2 && (strcmp(argv[2], "--number") == 0 || strcmp(argv[2], "-n") == 0)) {
    NumberLines = 1;
  }

  FILE *fp = fopen(Arg, "r");
  if (!fp) {
    perror(Arg);
    return 1;
  }

  PrintStream(fp, NumberLines);
  fclose(fp);
  return 0;
}