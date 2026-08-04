import os
import shutil
from pathlib import Path
from arghandle import ArgHandle, ArgNotFound, IndexOutOfRange

# PCCAlwaysName is always __pycache__
PCCAlwaysName: str = "__pycache__"


def DeletePyCacheDirRecursive(RootDir, TargetName: str = PCCAlwaysName):
    try:
        with os.scandir(RootDir) as E:
            for e in E:
                if e.is_dir(follow_symlinks=True):
                    if e.name == "__pycache__":
                        shutil.rmtree(e.path)
                        print(f"Deleted {e.path}!")
                    else:
                        DeletePyCacheDirRecursive(e.path, PCCAlwaysName)
    except PermissionError:
        pass


# PCC Version Info
Version = """
PyCacheCleaner - PyStdtools v1.0.0
Copyright (C) 2026 Alan
License GPLv3+: GNU GPL version 3 or later <https://gnu.org>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
"""

# Register arguments:
Cli = ArgHandle("PyCacheCleaner", "v1.1.0")
Cli.RegisterArg(
    ["--path", "-p"],
    StrictIndex=1,
    StrictIndex_ExitOnError=True,
    HelpMsg="Path where pycache is (except pwd)",
)
Cli.RegisterArg(
    ["--recursive", "-r"],
    HelpMsg="Recursively delete ALL __pycache__ directories",
)
Cli.HandleBasic()

# Handling starts:
HasPathArg = Cli.IsArgInActualArgs("--path") or Cli.IsArgInActualArgs("-p")

if HasPathArg:
    PathIndex = Cli.WhereArg("--path")
    if isinstance(PathIndex, ArgNotFound):
        PathIndex = Cli.WhereArg("-p")
    if isinstance(PathIndex, ArgNotFound):
        raise SystemExit("Could not locate --path/-p in arguments.")
    ActualPathIndex = PathIndex + 2
    PathLoc = Cli.SetVariableToIndex("PathLoc", ActualPathIndex)
    if isinstance(PathLoc, IndexOutOfRange):
        raise SystemExit("No value provided after --path/-p")
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
    if Cli.IsArgInActualArgs("--recursive") or Cli.IsArgInActualArgs("-r"):
        NextAfterRecursive = Cli.NextAfter("--recursive") or Cli.NextAfter("-r")
        DeletePyCacheDirRecursive(NextAfterRecursive)
        raise SystemExit


# TODO:
# Fix the bug in handling NextAfter()

# NOTE:
# You'll need to have a new update for ArgHandle that fixes this bug. (So v2.2.0)
