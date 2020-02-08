#																		 \
  Weiss is a UCI compliant chess engine. 								 \
  Copyright (C) 2020  Terje Kirstihagen 								 \
																		 \
  This program is free software: you can redistribute it and/or modify	 \
  it under the terms of the GNU General Public License as published by	 \
  the Free Software Foundation, either version 3 of the License, or		 \
  (at your option) any later version.									 \
																		 \
  This program is distributed in the hope that it will be useful,		 \
  but WITHOUT ANY WARRANTY; without even the implied warranty of		 \
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			 \
  GNU General Public License for more details.							 \
																		 \
  You should have received a copy of the GNU General Public License		 \
  along with this program.  If not, see <https://www.gnu.org/licenses/>. \
#																		 \

# General
EXE    = weiss
SRC    = *.c fathom/tbprobe.c
OUT    = ../bin/
COMP   = gcc

# Defines
DEV    = -DDEV
POPCNT = -msse3 -mpopcnt
PEXT   = $(POPCNT) -DUSE_PEXT -mbmi2

# Flags
STD    = -std=gnu11
LIBS   = -pthread -lm
WARN   = -Wall -Wextra -Wshadow

CFLAGS = $(STD) $(WARN) -O3 -flto -march=native
RFLAGS = $(STD) $(WARN) -O3 -flto -static
PFLAGS = $(STD) $(WARN) -O0 -march=native -pg -p

# PGO
PGODIR = "../pgo"
PGOGEN = -fprofile-generate=$(PGODIR)
PGOUSE = -fprofile-use=$(PGODIR)

# Try to detect windows environment by seeing
# whether the shell filters out the "s or not.
ifeq ($(shell echo "check_quotes"),"check_quotes")
	RUNBENCH = $(EXE) bench 12 > nul 2>&1
	CLEAN    = rmdir /s /q $(PGODIR)
else
	RUNBENCH = ./$(EXE) bench 12 > /dev/null 2>&1
	CLEAN    = $(RM) -rf $(PGODIR)
endif

# Compilations
BASE    = $(COMP) $(CFLAGS) $(SRC) $(LIBS)
RELEASE = $(COMP) $(RFLAGS) $(SRC) $(LIBS) -o $(OUT)$(EXE)
BENCH   = $(BASE) -o $(EXE)
BIN     = $(BASE) -o $(OUT)$(EXE)

# For OpenBench
bench-basic:
	$(BENCH)

bench-pext:
	$(BENCH) $(PEXT)

bench-pgo:
	$(BENCH) $(PEXT) $(PGOGEN)
	$(RUNBENCH)
	$(BENCH) $(PEXT) $(PGOUSE)
	$(CLEAN)

# For normal use
pext:
	$(BIN)-pext $(PEXT)

pgo:
	$(BENCH) $(PEXT) $(PGOGEN)
	$(RUNBENCH)
	$(BIN)-pext-pgo.exe $(PEXT) $(PGOUSE)
	$(CLEAN)

release:
	$(RELEASE)-nopopcnt.exe
	$(RELEASE)-popcnt.exe   $(POPCNT)
	$(RELEASE)-pext.exe     $(PEXT)

dev:
	$(BENCH)-dev $(PEXT) $(DEV)

gprof:
	$(COMP) $(PFLAGS) $(SRC) $(LIBS) $(PEXT) -o $(OUT)$(EXE)-prof