#																		 \
  Weiss is a UCI compliant chess engine. 								 \
  Copyright (C) 2023 Terje Kirstihagen 								 \
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
SRC    = *.c pyrrhic/tbprobe.c tuner/*.c query/*.c noobprobe/*.c onlinesyzygy/*.c
CC     = gcc

# Defines
POPCNT = -msse3 -mpopcnt
PEXT   = $(POPCNT) -DUSE_PEXT -mbmi2

# Flags
STD    = -std=gnu11
LIBS   = -pthread -lm
WARN   = -Wall -Wextra -Wshadow -Werror -Wmissing-declarations

FLAGS  = $(STD) $(WARN) -O3 -flto
CFLAGS = $(FLAGS) -march=native
RFLAGS = $(FLAGS) -static

# Use pext if supported and not a ryzen 1/2 cpu
PROPS = $(shell echo | $(CC) -march=native -E -dM -)
ifneq ($(findstring __BMI2__, $(PROPS)),)
	ifeq ($(findstring __znver1, $(PROPS)),)
		ifeq ($(findstring __znver2, $(PROPS)),)
			CFLAGS += -DUSE_PEXT
		endif
	endif
endif

# PGO
ifneq ($(findstring gcc, $(CC)),)
	PGO_DIR = "pgo"
	PGO_GEN = -fprofile-generate=$(PGO_DIR)
	PGO_USE = -fprofile-use=$(PGO_DIR)
else ifneq ($(findstring clang, $(CC)),)
	PGO_MERGE = llvm-profdata merge -output=weiss.profdata *.profraw
	PGO_GEN = -fprofile-instr-generate
	PGO_USE = -fprofile-instr-use=weiss.profdata
endif

# Try to detect windows environment by seeing
# whether the shell filters out " or not.
ifeq ($(shell echo "test"), "test")
	PGO_BENCH = $(EXE) bench 12 > nul 2>&1
	PGO_CLEAN = rmdir /s /q $(PGO_DIR)
else
	PGO_BENCH = ./$(EXE) bench 12 > /dev/null 2>&1
	PGO_CLEAN = $(RM) -rf $(PGO_DIR)
endif

# Link winsock32 on windows for NoobBook
ifeq ($(OS), Windows_NT)
	LIBS += -lwsock32
endif

# Compilations
BASIC   = $(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(EXE)
RELEASE = $(CC) $(RFLAGS) $(SRC) $(LIBS) -o $(EXE)

# Targets
pgo: clean
	$(BASIC) $(PGO_GEN)
	$(PGO_BENCH)
	$(PGO_MERGE)
	$(BASIC) $(PGO_USE)
	@$(PGO_CLEAN)

basic: clean
	$(BASIC)

dev: clean
	$(BASIC) -DDEV

tune: clean
	$(BASIC) -DTUNE -fopenmp

release: clean
	$(RELEASE)-nopopcnt.exe
	$(RELEASE)-popcnt.exe   $(POPCNT)
	$(RELEASE)-pext.exe     $(PEXT)

clean:
	@$(RM) -f $(EXE)
	@$(PGO_CLEAN)
