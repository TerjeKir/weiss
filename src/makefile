# weiss makefile

#general
NAME	= weiss
VER		= 0.3
EXE		= $(NAME) $(VER)
SRC     = *.c fathom/tbprobe.c
OUT		= ../bin/

#generic flags
STD		= -std=gnu11
WFLAGS 	= -Wall -Wextra -Wshadow
OPTIM	= -O3 -march=native -flto -funroll-loops
POPTIM  = -O0 -march=native

#pgo
PGODIR  = "../pgo"
PGOMAKE	= -fprofile-generate=$(PGODIR)
PGOUSE  = -fprofile-use=$(PGODIR)

default: 	#for quick tests
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) -DCLI      -o       "$(EXE) dev.exe"

basic: 		#for basic compile
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC)            -o "$(OUT)$(EXE) basic.exe"

pgo:		#for pgo-compile
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) $(PGOUSE)  -o "$(OUT)$(EXE).exe"

pgomake: 	#for generating profile data -> pgo
	gcc $(STD) $(OPTIM) $(WFLAGS) $(SRC) $(PGOMAKE) -o "$(OUT)$(EXE) pgo.exe"

prof:		#for gprof profiling
	gcc $(STD) $(POPTIM) $(WFLAGS) $(SRC) -pg       -o "$(OUT)$(EXE) prof.exe"