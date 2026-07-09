import os
import sys
import shutil
import fnmatch
from pathlib import Path

os.system("")  # Enable ANSI on Windows

if len(sys.argv) < 2:
    print("\x1b[31mArguments not given!\033[0m\nUsage: clean <--command(s)>")
    sys.exit(1)

args = sys.argv[1:]

if any(arg.lower() in ("--help", "-h") for arg in args):
    print(
        "clean --help/-h called, cmds:"
        "\n<DirName>: Just the dir name, no flags..."
        "\n<MultiDirs>: like these: dirname1 dirname2... to delete all of them given as options"
        "\n--force: Forcefully delete a folder, like this: clean MyDir1 --force"
        '\n--except: Deletes everything in a folder except the argument given, like this: clean MyDir1 --except "*.py"'
        "\n--version: Print version and exit."
    )
    sys.exit(0)

if any(arg.lower() in ("--version", "-v") for arg in args):
    print("""
Clean - stdtools v1.0.0
Copyright (C) 2026 Alan
License GPLv3+: GNU GPL version 3 or later <http://gnu.org>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
    """)
    sys.exit(0)

# Argument parsing
dirlist = []
flags = {}
i = 0
while i < len(args):
    arg = args[i]
    if arg == "--except":
        if i + 1 >= len(args):
            print(
                "\x1b[31m--except requires a pattern argument, e.g. --except *.py\033[0m"
            )
            sys.exit(1)
        flags["except"] = args[i + 1]
        i += 2
    elif arg == "--force":
        flags["force"] = True
        i += 1
    elif arg == "--version":
        flags["version"] = True
        i + 1
    elif arg.startswith("--"):
        print(f"\x1b[31mUnknown flag: {arg}\033[0m")
        sys.exit(1)
    else:
        dirlist.append(arg)
        i += 1

if not dirlist:
    print("\x1b[31mNo directories given!\033[0m")
    sys.exit(1)

# Feature logics

# --except: delete everything in dir(s) except files matching the pattern
if "except" in flags:
    pattern = flags["except"]
    for d in dirlist:
        path = Path(d)
        if not path.exists() or not path.is_dir():
            print(f"\x1b[31mDirectory not found: {d}\033[0m")
            continue
        deleted = 0
        for item in path.iterdir():
            if not fnmatch.fnmatch(item.name, pattern):
                if item.is_dir():
                    shutil.rmtree(item)
                else:
                    item.unlink()
                deleted += 1
        print(f"Cleaned '{d}' ({deleted} items removed), kept: {pattern}")

elif "force" in flags:
    for d in dirlist:
        try:
            shutil.rmtree(d)
            print(f"Force deleted: {d}")
        except FileNotFoundError:
            print(f"\x1b[31mNot found: {d}\033[0m")
        except Exception as e:
            print(f"\x1b[31mFailed to delete {d}: {e}\033[0m")

else:
    for d in dirlist:
        path = Path(d)
        if path.exists() and path.is_dir():
            shutil.rmtree(path)
            print(f"Deleted: {d}")
        else:
            print(f"\x1b[31mDirectory not found: {d}\033[0m")

