SRC     = *.c fathom/tbprobe.c
OUT		= ../bin/
EXE		= weiss
VER		= 0.3

STD		= -std=gnu11
WFLAGS 	= -Wall -Wextra -Wshadow
OPTIM	= -O3

default:
	gcc $(STD) $(OPTIM) $(SRC) -o "$(EXE) $(VER).exe"

bin:
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) -o "$(OUT)$(EXE) $(VER).exe"

prof:
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) -o "$(OUT)$(EXE) $(VER)prof.exe" -pg