#!/usr/bin/env python3
"""
tpath - Run the Nth occurrence of a command found in PATH.

CLI usage:
    tpath <cmd> [cmd args...]           # run the 2nd instance (default)
    tpath -i <N> <cmd> [cmd args...]    # run the Nth instance
    tpath --help / -h                   # show help

API usage:
    from tpath import FindInstance, RunInstance, TPath

    fi = FindInstance("python3")
    fi.all()
    fi.nth(2)
    fi.count()
    fi.exists(2)

    ri = RunInstance(2)
    ri.run("python3", ["--version"])
    ri.spawn("python3", ["--version"])
    result = ri.capture("python3", ["-c", "print(1)"])
    result.stdout

    TPath.run("python3", nth=2)
    TPath.spawn("python3", ["--version"], nth=2)
    TPath.find("python3")
    TPath.list("python3")
"""

from __future__ import annotations

import os
import sys
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

from arghandle import ArgHandle, NoVarIndex


class TPathError(Exception):
    """Base exception for all tpath errors."""


class CommandNotFound(TPathError):
    def __init__(self, cmd: str):
        self.cmd = cmd
        super().__init__(f"'{cmd}' not found in PATH")


class InstanceNotFound(TPathError):
    def __init__(self, cmd: str, nth: int, total: int):
        self.cmd = cmd
        self.nth = nth
        self.total = total
        super().__init__(
            f"'{cmd}' has only {total} occurrence(s) in PATH (requested #{nth})"
        )


@dataclass
class FindInstance:
    """
    Searches PATH for every executable matching `cmd`.

    Example:
        fi = FindInstance("python3")
        fi.all()      # ['/usr/bin/python3', '/bin/python3']
        fi.nth(1)     # '/usr/bin/python3'
        fi.count()    # 2
        fi.exists(2)  # True
    """

    cmd: str
    _paths: list[Path] = field(default_factory=list, init=False, repr=False)
    _scanned: bool = field(default=False, init=False, repr=False)

    @staticmethod
    def _extensions() -> list[str]:
        if os.name != "nt":
            return []
        pathext = os.environ.get("PATHEXT", ".EXE;.CMD;.BAT;.COM")
        return [e for e in pathext.split(";") if e]

    @staticmethod
    def _probe(directory: Path, cmd: str) -> list[Path]:
        p = Path(cmd)
        if p.suffix:
            return [directory / p]

        candidates = [directory / cmd]
        for ext in FindInstance._extensions():
            candidates.append(directory / (cmd + ext))
            candidates.append(directory / (cmd + ext.lower()))
        return candidates

    def _scan(self) -> None:
        if self._scanned:
            return
        seen: set[str] = set()
        for part in os.environ.get("PATH", "").split(os.pathsep):
            directory = Path(part)
            for candidate in self._probe(directory, self.cmd):
                key = os.path.normcase(candidate)
                if (
                    key not in seen
                    and candidate.is_file()
                    and os.access(candidate, os.X_OK)
                ):
                    self._paths.append(candidate)
                    seen.add(key)
        self._scanned = True

    def all(self) -> list[Path]:
        self._scan()
        return list(self._paths)

    def count(self) -> int:
        self._scan()
        return len(self._paths)

    def exists(self, nth: int = 1) -> bool:
        self._scan()
        return 1 <= nth <= len(self._paths)

    def nth(self, n: int) -> Path:
        self._scan()
        if not self._paths:
            raise CommandNotFound(self.cmd)
        if n < 1 or n > len(self._paths):
            raise InstanceNotFound(self.cmd, n, len(self._paths))
        return self._paths[n - 1]

    def first(self) -> Path:
        return self.nth(1)

    def last(self) -> Path:
        self._scan()
        return self.nth(len(self._paths))

    def __repr__(self) -> str:
        self._scan()
        return f"FindInstance(cmd={self.cmd!r}, found={self._paths!r})"


class RunInstance:
    """
    Runs a specific Nth occurrence of a command.

    Example:
        ri = RunInstance(2)
        ri.spawn("python3", ["--version"])
    """

    def __init__(self, nth: int = 2):
        if nth < 1:
            raise ValueError(f"nth must be >= 1, got {nth}")
        self.nth = nth

    def _resolve(self, cmd: str) -> Path:
        return FindInstance(cmd).nth(self.nth)

    def resolve(self, cmd: str) -> Path:
        return self._resolve(cmd)

    def run(self, cmd: str, args: Optional[list[str]] = None) -> None:
        """
        Run the Nth occurrence of cmd, replacing the current process on Unix.
        Windows always goes through subprocess, since os.execv there doesn't
        give real process-replacement semantics and was causing output to
        interleave/garble against tpath's own prints.
        """
        args = args or []
        target = self._resolve(cmd)
        sys.stdout.flush()
        sys.stderr.flush()
        if os.name == "nt" or not hasattr(os, "execv"):
            result = subprocess.run([target] + args)
            sys.exit(result.returncode)
        os.execv(target, [str(target)] + args)

    def spawn(
        self, cmd: str, args: Optional[list[str]] = None, **kwargs
    ) -> subprocess.CompletedProcess:
        args = args or []
        return subprocess.run([self._resolve(cmd)] + args, **kwargs)

    def capture(
        self, cmd: str, args: Optional[list[str]] = None, **kwargs
    ) -> subprocess.CompletedProcess:
        kwargs.setdefault("text", True)
        kwargs.setdefault("capture_output", True)
        return self.spawn(cmd, args or [], **kwargs)

    def __repr__(self) -> str:
        return f"RunInstance(nth={self.nth})"


class TPath:
    """
    High-level facade over FindInstance and RunInstance.

    Example:
        TPath.run("python3", nth=2)
        TPath.spawn("python3", ["--version"], nth=1)
        TPath.list("python3")
        TPath.find("python3").nth(2)
    """

    @classmethod
    def find(cls, cmd: str) -> FindInstance:
        return FindInstance(cmd)

    @classmethod
    def list(cls, cmd: str) -> list[Path]:
        return FindInstance(cmd).all()

    @classmethod
    def resolve(cls, cmd: str, nth: int = 2) -> Path:
        return RunInstance(nth).resolve(cmd)

    @classmethod
    def run(cls, cmd: str, args: Optional[list[str]] = None, nth: int = 2) -> None:
        RunInstance(nth).run(cmd, args)

    @classmethod
    def spawn(
        cls, cmd: str, args: Optional[list[str]] = None, nth: int = 2, **kwargs
    ) -> subprocess.CompletedProcess:
        return RunInstance(nth).spawn(cmd, args, **kwargs)

    @classmethod
    def capture(
        cls, cmd: str, args: Optional[list[str]] = None, nth: int = 2, **kwargs
    ) -> subprocess.CompletedProcess:
        return RunInstance(nth).capture(cmd, args, **kwargs)


def _parse_cli() -> tuple[int, str, list[str]]:
    cli = ArgHandle()
    cli.ProgramName("tpath")
    cli.RegisterArg(
        ["--version", "-v"],
        HelpMsg="Print version",
    )
    cli.RegisterArg(
        ["-i", "--index"],
        StrictIndex=1,
        StrictIndex_ExitOnError=True,
        VarIndex=2,
        HelpMsg="Which occurrence to run (1-based). Usage: -i <N> <cmd> [args...]",
    )
    cli.PrintOnNoArgs("No command given! Use -h or --help for usage.", Exit=True)
    cli.HandleHelp()

    if cli.IsArgInActualArgs("--version") or cli.IsArgInActualArgs("-v"):
        print("""
TPath - stdTools 1.0.0
Copyright (C) 2026 Alan
License GPLv3+: GNU GPL version 3 or later <http://gnu.org>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

        """)
        raise SystemExit

    if sys.argv[1] in ("-i", "--index"):
        if isinstance(cli.i, NoVarIndex):
            sys.exit("error: -i requires an integer argument\n")
        try:
            nth = int(cli.i)
        except ValueError:
            sys.exit(f"error: '{cli.i}' is not a valid integer for -i\n")
        if nth < 1:
            sys.exit("error: -i must be >= 1\n")
        if len(sys.argv) < 4:
            sys.exit("error: no command specified after -i <N>\n")

        cmd, cmd_args = sys.argv[3], sys.argv[4:]
    else:
        nth, cmd, cmd_args = 2, sys.argv[1], sys.argv[2:]

    return nth, cmd, cmd_args


def main():
    nth, cmd, cmd_args = _parse_cli()
    try:
        print(
            f"[tpath] running occurrence #{nth} of '{cmd}': {TPath.resolve(cmd, nth)}",
            file=sys.stderr,
        )
        TPath.run(cmd, cmd_args, nth=nth)
    except (CommandNotFound, InstanceNotFound) as exc:
        sys.exit(f"error: {exc}\n")


if __name__ == "__main__":
    main()