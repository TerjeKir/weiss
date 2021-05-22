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

    uint64_t tbhits;

    int score;
    Depth depth;
    bool doPruning;

    Move bestMove;

    jmp_buf jumpBuffer;

    Stack ss[128];

    int16_t history[COLOR_NB][64][64];

    // Anything below here is not zeroed out between searches
    Position pos;
    PawnCache pawnCache;

    int index;
    int count;

    pthread_mutex_t mutex;
    pthread_cond_t sleepCondition;
    pthread_t *pthreads;

} Thread;


Thread *InitThreads(int threadCount);
uint64_t TotalNodes(const Thread *threads);
uint64_t TotalTBHits(const Thread *threads);
void PrepareSearch(Thread *threads, Position *pos);
void ResetThreads(Thread *threads);
void Wait(Thread *thread, volatile bool *condition);
void Wake(Thread *thread);
