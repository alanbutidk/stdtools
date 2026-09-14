from typing import Any
from arghandle import ArgHandle, ArgTypeError

import string
import secrets
import pyperclip

# pyright: reportAttributeAccessIssue=false


def WriteToFile(CONTENT: Any, FILE: str) -> bool:
    with open(FILE, "w") as f:
        f.write(CONTENT)
    return True


def PassGen(k: int) -> str:
    Alphabets = string.ascii_letters + string.digits + string.punctuation
    Letters = string.ascii_letters
    FirstChar = secrets.choice(Letters)

    PassPart2 = "".join(secrets.choice(Alphabets) for _ in range(k - 1))
    return FirstChar + PassPart2


def ResolvePassLen(cli: ArgHandle, default: int = 8) -> int:
    if not cli.pass_len:
        return default

    raw = cli.pass_len.value
    if raw is None:
        cli.ErrorArgPrint(
            "No argument given after --pass-len/-pl", Exit=False, Warn=True
        )
        return default

    try:
        return int(raw)
    except ValueError:
        cli.ErrorArgPrint(f"Invalid password length: {raw!r}", Exit=False, Warn=True)
        return default


if __name__ == "__main__":
    cli = ArgHandle("passgen", "v1.0.1")
    cli.PrintOnNoArgs("No arguments given, use --help/-h for usage.")
    try:
        cli.RegisterArg(
            ["--write-to-file", "-wtf"],
            Type=str,
            Type_ExitOnError=True,
            HelpMsg="Write to file or wtf.",
        )
        cli.RegisterArg(
            ["--pass-len", "-pl"],
            Type=int,
            Type_ExitOnError=True,
            HelpMsg="Password length (def: 8)",
        )
    except (ArgTypeError, ValueError):
        print(
            "\033[31mAn incorrect value was provided. You might want to check the arguments.\033[0m"
        )
        raise SystemExit  # noqa
    cli.HandleBasic()

    if cli.write_to_file:
        passlen = ResolvePassLen(cli)
        passw = PassGen(passlen)

        filename = cli.write_to_file.value
        if filename is None:
            cli.ErrorArgPrint("No filename given after --write-to-file/-wtf")

        WriteToFile(passw, filename)  # pyright: ignore[reportArgumentType]
        pyperclip.copy(str(passw))
        print(f"Written to file: {filename} AND password copied to clipboard!")
        raise SystemExit

    if cli.pass_len:
        passlen = ResolvePassLen(cli)
        passw = PassGen(passlen)
        pyperclip.copy(str(passw))
        print("Password copied to clipboard!")
