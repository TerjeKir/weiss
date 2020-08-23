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
#include "types.h"


typedef struct Thread {

    uint64_t nodes;
    uint64_t tbhits;

    int score;
    Depth depth;
    Move bestMove;
    Move ponderMove;
    Depth seldepth;

    PV pv;

    jmp_buf jumpBuffer;

    int history[PIECE_NB][64];
    Move killers[MAXDEPTH][2];

    // Anything below here is not zeroed out between searches
    Position pos;

    int index;
    int count;

    pthread_mutex_t mutex;
    pthread_cond_t sleepCondition;
    pthread_t *pthreads;

} Thread;


Thread *InitThreads(int threadCount);
uint64_t TotalNodes(const Thread *threads);
uint64_t TotalTBHits(const Thread *threads);
void Wait(Thread *thread, volatile bool *condition);
void Wake(Thread *thread);
