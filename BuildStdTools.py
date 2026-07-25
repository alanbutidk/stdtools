# stdtools builder.
# Copyright (c) 2026 Alan. All Rights Reserved.

import sys
import subprocess as s
from pathlib import Path

COMPILER_FOR_C = "cc"
COMPILER_FOR_PY = "PyCC"

CFILES = [
    Path("C/timestamp.c").resolve(),
    Path("Clones/fresh.c").resolve(),
    Path("Clones/lst.c").resolve(),
]
PY = Path("Py/").resolve()
FILES = []

for path in PY.rglob("*"):
    if path.is_file():
        FILES.append(str(path))


def Build() -> None | None:
    s.run(["gcc", "-o"])


if len(sys.argv) < 2:
    print("Building all programs.")
