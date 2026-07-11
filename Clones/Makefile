FILE := lst.c
CC := gcc
APP := lst
EXE := $(if $(filter Windows_NT,$(OS)),.exe,)
ifeq ($(OS), Windows_NT)
    MFLAG = -mconsole
else
    MFLAG = 
endif
build:
    $(CC) -o $(APP)$(EXE) $(FILE) $(MFLAG)

