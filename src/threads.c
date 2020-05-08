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

#include <stdlib.h>
#include <string.h>

#include "types.h"


Thread *InitThreads(int threadCount) {

    Thread *threads = calloc(threadCount, sizeof(Thread));

    for (int i = 0; i < threadCount; ++i)
        threads[i].index = i;

    threads[0].threads = threads;

    return threads;
}

uint64_t TotalNodes(Thread *threads, int threadCount) {

    uint64_t total = 0;

    for (int i = 0; i < threadCount; ++i)
        total += threads[i].nodes;

    return total;
}

uint64_t TotalTBHits(Thread *threads, int threadCount) {

    uint64_t total = 0;

    for (int i = 0; i < threadCount; ++i)
        total += threads[i].tbhits;

    return total;
}