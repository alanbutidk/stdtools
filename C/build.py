#!/usr/bin/env python
"""
build.py script to build stdtools C tools.
Copyright (c) 2026 Alan. All Rights Reserved.

---------

This file can be modified to your likings. By default, Only use this exact code script.


By default, the recipes this program allows are:

tbuild, tclean :: Related to timestamp
ebuild, eclean :: Related to exec

all :: Build everything.

"""

from sys import argv, platform
from sysconfig import get_path, get_config_var
from os import environ, path
import subprocess as s
from typing import Any, Self

global EXE

if platform == "win32":
    import ctypes as ct

    k32 = ct.windll.kernel32
    HStdOUT = k32.GetStdHandle(-11)
    Mode = ct.c_ulong()
    k32.GetConsoleMode(HStdOUT, ct.byref(Mode))
    Mode.value |= 0x0004
    k32.SetConsoleMode(HStdOUT, Mode)
    EXE = ".exe"
else:
    EXE = ""


def _PrintHelp(Arg: str) -> str | None:
    print(f"""\033[33m
Builder for stdTools::C, {Arg} was used to call help.

Standard Commands:

help/h
version/v

Recipes:

tbuild :: Build timestamp.c to executable
ebuild :: Build exec.c to executable with linked Python lib and DLLs/SOs

tclean :: Clean timestamp executable (if found)
eclean :: Clean exec executable (if found)

all :: Build everything.
\033[0m""")


if len(argv) < 2:
    print("\033[31mNo recipe to build. Use 'help'/'h' for usage.\033[0m")
    raise SystemExit

if argv[1] == "help" or argv[1] == "h":
    _PrintHelp(argv[1])
    raise SystemExit
if argv[1] == "version" or argv[1] == "v":
    raise SystemExit("\033[33mstdTools builder v1.0.0\033[0m")

Arg = argv[1]


def _ebuild() -> None | str:
    PyInclude = get_path("include")
    if platform == "win32":
        CCFlags = f"-IHeaders -ILuaJITHeaders -I{str(__import__('pathlib').Path(PyInclude).resolve()).replace('\\', '/')} -L. -lpython313 -llibluajit5.1 -mconsole"
    else:
        CCFlags = f"-IHeaders -ILuaJITHeaders -I{str(__import__('pathlib').Path(PyInclude).resolve())} -L. -lpython313 -llibluajit51"
    return CCFlags


def _tbuild() -> None | str:
    if platform == "win32":
        CCFlags = "-IHeaders -mconsole"
    else:
        CCFlags = "-IHeaders"
    return CCFlags


class Build:
    def __init__(self):
        pass

    def Core(self, BuildT: bool = False, BuildE: bool = False) -> Any | Any:
        self.CCFlagE = _ebuild()
        self.CCFlagT = _tbuild()
        self.CC = environ.get("CC", "gcc")
        self.TCMD = f"{self.CC} -o timestamp{EXE} timestamp.c {self.CCFlagT}"
        self.ECMD = f"{self.CC} -o exec{EXE} exec.c {self.CCFlagE}"
        if BuildT:
            RUN1 = s.run(self.TCMD, shell=True, capture_output=True, text=True)
            if RUN1.returncode != 0:
                print(
                    f"\033[31mError while compiling: timestamp.c, errno: {RUN1.returncode}, Error output: {RUN1.stderr}\033[0m"
                )
                raise SystemExit
                return None
        if BuildE:
            RUN2 = s.run(self.ECMD, shell=True, capture_output=True, text=True)
            if RUN2.returncode != 0:
                print(
                    f"\033[31mError while compiling: exec.c, errno: {RUN2.returncode}, Error output: {RUN2.stderr}\033[0m"
                )
                raise SystemExit
            return None
        return None

    def EBuild(self) -> Self | None:
        self.Core(BuildT=True)
        return None

    def TBuild(self) -> Self | None:
        self.Core(BuildE=True)

    def All(self) -> Self | None:
        self.Core(BuildT=True, BuildE=True)


Builder = Build()
if Arg == "tclean":
    if platform == "win32":
        __import__("pathlib").Path("timestamp.exe").unlink(missing_ok=True)
    else:
        __import__("pathlib").Path("timestamp").unlink(missing_ok=True)

    raise SystemExit("\033[33mCompleted task: tclean\033[0m")
elif Arg == "eclean":
    if platform == "win32":
        __import__("pathlib").Path("exec.exe").unlink(missing_ok=True)
    else:
        __import__("pathlib").Path("exec").unlink(missing_ok=True)

    raise SystemExit("\033[33mCompled task: eclean\033[0m")
elif Arg == "tbuild":
    Builder.TBuild()
    raise SystemExit("\033[33mFinished task: 'tbuild'\033[0m")
elif Arg == "ebuild":
    Builder.EBuild()
    raise SystemExit("\033[33mFinished task: 'ebuild'\033[0m")
elif Arg == "all":
    Builder.All()
    raise SystemExit("\033[33mFinished task: 'all'\033[0m")
else:
    raise SystemExit(
        f"\033[31mArgument: {Arg} is NOT a recognized way to build. Recommended to use 'help'/'h' for usage\033[0m"
    )
