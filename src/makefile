# weiss makefile

# General
EXE	   = weiss
SRC    = *.c fathom/tbprobe.c
OUT	   = ../bin/

# Defines
DEV    = -DDEV
PEXT   = -DUSE_PEXT -mbmi2

# Flags
STD    = -std=gnu11
LIBS   = -pthread
WARN   = -Wall -Wextra -Wshadow
OPTIM  = -O3 -march=native -flto

CFLAGS = $(STD) $(WARN) $(LIBS) $(OPTIM)
PFLAGS = $(STD) $(WARN) $(LIBS) -O0 -march=native -pg -p

# PGO
PGODIR = "../pgo"
PGOGEN = -fprofile-generate=$(PGODIR)
PGOUSE = -fprofile-use=$(PGODIR)


# For OpenBench
bench-basic:
	gcc $(CFLAGS) $(SRC)         -o $(EXE)

bench-pext:
	gcc $(CFLAGS) $(SRC) $(PEXT) -o $(EXE)

bench-pgo:
	gcc $(CFLAGS) $(SRC) $(PEXT) $(PGOGEN) -o "$(EXE)-temp.exe"
	$(EXE)-temp.exe bench 6
	gcc $(CFLAGS) $(SRC) $(PEXT) $(PGOUSE) -o $(EXE)
	rmdir /s /q "../pgo"
	del /f "$(EXE)-temp.exe"

# For normal use
dev:
	gcc $(CFLAGS) $(SRC) $(PEXT) $(DEV)    -o "$(EXE)-dev"

pext:
	gcc $(CFLAGS) $(SRC) $(PEXT)           -o "$(OUT)$(EXE)-pext"

pgo:
	gcc $(CFLAGS) $(SRC) $(PEXT) $(PGOGEN) -o "$(EXE)-temp.exe"
	$(EXE)-temp.exe bench 6
	gcc $(CFLAGS) $(SRC) $(PEXT) $(PGOUSE) -o "$(OUT)$(EXE)-pext-pgo.exe"
	rmdir /s /q "../pgo"
	del /f "$(EXE)-temp.exe"

gprof:
	gcc $(PFLAGS) $(SRC) $(PEXT)           -o "$(OUT)$(EXE)-prof"