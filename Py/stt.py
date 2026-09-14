"""stt.py -> Space to Tab

Useful for languages like python.
If arg is given: st, convert to S->T(Space->Tab)
If arg is given: ts, convert to space from all tabs
If no s found in s->t. Prints:
No space found in file [FILE]
The word space will be replaced by Tab when t->s mode
"""

"""Copyright (c) 2026 Alan. All Rights Reserved."""

# ruff: noqa: E402

from arghandle import (
    ArgHandle,
    IndexOutOfRange,
)  # This is a pip-install library, install via: pip install arghandle==1.3.5
from pathlib import Path
import re

Version = """
SpaceToTab v1.0.0
Copyright (C) 2026 Alan
License GPLv3+: GNU GPL version 3 or later <http://gnu.org>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
"""


# s->t
def SpaceToTab(filename, SpacePerTab=4):
    path = Path(filename)
    if not path.exists():
        raise SystemExit(f"Error: File {filename} does not exist!")
    TempPath = path.with_suffix(path.suffix + ".tmp")
    pattern = re.compile(r"^([ \t]+)")
    FoundSpace = False
    with (
        open(filename, "r", encoding="utf-8") as infile,
        open(TempPath, "w", encoding="utf-8") as outfile,
    ):
        for Line in infile:
            Match = pattern.match(Line)
            if Match:
                Whitespace = Match.group(1)
                Content = Line[len(Whitespace) :]
                TotalSpace = Whitespace.count(" ")
                ExistingTabs = Whitespace.count("\t")
                if TotalSpace > 0:
                    FoundSpace = True
                NewTabs = TotalSpace // SpacePerTab
                RemainderSpaces = TotalSpace % SpacePerTab
                NewIndent = ("\t" * (ExistingTabs + NewTabs)) + " " * RemainderSpaces
                Line = NewIndent + Content
            outfile.write(Line)
    if not FoundSpace:
        TempPath.unlink()
        print(f"No space found in file {filename}")
        return
    TempPath.replace(path)
    print("Finished conversion!")


# t->s
def TabToSpace(filepath, SpacePerTab=4):
    path = Path(filepath)
    if not path.is_file():
        raise SystemExit(f"Error: File '{filepath}' does not exist.")
    TempPath = path.with_suffix(path.suffix + ".tmp")
    pattern = re.compile(r"^([ \t]+)")
    FoundTab = False
    with (
        open(path, "r", encoding="utf-8") as infile,
        open(TempPath, "w", encoding="utf-8") as outfile,
    ):
        for line in infile:
            match = pattern.match(line)
            if match:
                whitespace = match.group(1)
                content = line[len(whitespace) :]
                tab_count = whitespace.count("\t")
                space_count = whitespace.count(" ")
                if tab_count > 0:
                    FoundTab = True
                converted_spaces = tab_count * SpacePerTab
                new_indentation = " " * (space_count + converted_spaces)
                line = new_indentation + content
            outfile.write(line)
    if not FoundTab:
        TempPath.unlink()
        print(f"No tab found in file {filepath}")
        return
    TempPath.replace(path)
    print("Finished conversion!")


# ----------------------------------------------------------------------------------------------------------------------------------------
# Argument handling
cli = ArgHandle("SpaceToTab", "v1.1.0")
cli.PrintOnNoArgs("No Arguments given! use -h or --help for help")
cli.RegisterArg(["ts", "-ts"], HelpMsg="Converts tab to space")
cli.RegisterArg(["st", "-st"], HelpMsg="Convert space to tab")
cli.HandleBasic()

# WE DO NOT WANT TO CHANGE THE THINGS USED HERE. ONLY UPDATE THE VERSION PARAMS!

if cli.ArgCount() < 2:
    file = cli.SetVariableToIndex("File", 2)

    if isinstance(file, IndexOutOfRange):
        raise SystemExit("No file given! Use -h or --help to get usage!")

    if cli.IsArgInActualArgs("ts") or cli.IsArgInActualArgs("-ts"):
        TabToSpace(file, SpacePerTab=4)

    elif cli.IsArgInActualArgs("st") or cli.IsArgInActualArgs("-st"):
        SpaceToTab(file, SpacePerTab=4)

    else:
        cli.ErrorArgPrint("Unknown argument found! Use --help/-h for usage!")
