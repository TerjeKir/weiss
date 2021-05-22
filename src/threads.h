/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2020  Terje Kirstihagen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <setjmp.h>

#include "board.h"
#include "evaluate.h"
#include "types.h"


#define SS_OFFSET 10


typedef struct {
    int eval;
    Depth ply;
    Move excluded;
    Move killers[2];
    PV pv;
} Stack;

typedef struct Thread {

    int16_t history[COLOR_NB][64][64];
    Stack ss[128];
    jmp_buf jumpBuffer;
    uint64_t tbhits;
    Move bestMove;
    int score;
    Depth depth;
    bool doPruning;

    // Anything below here is not zeroed out between searches
    Position pos;
    PawnCache pawnCache;

    int index;
    int count;

} Thread;


extern Thread *threads;


Thread *InitThreads(int threadCount);
uint64_t TotalNodes();
uint64_t TotalTBHits();
void PrepareSearch(Position *pos);
void StartMainThread(void *(*func)(void *), Position *pos);
void StartHelpers(void *(*func)(void *));
void WaitForHelpers();
void ResetThreads();
void RunWithAllThreads(void *(*func)(void *));
void Wait(volatile bool *condition);
void Wake();
