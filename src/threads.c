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

    // Each thread knows its own index and total thread count
    for (int i = 0; i < count; ++i)
        threads[i].index = i,
        threads[i].count = count;

    // Used for letting the main thread sleep
    pthread_mutex_init(&threads->mutex, NULL);
    pthread_cond_init(&threads->sleepCondition, NULL);

    // Array of pthreads
    threads->pthreads = calloc(count, sizeof(pthread_t));

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

// Thread sleeps until it is woken up
void Wait(Thread *thread, volatile bool *condition) {

    pthread_mutex_lock(&thread->mutex);
    while (!*condition)
        pthread_cond_wait(&thread->sleepCondition, &thread->mutex);
    pthread_mutex_unlock(&thread->mutex);
}

// Wakes up a sleeping thread
void Wake(Thread *thread) {

    pthread_mutex_lock(&thread->mutex);
    pthread_cond_signal(&thread->sleepCondition);
    pthread_mutex_unlock(&thread->mutex);
}
