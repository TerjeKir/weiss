SRC     = *.c fathom/tbprobe.c
OUT		= ../bin/
EXE		= weiss
VER		= 0.1.1

STD		= -std=gnu11
WFLAGS 	= -Wall -Wextra -Wshadow

default:
	gcc $(STD) $(SRC) -o "$(EXE)$(VER).exe"

bin:
	gcc $(STD) $(WFLAGS) $(SRC) -o "$(OUT)$(EXE)$(VER).exe"