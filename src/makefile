# weiss makefile

#general
NAME	= weiss
VER		= 0.3
SRC     = *.c fathom/tbprobe.c
OUT		= ../bin/

#generic flags
STD		= -std=gnu11
WFLAGS 	= -Wall -Wextra -Wshadow
OPTIM	= -O3 -march=native

#pgo
PGODIR  = "../prof"
PGOMAKE	= -fprofile-generate=$(PGODIR)
PGOUSE  = -fprofile-use=$(PGODIR)

default: 	#for quick tests
	gcc $(STD) $(OPTIM) $(SRC) -o "$(NAME) $(VER).exe"

pgoc:		#for pgo-compile
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) $(PGOUSE) -o "$(OUT)$(NAME) $(VER) pgoc.exe"

pgom: 		#for generating profile data -> pgo
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) $(PGOMAKE) -o "$(OUT)$(NAME) $(VER) pgom.exe"

basic: 		#for basic compile
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) -o "$(OUT)$(NAME) $(VER).exe"

prof:		#for gprof profiling
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) -pg -o "$(OUT)$(NAME) $(VER) prof.exe"