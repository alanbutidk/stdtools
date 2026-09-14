# Standard Tools (stdTools)
**stdTools** or **Standard Tools** is a collection of tools written mostly in python.

These tools are either:

- Clones (ls/dir, cls/clear, cat/type)
- Custom python written tools
- Custom C written tools
- Custom shell scripts.

---

> Python scripts use arghandle, a custom-built arg-parsing lib that provides click-based
support out-of-the-box. Using only 1/2 libraries (which are stdlib-already).

The clones are heavily optimized and measured:
```sh
$ time lst C:\Windows\System32
...
xpspushlayer.dll
XpsRasterService.dll
xpsservices.dll
XpsToPclmConverter.dll
XpsToPwgrConverter.dll
xwizard.dtd
xwizard.exe
xwizards.dll
xwreg.dll
xwtpdui.dll
xwtpw32.dll
X_80.contrast-black.png
X_80.contrast-white.png
X_80.png
zh-CN
zh-TW
zipcontainer.dll
zipfldr.dll
ztrace_maps.dll
{A6D608F0-0BDE-491A-97AE-5C4B05D86E01}.bat
{EC94D02F-D200-4428-9531-05AF7F9799CB}.bat

real    0m0.983s
user    0m0.000s
sys     0m0.015s

$ time inside inside.c
```
```c
//...

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
```
```sh
real    0m0.250s
user    0m0.000s
sys     0m0.015s
```

*inside measuring inside.c -- The source code of inside itself.* 

## build.py

There is a super complicated build-script avaliable to only the C directory in this repo.

The usage for the latter is:
```bash
python build.py help # Or 'h'
python build.py version # or 'v'
python build.py all # build everything
python build.py ebuild # build exec
python build.py tbuild # build timestamp
```

### License
**stdTools** is licensed under GPLv3+

View: [LICENSE](./LICENSE)
