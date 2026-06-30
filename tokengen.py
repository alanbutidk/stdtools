from typing import Any
from arghandle import ArgHandle, NotFoundInArgs
import datetime
import hashlib
import os


def GenToken(
	FileName: str = None, WriteToFile: bool = False, AppName: str = ""
) -> tuple | Any:
	Timestamp = datetime.datetime.now(datetime.UTC).strftime("%Y%m%d%H%M%S")
	RandomBytes = os.urandom(64)
	Hash256 = hashlib.sha256(RandomBytes).hexdigest()
	if AppName == "":
		if WriteToFile and FileName is not None:
			with open(FileName, "a") as f:
				f.write(f"{Timestamp}{Hash256}")
				return f"Completed write to file: {FileName}"
		elif WriteToFile and FileName is None:
			raise SystemExit("Filename not given.")
		else:
			return f"{Timestamp}{Hash256}"
	elif not AppName == "":
		if WriteToFile and FileName is not None:
			with open(FileName, "a") as fn:
				fn.write(f"{AppName}{Timestamp}{Hash256}")
				return f"Completed write to file: {FileName}"
		elif WriteToFile and not FileName is not None:
			raise SystemExit("Filename not given.")
		else:
			return f"{AppName}{Timestamp}{Hash256}"
	else:
		print("Could not generate token.")


if __name__ == "__main__":
	cli = ArgHandle()
	cli.ProgramName("TokenGen")
	cli.RegisterArg(
		["--version", "-v"],
		HelpMsg="Print version & exit",
	)
	cli.RegisterArg(
		["--writetofile", "-wtf"],
		StrictIndex=2,
		StrictIndex_ExitOnError=True,
		HelpMsg="Yes, it is -wtf. Write-To-File.",
	)
	cli.RegisterArg(
		["-a", "--appname"],
		HelpMsg="AppName for token. If none then only token without name.",
	)
	cli.HandleHelp()
	if cli.IsArgInActualArgs("--version") or cli.IsArgInActualArgs("-v"):
		print("""
TokenGen - stdtools v1.0.0
Copyright (C) 2026 Alan
License GPLv3+: GNU GPL version 3 or later <https://gnu.org>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
		""")
		raise SystemExit
	AppName = cli.NextAfter("-a") or cli.NextAfter("--appname")
	if isinstance(AppName, NotFoundInArgs):
		print("No appname provided.")
		AppName = ""

	if cli.IsArgInActualArgs("-wtf") or cli.IsArgInActualArgs("--writetofile"):
		filename = cli.NextAfter("-wtf") or cli.NextAfter("--writetofile")
		GenToken(AppName=AppName, FileName=filename, WriteToFile=True)
	else:
		print(GenToken(AppName=AppName))
