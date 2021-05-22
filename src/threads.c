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

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "threads.h"


Thread *threads;
static pthread_t *pthreads;

// Used for letting the main thread sleep without using cpu
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sleepCondition = PTHREAD_COND_INITIALIZER;


// Allocates memory for thread structs
Thread *InitThreads(int count) {

    if (threads) free(threads);
    if (pthreads) free(pthreads);

    Thread *t = calloc(count, sizeof(Thread));

    // Each thread knows its own index and total thread count
    for (int i = 0; i < count; ++i)
        t[i].index = i,
        t[i].count = count;

    // Array of pthreads
    pthreads = calloc(count, sizeof(pthread_t));

    return t;
}

// Tallies the nodes searched by all threads
uint64_t TotalNodes() {
    uint64_t total = 0;
    for (int i = 0; i < threads->count; ++i)
        total += threads[i].pos.nodes;
    return total;
}

// Tallies the tbhits of all threads
uint64_t TotalTBHits() {
    uint64_t total = 0;
    for (int i = 0; i < threads->count; ++i)
        total += threads[i].tbhits;
    return total;
}

// Setup threads for a new search
void PrepareSearch(Position *pos) {
    for (Thread *t = threads; t < threads + threads->count; ++t) {
        memset(t, 0, offsetof(Thread, pos));
        memcpy(&t->pos, pos, sizeof(Position));
        for (Depth d = 0; d < MAX_PLY; ++d)
            (t->ss+SS_OFFSET+d)->ply = d;
    }
}

// Start the main thread running the provided function
void StartMainThread(void *(*func)(void *), Position *pos) {
    pthread_create(&pthreads[0], NULL, func, pos);
    pthread_detach(pthreads[0]);
}

// Start helper threads running the provided function
void StartHelpers(void *(*func)(void *)) {
    for (int i = 1; i < threads->count; ++i)
        pthread_create(&pthreads[i], NULL, func, &threads[i]);
}

void WaitForHelpers() {
    for (int i = 1; i < threads->count; ++i)
        pthread_join(pthreads[i], NULL);
}

// Resets all data that isn't reset each turn
void ResetThreads() {
    for (int i = 0; i < threads->count; ++i)
        memset(threads[i].pawnCache, 0, sizeof(PawnCache));
}

// Runs the given function once in each thread
void RunWithAllThreads(void *(*func)(void *)) {
    for (int i = 0; i < threads->count; ++i)
        pthread_create(&pthreads[i], NULL, func, &threads[i]);
    for (int i = 0; i < threads->count; ++i)
        pthread_join(pthreads[i], NULL);
}

// Thread sleeps until it is woken up
void Wait(volatile bool *condition) {
    pthread_mutex_lock(&mutex);
    while (!*condition)
        pthread_cond_wait(&sleepCondition, &mutex);
    pthread_mutex_unlock(&mutex);
}

// Wakes up a sleeping thread
void Wake() {
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&sleepCondition);
    pthread_mutex_unlock(&mutex);
}
