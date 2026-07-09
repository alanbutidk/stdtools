CC := gcc
CFLAGS := -Wall -O0

ifeq ($(OS),Windows_NT)
	PYNAME := python
	EXE_EXT := .exe
	NUITKA_OUT := StubExec.exe
	NUITKA_FLAGS := --onefile --output-dir=. --output-filename=StubExec.exe \
		--no-deployment-flag=self-execution \
		--remove-output \
		--nofollow-import-to=tkinter \
		--nofollow-import-to=unittest \
		--nofollow-import-to=email \
		--nofollow-import-to=html \
		--nofollow-import-to=http \
		--nofollow-import-to=xml \
		--nofollow-import-to=pydoc \
		--nofollow-import-to=doctest \
		--nofollow-import-to=difflib \
		--nofollow-import-to=multiprocessing \
		--nofollow-import-to=logging \
		--nofollow-import-to=asyncio \
		--nofollow-import-to=concurrent \
		--nofollow-import-to=urllib \
		--nofollow-import-to=xmlrpc \
		--nofollow-import-to=sqlite3
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		PYNAME := python3
		EXE_EXT :=
		NUITKA_OUT := StubExec
		NUITKA_FLAGS := --onefile --output-dir=. --output-filename=StubExec \
			--no-deployment-flag=self-execution \
			--remove-output \
			--nofollow-import-to=tkinter \
			--nofollow-import-to=unittest \
			--nofollow-import-to=email \
			--nofollow-import-to=html \
			--nofollow-import-to=http \
			--nofollow-import-to=xml \
			--nofollow-import-to=pydoc \
			--nofollow-import-to=doctest \
			--nofollow-import-to=difflib \
			--nofollow-import-to=multiprocessing \
			--nofollow-import-to=logging \
			--nofollow-import-to=asyncio \
			--nofollow-import-to=concurrent \
			--nofollow-import-to=urllib \
			--nofollow-import-to=xmlrpc \
			--nofollow-import-to=sqlite3
	endif
endif

TARGET := Stub$(EXE_EXT)

all: $(TARGET)

$(TARGET): Stubs/StubSrc.c
	$(CC) $(CFLAGS) -o Stubs/$(TARGET) Stubs/StubSrc.c

nuitka: StubExec.py
	$(PYNAME) -m nuitka $(NUITKA_FLAGS) StubExec.py

clean:
	rm -f Stubs/$(TARGET)
	rm -f $(NUITKA_OUT)
	rm -rf StubExec.build StubExec.dist StubExec.onefile-build
