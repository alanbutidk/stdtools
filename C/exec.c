/* exec -> Execute a program
 * Copyright (c) 2026 Alan. All Rights Reserved.
 */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "os.h"

// This part of the file is completly dedicated to Python.

int _pyexec_init(void) {
  Py_Initialize();
  return 0;
}

PyStatus _pyexec_initconfig(PyConfig Config) {
  return Py_InitializeFromConfig(&Config);
}

int _pyexec_close(void) {
  Py_Finalize();
  return 0;
}

int _pyexec_close_ex(void) {
  Py_FinalizeEx();
  return 0;
}

int _pyexec_runfile(int argc, char *argv[], const char *Filename) {
  PyStatus Status;
  PyConfig Config;

  PyConfig_InitPythonConfig(&Config);
  wchar_t **wargv = malloc(argc * sizeof(wchar_t *));
  if (!wargv) {
    PyConfig_Clear(&Config);
    return -1;
  }

  for (int i = 0; i < argc; i++) {
    wargv[i] = Py_DecodeLocale(argv[i], NULL);
  }

  Status = PyConfig_SetArgv(&Config, argc, wargv);
  if (PyStatus_Exception(Status)) {
    for (int i = 0; i < argc; i++)
      PyMem_RawFree(wargv[i]);
    free(wargv);
    PyConfig_Clear(&Config);
    Py_ExitStatusException(Status);
  }

  Status = _pyexec_initconfig(Config);
  PyConfig_Clear(&Config);

  if (PyStatus_Exception(Status)) {
    Py_ExitStatusException(Status);
  }

  FILE *fp = fopen(Filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "Could not execute python file %s\n", Filename);
    return -1;
  }
  PyRun_SimpleFile(fp, Filename);
  fclose(fp);
  _pyexec_close_ex();
  return 0;
}

// End of python runners.

// This part of the file is dedicated to Lua, or LuaJIT.

#include "lauxlib.h"
#include "lua.h"
#include "luajit.h"
#include "lualib.h"

int _luaexec_runfile(int argc, char *argv[], const char *Filename) {
  lua_State *L = luaL_newstate();
  if (!L)
    return 1;
  luaL_openlibs(L);
  luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

  lua_createtable(L, argc, 0);
  for (int i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "arg");

  if (luaL_dofile(L, Filename) != 0) {
    fprintf(stderr, "Error while execution: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  }
  lua_close(L);
  return 0;
}

// End of Lua runners.

// Internal function to determine the suffix is same.
bool _HasSuffix(const char *str, const char *suffix) {
  if (!str || !suffix)
    return false;
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  if (str_len < suffix_len)
    return false;
  return strcmp(str + (str_len - suffix_len), suffix) == 0;
}

int main(int argc, char *argv[]) {
  int _PySuffix = 0;
  int _LuaSuffix = 0;
  if (argc == 1) {
    printf("\033[31mNo arguments given! Use --help/-h for usage.\n\033[0m");
    return 1;
  }
  char *Arg = argv[1];

  if (_HasSuffix(Arg, ".py")) {
    int _PySuffix = 1;
  } else if (_HasSuffix(Arg, ".lua")) {
    int _LuaSuffix = 1;
  } else {
    return 1;
  }
  if (_PySuffix == 1) {
    _pyexec_runfile(argc, argv, Arg);
    return 0;
  } else if (_LuaSuffix == 1) {
    _luaexec_runfile(argc, argv, Arg);
    return 0;
  } else {
    return EXIT_FAILURE;
  }

  return 0;
}
