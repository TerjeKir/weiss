# weiss makefile

# General
EXE	   = weiss
SRC    = *.c fathom/tbprobe.c
OUT	   = ../bin/

# Defines
CLI    = -DCLI
PEXT   = -DUSE_PEXT -mbmi2

# Flags
STD    = -std=gnu11
LIBS   = -pthread
WARN   = -Wall -Wextra -Wshadow
OPTIM  = -O3 -march=native -flto -funroll-loops

CFLAGS = $(STD) $(WARN) $(LIBS) $(OPTIM)
PFLAGS = $(STD) $(WARN) $(LIBS) -O0 -march=native -pg -p

# PGO
PGODIR = "../pgo"
PGOGEN = -fprofile-generate=$(PGODIR)
PGOUSE = -fprofile-use=$(PGODIR)


basic: # for openBench
	gcc $(CFLAGS) $(SRC) -o $(EXE)

dev:   # for tests not included in the others
	gcc $(CFLAGS) $(SRC) $(PEXT) $(CLI)    -o "$(EXE)-dev.exe"

pext:  # for basic compile
	gcc $(CFLAGS) $(SRC) $(PEXT)           -o "$(OUT)$(EXE)-pext.exe"

gprof: # for gprof profiling
	gcc $(PFLAGS) $(SRC) $(PEXT)           -o "$(OUT)$(EXE)-prof.exe"

pgo:   # profile guided optimization, for windows
	gcc $(CFLAGS) $(SRC) $(PEXT) $(PGOGEN) -o "$(EXE)-temp.exe"
	$(EXE)-temp.exe bench 6
	gcc $(CFLAGS) $(SRC) $(PEXT) $(PGOUSE) -o "$(OUT)$(EXE)-pext-pgo.exe"
	rmdir /s /q "../pgo"
	del /f "$(EXE)-temp.exe"