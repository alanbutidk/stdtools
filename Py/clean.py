import shutil
import fnmatch
import sys
from pathlib import Path
from arghandle import ArgHandle

cli = ArgHandle("clean", "v2.0.0")

cli.RegisterArg(
    ["-e", "--except"],
    Type=str,
    HelpMsg='Delete everything in a folder except the given pattern, e.g. --except "*.py"',
)
cli.RegisterArg(
    ["--force", "-f"],
    Type=bool,
    HelpMsg="Forcefully delete a folder without confirmation",
)


cli.PrintOnNoArgs("No arguments provided! Call --help/-h for usage.")
cli.HandleBasic()

dirlist = []
skip_next = False
for i, arg in enumerate(sys.argv[1:], start=1):
    if skip_next:
        skip_next = False
        continue
    if arg in ("--except", "-e"):
        skip_next = True
        continue
    if arg in ("--force", "-f"):
        continue
    if arg.startswith("-"):
        cli.ErrorArgPrint(f"Unknown flag: {arg}", Exit=False)
        sys.exit(1)
    dirlist.append(arg)

if not dirlist:
    cli.ErrorArgPrint("No directories given!", Exit=False)
    sys.exit(1)

# --except: delete everything in dir(s) except files matching the pattern
if cli.e:
    if cli.e.value is None:
        cli.ErrorArgPrint(
            "--except requires a pattern argument, e.g. --except *.py", Exit=False
        )
        sys.exit(1)

    pattern = cli.e.value
    for d in dirlist:
        path = Path(d)
        if not path.exists() or not path.is_dir():
            cli.ErrorArgPrint(f"Directory not found: {d}", Exit=False)
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

elif cli.force.value:
    for d in dirlist:
        try:
            shutil.rmtree(d)
            print(f"Force deleted: {d}")
        except FileNotFoundError:
            cli.ErrorArgPrint(f"Not found: {d}", Exit=False)
        except Exception as e:
            cli.ErrorArgPrint(f"Failed to delete {d}: {e}", Exit=False)

else:
    for d in dirlist:
        path = Path(d)
        if path.exists() and path.is_dir():
            shutil.rmtree(path)
            print(f"Deleted: {d}")
        else:
            cli.ErrorArgPrint(f"Directory not found: {d}", Exit=False)