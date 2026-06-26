import shutil
from pathlib import Path
import sys
from arghandle import ArgHandle

# PCC Version Info
Version = """
PyCacheCleaner - PyStdtools v1.0.0
Copyright (C) 2026 Alan
License GPLv3+: GNU GPL version 3 or later <http://gnu.org>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
"""


# PCCAlwaysName is always pycache.
PCCAlwaysName: str = "__pycache__"

# Register arguments:
Cli = ArgHandle()
Cli.ProgramName("PyCacheCleaner")
Cli.RegisterArg(
    ["--path", "-p"],
    StrictIndex=1,
    StrictIndex_ExitOnError=True,
    HelpMsg="Path where pycache is (except pwd)",
)
Cli.RegisterArg(
    ["--version", "-v"],
    StrictIndex=1,
    StrictIndex_ExitOnError=True,
    HelpMsg="Print version info.",
)
Cli.HandleHelp()

# Handling starts:
HasPathArg = Cli.IsArgInActualArgs("--path") or Cli.IsArgInActualArgs("-p")
HasVersionArg = Cli.IsArgInActualArgs("--version") or Cli.IsArgInActualArgs("-v")

if HasVersionArg:
    print(Version)
    raise SystemExit  # because our job is done.

if HasPathArg:
    PathIndex = (Cli.WhereArg("--path") or Cli.WhereArg("-p")) + 1
    PathLoc = ArgHandle.SetVariableToIndex(PathIndex)
    TargetDir = Path(PathLoc)
else:
    TargetDir = Path(".")

PycachePath: Path = TargetDir / PCCAlwaysName

if TargetDir.exists() and PycachePath.is_dir():
    try:
        shutil.rmtree(PycachePath)
        print(f"Removed: {PycachePath.resolve()}")
    except (OSError, PermissionError) as e:
        print(f"Error removing {PycachePath}: {e}")
else:
    print(f"No {PCCAlwaysName} directory found in: {TargetDir.resolve()}")

