/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2023 Terje Kirstihagen

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

#include "movegen.h"
#include "threads.h"


Thread *Threads;
static pthread_t *pthreads;

// Used for letting the main thread sleep without using cpu
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sleepCondition = PTHREAD_COND_INITIALIZER;


// Allocates memory for thread structs
void InitThreads(int count) {

    if (Threads)  free(Threads);
    if (pthreads) free(pthreads);

    Threads  = calloc(count, sizeof(Thread));
    pthreads = calloc(count, sizeof(pthread_t));

    // Each thread knows its own index and total thread count
    for (int i = 0; i < count; ++i)
        Threads[i].index = i,
        Threads[i].count = count;
}

// Sorts all rootmoves searched by multiPV
void SortRootMoves(Thread *thread, int multiPV) {
    for (int i = 0; i < multiPV; ++i) {

        int bestIdx = i;
        int bestScore = thread->rootMoves[i].score;

        for (int k = i + 1; k < multiPV; ++k)
            if (thread->rootMoves[k].score > bestScore)
                bestScore = thread->rootMoves[bestIdx = k].score;

        RootMove best = thread->rootMoves[bestIdx];
        thread->rootMoves[bestIdx] = thread->rootMoves[i];
        thread->rootMoves[i] = best;
    }
}

// Tallies the nodes searched by all threads
uint64_t TotalNodes() {
    uint64_t total = 0;
    for (int i = 0; i < Threads->count; ++i)
        total += Threads[i].pos.nodes;
    return total;
}

// Tallies the tbhits of all threads
uint64_t TotalTBHits() {
    uint64_t total = 0;
    for (int i = 0; i < Threads->count; ++i)
        total += Threads[i].tbhits;
    return total;
}

// Setup threads for a new search
void PrepareSearch(Position *pos, Move searchmoves[]) {
    int rootMoveCount = LegalMoveCount(pos, searchmoves);

    for (Thread *t = Threads; t < Threads + Threads->count; ++t) {
        memset(t, 0, offsetof(Thread, pos));
        memcpy(&t->pos, pos, sizeof(Position));
        t->rootMoveCount = rootMoveCount;
        for (Depth d = 0; d <= MAX_PLY; ++d)
            (t->ss+SS_OFFSET+d)->ply = d;
        for (Depth d = -7; d < 0; ++d)
            (t->ss+SS_OFFSET+d)->continuation = &t->continuation[0][0][EMPTY][0],
            (t->ss+SS_OFFSET+d)->contCorr = &t->contCorrHistory[EMPTY][0];
    }
}

// Start the main thread running the provided function
void StartMainThread(void *(*func)(void *), Position *pos) {
    pthread_create(&pthreads[0], NULL, func, pos);
    pthread_detach(pthreads[0]);
}

static bool helpersActive = false;

// Start helper threads running the provided function
void StartHelpers(void *(*func)(void *)) {
    helpersActive = true;
    for (int i = 1; i < Threads->count; ++i)
        pthread_create(&pthreads[i], NULL, func, &Threads[i]);
}

// Wait for helper threads to finish
void WaitForHelpers() {
    if (!helpersActive) return;
    for (int i = 1; i < Threads->count; ++i)
        pthread_join(pthreads[i], NULL);
    helpersActive = false;
}

// Reset all data that isn't reset each turn
void ResetThreads() {
    for (int i = 0; i < Threads->count; ++i)
        memset(Threads[i].pawnCache,       0, sizeof(PawnCache)),
        memset(Threads[i].history,         0, sizeof(Threads[i].history)),
        memset(Threads[i].pawnHistory,     0, sizeof(Threads[i].pawnHistory)),
        memset(Threads[i].captureHistory,  0, sizeof(Threads[i].captureHistory)),
        memset(Threads[i].continuation,    0, sizeof(Threads[i].continuation)),
        memset(Threads[i].pawnCorrHistory, 0, sizeof(Threads[i].pawnCorrHistory)),
        memset(Threads[i].matCorrHistory,  0, sizeof(Threads[i].matCorrHistory)),
        memset(Threads[i].contCorrHistory, 0, sizeof(Threads[i].contCorrHistory));
}

// Run the given function once in each thread
void RunWithAllThreads(void *(*func)(void *)) {
    for (int i = 0; i < Threads->count; ++i)
        pthread_create(&pthreads[i], NULL, func, &Threads[i]);
    for (int i = 0; i < Threads->count; ++i)
        pthread_join(pthreads[i], NULL);
}

// Thread sleeps until it is woken up
void Wait(atomic_bool *condition) {
    pthread_mutex_lock(&mutex);
    while (!atomic_load(condition))
        pthread_cond_wait(&sleepCondition, &mutex);
    pthread_mutex_unlock(&mutex);
}

// Wakes up a sleeping thread
void Wake() {
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&sleepCondition);
    pthread_mutex_unlock(&mutex);
}
