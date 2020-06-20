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
SRC    = *.c pyrrhic/tbprobe.c noobprobe/noobprobe.c tuner/tuner.c
COMP   = g++

# Defines
POPCNT = -msse3 -mpopcnt
PEXT   = $(POPCNT) -DUSE_PEXT -mbmi2

# Flags
STD    = -std=c++17
LIBS   = -pthread -lm
WARN   = -Wall -Wextra -Wshadow

FLAGS  = $(STD) $(WARN) -O3 -flto
CFLAGS = $(FLAGS) -march=native
RFLAGS = $(FLAGS) -static

# PGO
PGODIR = "../pgo"
PGOGEN = -fprofile-generate=$(PGODIR)
PGOUSE = -fprofile-use=$(PGODIR)

# Use pext if supported and not a ryzen cpu
PROPS = $(shell echo | $(CC) -march=native -E -dM -)
CPU   = $(shell echo | $(CC) -march=native -Q --help=target | grep march)
ifneq ($(findstring __BMI2__, $(PROPS)),)
	ifeq ($(findstring znver, $(CPU)),)
		CFLAGS += -DUSE_PEXT
	endif
endif

# Try to detect windows environment by seeing
# whether the shell filters out " or not.
ifeq ($(shell echo "test"), "test")
	RUNBENCH = $(EXE) bench 12 > nul 2>&1
	CLEAN    = rmdir /s /q $(PGODIR)
else
	RUNBENCH = ./$(EXE) bench 12 > /dev/null 2>&1
	CLEAN    = $(RM) -rf $(PGODIR)
endif

# Link winsock32 on windows for NoobBook
ifeq ($(OS), Windows_NT)
	LIBS += -lwsock32
endif

# Compilations
BASIC   = $(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(EXE)
RELEASE = $(CC) $(RFLAGS) $(SRC) $(LIBS) -o $(EXE)

# Targets
pgo:
	$(BASIC) $(PGOGEN)
	$(RUNBENCH)
	$(BASIC) $(PGOUSE)
	$(CLEAN)

basic:
	$(BASIC)

dev:
	$(BASIC) -DDEV

tune:
	$(BASIC) -DTUNE -fopenmp

release:
	$(RELEASE)-nopopcnt.exe
	$(RELEASE)-popcnt.exe   $(POPCNT)
	$(RELEASE)-pext.exe     $(PEXT)
