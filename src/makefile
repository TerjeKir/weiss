SRC     = *.c fathom/tbprobe.c
OUT		= ../bin/
EXE		= weiss
VER		= 0.3

STD		= -std=gnu11
WFLAGS 	= -Wall -Wextra -Wshadow
OPTIM	= -O2

default:
	gcc $(STD) $(SRC) -o "$(EXE) $(VER).exe" $(OPTIM)

bin:
	gcc $(STD) $(WFLAGS) $(SRC) -o "$(OUT)$(EXE) $(VER).exe" $(OPTIM)

prof:
	gcc $(STD) $(WFLAGS) $(SRC) -o "$(OUT)$(EXE) $(VER)prof.exe" $(OPTIM) -pg