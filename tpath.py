#!/usr/bin/env python3
"""
tpath - Run the Nth occurrence of a command found in PATH.

CLI usage:
    tpath <cmd> [cmd args...]           # run the 2nd instance (default)
    tpath -i <N> <cmd> [cmd args...]    # run the Nth instance
    tpath --help / -h                   # show help

API usage:
    from tpath import FindInstance, RunInstance, TPath

    # FindInstance: search PATH for a command
    fi = FindInstance("python3")
    fi.all()          # -> ['/usr/bin/python3', '/bin/python3']
    fi.nth(2)         # -> '/bin/python3'
    fi.count()        # -> 2
    fi.exists(2)      # -> True

    # RunInstance: run a specific occurrence
    ri = RunInstance(2)
    ri.run("python3", ["--version"])       # exec-replaces current process
    ri.spawn("python3", ["--version"])     # subprocess, returns CompletedProcess
    result = ri.capture("python3", ["-c", "print(1)"])
    result.stdout                          # -> '1\\n'

    # TPath: high-level one-liner facade
    TPath.run("python3", nth=2)
    TPath.spawn("python3", ["--version"], nth=2)
    TPath.find("python3")                  # -> FindInstance
    TPath.list("python3")                  # -> ['/usr/bin/python3', ...]
"""

from __future__ import annotations

import os
import sys
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

from arghandle.arghandle import Experimental, NoVarIndex


# ---------------------------------------------------------------------------
# Exceptions
# ---------------------------------------------------------------------------


class TPathError(Exception):
    """Base exception for all tpath errors."""


class CommandNotFound(TPathError):
    """Raised when the command is not found anywhere in PATH."""

    def __init__(self, cmd: str):
        self.cmd = cmd
        super().__init__(f"'{cmd}' not found in PATH")


class InstanceNotFound(TPathError):
    """Raised when the requested Nth instance doesn't exist."""

    def __init__(self, cmd: str, nth: int, total: int):
        self.cmd = cmd
        self.nth = nth
        self.total = total
        super().__init__(
            f"'{cmd}' has only {total} occurrence(s) in PATH (requested #{nth})"
        )


# ---------------------------------------------------------------------------
# FindInstance
# ---------------------------------------------------------------------------


@dataclass
class FindInstance:
    """
    Searches PATH for every executable matching `cmd`.

    Attributes:
        cmd     -- the bare command name (e.g. "python3")
        _paths  -- cached list of absolute paths, lazily populated

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

    # -- internal -----------------------------------------------------------

    @staticmethod
    def _extensions() -> list[str]:
        """
        On Windows, return every extension from PATHEXT (e.g. ['.EXE', '.CMD', …]).
        On Unix, return an empty list — no extension needed.
        """
        if os.name != "nt":
            return []
        pathext = os.environ.get("PATHEXT", ".EXE;.CMD;.BAT;.COM")
        return [e for e in pathext.split(";") if e]

    @staticmethod
    def _probe(directory: Path, cmd: str) -> list[Path]:
        """
        Yield candidate Paths for cmd inside directory.
        If cmd already has an extension (e.g. 'ls.exe'), only that is tried.
        Otherwise on Windows we append each PATHEXT extension; on Unix just the bare name.
        """
        p = Path(cmd)
        if p.suffix:
            # User was explicit about the extension
            return [directory / p]

        candidates = [directory / cmd]
        for ext in FindInstance._extensions():
            candidates.append(directory / (cmd + ext))
            candidates.append(directory / (cmd + ext.lower()))
        return candidates

    def _scan(self) -> None:
        """Walk PATH once, probe candidates with pathlib, cache hits."""
        if self._scanned:
            return
        seen: set[str] = set()
        for part in os.environ.get("PATH", "").split(os.pathsep):
            directory = Path(part)
            for candidate in self._probe(directory, self.cmd):
                # normcase gives case-insensitive dedup on Windows
                key = os.path.normcase(candidate)
                if (
                    key not in seen
                    and candidate.is_file()
                    and os.access(candidate, os.X_OK)
                ):
                    self._paths.append(candidate)
                    seen.add(key)
        self._scanned = True

    # -- public API ---------------------------------------------------------

    def all(self) -> list[Path]:
        """Return all occurrences in PATH order as Path objects."""
        self._scan()
        return list(self._paths)

    def count(self) -> int:
        """Return how many occurrences exist."""
        self._scan()
        return len(self._paths)

    def exists(self, nth: int = 1) -> bool:
        """Return True if the Nth occurrence exists (1-based)."""
        self._scan()
        return 1 <= nth <= len(self._paths)

    def nth(self, n: int) -> Path:
        """
        Return the Path of the Nth occurrence (1-based).

        Raises:
            CommandNotFound    if the command is absent entirely.
            InstanceNotFound   if fewer than N occurrences exist.
        """
        self._scan()
        if not self._paths:
            raise CommandNotFound(self.cmd)
        if n < 1 or n > len(self._paths):
            raise InstanceNotFound(self.cmd, n, len(self._paths))
        return self._paths[n - 1]

    def first(self) -> Path:
        """Shortcut for nth(1)."""
        return self.nth(1)

    def last(self) -> Path:
        """Shortcut for nth(count())."""
        self._scan()
        return self.nth(len(self._paths))

    def __repr__(self) -> str:
        self._scan()
        return f"FindInstance(cmd={self.cmd!r}, found={self._paths!r})"


# ---------------------------------------------------------------------------
# RunInstance
# ---------------------------------------------------------------------------


class RunInstance:
    """
    Runs a specific Nth occurrence of a command.

    Constructor:
        RunInstance(nth)   -- which occurrence to target (1-based, default 2)

    Methods:
        run(cmd, args)     -- os.execv into the target; replaces current process.
        spawn(cmd, args)   -- subprocess.run; returns CompletedProcess.
        capture(cmd, args) -- like spawn but captures stdout/stderr.
        resolve(cmd)       -- just return the resolved path without running.

    Example:
        ri = RunInstance(2)
        ri.spawn("python3", ["--version"])
    """

    def __init__(self, nth: int = 2):
        if nth < 1:
            raise ValueError(f"nth must be >= 1, got {nth}")
        self.nth = nth

    # -- internal -----------------------------------------------------------

    def _resolve(self, cmd: str) -> Path:
        """Resolve cmd to the Nth Path, raising on failure."""
        return FindInstance(cmd).nth(self.nth)

    # -- public API ---------------------------------------------------------

    def resolve(self, cmd: str) -> Path:
        """Return the Path of the Nth occurrence of cmd."""
        return self._resolve(cmd)

    def run(self, cmd: str, args: Optional[list[str]] = None) -> None:
        """
        Exec-replace the current process with the Nth occurrence of cmd.
        Does not return on success (Unix). Falls back to subprocess on Windows.
        """
        args = args or []
        target = self._resolve(cmd)
        try:
            os.execv(target, [str(target)] + args)
        except AttributeError:
            # Windows has no os.execv
            result = subprocess.run([target] + args)
            sys.exit(result.returncode)

    def spawn(
        self, cmd: str, args: Optional[list[str]] = None, **kwargs
    ) -> subprocess.CompletedProcess:
        """Run the Nth occurrence of cmd as a subprocess, returning CompletedProcess."""
        args = args or []
        return subprocess.run([self._resolve(cmd)] + args, **kwargs)

    def capture(
        self, cmd: str, args: Optional[list[str]] = None, **kwargs
    ) -> subprocess.CompletedProcess:
        """Like spawn() but captures stdout and stderr as text."""
        kwargs.setdefault("text", True)
        kwargs.setdefault("capture_output", True)
        return self.spawn(cmd, args or [], **kwargs)

    def __repr__(self) -> str:
        return f"RunInstance(nth={self.nth})"


# ---------------------------------------------------------------------------
# TPath  (high-level facade)
# ---------------------------------------------------------------------------


class TPath:
    """
    High-level, one-liner facade over FindInstance and RunInstance.

    All methods are class methods — no instantiation needed.

    Example:
        TPath.run("python3", nth=2)
        TPath.spawn("python3", ["--version"], nth=1)
        TPath.list("python3")
        TPath.find("python3").nth(2)
    """

    @classmethod
    def find(cls, cmd: str) -> FindInstance:
        """Return a FindInstance for cmd (lazy; doesn't scan until used)."""
        return FindInstance(cmd)

    @classmethod
    def list(cls, cmd: str) -> list[Path]:
        """Return all occurrences of cmd in PATH as Path objects."""
        return FindInstance(cmd).all()

    @classmethod
    def resolve(cls, cmd: str, nth: int = 2) -> Path:
        """Return the Path of the Nth occurrence of cmd."""
        return RunInstance(nth).resolve(cmd)

    @classmethod
    def run(cls, cmd: str, args: Optional[list[str]] = None, nth: int = 2) -> None:
        """Exec-replace the current process with the Nth occurrence of cmd."""
        RunInstance(nth).run(cmd, args)

    @classmethod
    def spawn(
        cls, cmd: str, args: Optional[list[str]] = None, nth: int = 2, **kwargs
    ) -> subprocess.CompletedProcess:
        """Run the Nth occurrence of cmd as a subprocess."""
        return RunInstance(nth).spawn(cmd, args, **kwargs)

    @classmethod
    def capture(
        cls, cmd: str, args: Optional[list[str]] = None, nth: int = 2, **kwargs
    ) -> subprocess.CompletedProcess:
        """Run the Nth occurrence of cmd and capture its output."""
        return RunInstance(nth).capture(cmd, args, **kwargs)


# ---------------------------------------------------------------------------
# CLI entry point (reuses the API above)
# ---------------------------------------------------------------------------


def _parse_cli() -> tuple[int, str, list[str]]:
    """Parse CLI args with arghandle's Experimental class."""
    if len(sys.argv) < 2 or sys.argv[1] in ("--help", "-h"):
        print("""
        TPath, 2nd Instance in PATH. Usage:
        --help/-h : Print this help screen
        --version/-v : Print version
        -ts : Tab To Space;Usage: -ts <FILENAME>
        -st : Space To Tab;Usage: -st <FILENAME>""")

    exp = Experimental()

    if sys.argv[1] in ("-i", "--index"):
        exp.RegisterArg(
            ["-i", "--index"], VarIndex=2, HelpMsg="Which occurrence to run (1-based)"
        )

        if isinstance(exp.i, NoVarIndex):
            sys.exit("error: -i requires an integer argument\n")
        try:
            nth = int(exp.i)
        except ValueError:
            sys.exit(f"error: '{exp.i}' is not a valid integer for -i\n")
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

