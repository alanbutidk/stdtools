from typing import Any
from arghandle import ArgHandle, NotFoundInArgs
import string
import secrets


def WriteToFile(CONTENT: Any, FILE: str) -> Any | bool:
    with open(FILE, "w") as f:
        f.write(CONTENT)
    return True


def PassGen(k: int) -> int | str:
    Alphabets = string.ascii_letters + string.digits + string.punctuation
    Letters = string.ascii_letters
    FirstChar = secrets.choice(Letters)

    PassPart2 = "".join(secrets.choice(Alphabets) for _ in range(k - 1))
    return FirstChar + PassPart2


cli = ArgHandle("passgen", "v1.0.0")
cli.PrintOnNoArgs("No arguments given, use --help/-h for usage.")
cli.RegisterArg(["--version", "-v"], HelpMsg="Print version & exit")
cli.RegisterArg(["--write-to-file", "-wtf"], HelpMsg="Write to file or wtf.")
cli.RegisterArg(["--pass-len", "-pl"], HelpMsg="Password length (def: 8)")
cli.HandleBasic()
