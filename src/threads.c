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


// Allocates memory for thread structs
Thread *InitThreads(int count) {

    Thread *threads = calloc(count, sizeof(Thread));

    // Each thread knows its own index
    for (int i = 0; i < count; ++i)
        threads[i].index = i;

    // The main thread keeps track of how many threads are in use
    threads[0].count = count;

    return threads;
}

// Tallies the nodes searched by all threads
uint64_t TotalNodes(const Thread *threads) {

    uint64_t total = 0;
    for (int i = 0; i < threads->count; ++i)
        total += threads[i].nodes;
    return total;
}

// Tallies the tbhits of all threads
uint64_t TotalTBHits(const Thread *threads) {

    uint64_t total = 0;
    for (int i = 0; i < threads->count; ++i)
        total += threads[i].tbhits;
    return total;
}