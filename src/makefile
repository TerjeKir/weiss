SRC     = *.c fathom/tbprobe.c
OUT		= ../bin/
EXE		= weiss
VER		= 0.3

STD		= -std=gnu11
WFLAGS 	= -Wall -Wextra -Wshadow
OPTIM	= -O2 -march=native

PGODIR  = "../prof"
PGOMAKE	= -fprofile-generate=$(PGODIR)
PGOUSE  = -fprofile-use=$(PGODIR)

default:
	gcc $(STD) $(OPTIM) $(SRC) -o "$(EXE) $(VER).exe"

pgo:
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) $(PGOMAKE) -o "$(OUT)$(EXE) $(VER) pgo.exe"

bin:
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) $(PGOUSE) -o "$(OUT)$(EXE) $(VER).exe"

prof:
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) $(PGOUSE) -pg -o "$(OUT)$(EXE) $(VER) prof.exe"