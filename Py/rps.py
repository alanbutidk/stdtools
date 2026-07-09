import random
from typing import Any


def GetCompChoice() -> None | str:
    Choosable = ["r", "p", "s"]
    Comp = random.choice(Choosable)
    return Comp


def Winner(Player: str, Comp: str) -> str | Any:
    Choosable = ["r", "p", "s"]
    print(f"\nComputer chose: {Comp}\n")
    if Player == Comp:
        print("\nIts a tie!")
    elif (
        Player == "r"
        and Comp == "s"
        or Player == "p"
        and Comp == "r"
        or Player == "s"
        and Comp == "p"
    ):
        print("\nYou win this round!\n")
    else:
        print("\nComputer wins this round!")


try:
    while True:
        print("\nEnter 'r' for rock, 'p' for paper and 's' for scissors")
        Choice = input("").lower()
        if Choice == "q" or Choice == "exit" or Choice == "quit":
            print("Thanks for playing!")
            break
        if Choice == "":
            continue
        if Choice != "r" and Choice != "p" and Choice != "s":
            print("\nInvalid input. TRY AGAIN\n")
            continue
        CompChoice = GetCompChoice()
        Winner(Choice, CompChoice)
except KeyboardInterrupt:
    print("\nExitted!")
    raise SystemExit
