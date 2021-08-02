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

#include "makemove.h"
#include "movegen.h"
#include "threads.h"


Thread *threads;
static pthread_t *pthreads;

// Used for letting the main thread sleep without using cpu
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sleepCondition = PTHREAD_COND_INITIALIZER;


// Allocates memory for thread structs
void InitThreads(int count) {

    if (threads)  free(threads);
    if (pthreads) free(pthreads);

    threads  = calloc(count, sizeof(Thread));
    pthreads = calloc(count, sizeof(pthread_t));

    // Each thread knows its own index and total thread count
    for (int i = 0; i < count; ++i)
        threads[i].index = i,
        threads[i].count = count;
}

// Checks whether a move was already searched in multi-pv mode
bool AlreadySearchedMultiPV(Thread *thread, Move move) {
    for (int i = 0; i < thread->multiPV; ++i)
        if (thread->rootMoves[i].move == move)
            return true;
    return false;
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

// Checks if the move is in the list of searchmoves if any were given
static bool NotInSearchMoves(Move searchmoves[], Move move) {
    if (searchmoves[0] == 0) return false;
    for (Move *m = searchmoves; *m != 0; ++m)
        if (*m == move)
            return false;
    return true;
}

// Setup threads for a new search
void PrepareSearch(Position *pos, Move searchmoves[]) {
    int rootMoveCount = 0;
    MoveList list = { 0 };
    GenNoisyMoves(pos, &list);
    GenQuietMoves(pos, &list);
    for (int i = 0; i < list.count; ++i) {
        Move move = list.moves[list.next++].move;
        if (NotInSearchMoves(searchmoves, move)) continue;
        if (!MakeMove(pos, move)) continue;
        ++rootMoveCount;
        TakeMove(pos);
    }
    pos->nodes = 0;

    for (Thread *t = threads; t < threads + threads->count; ++t) {
        memset(t, 0, offsetof(Thread, pos));
        memcpy(&t->pos, pos, sizeof(Position));
        t->nullMover = -1;
        t->rootMoveCount = rootMoveCount;
        for (Depth d = 0; d <= MAX_PLY; ++d)
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

// Wait for helper threads to finish
void WaitForHelpers() {
    for (int i = 1; i < threads->count; ++i)
        pthread_join(pthreads[i], NULL);
}

// Reset all data that isn't reset each turn
void ResetThreads() {
    for (int i = 0; i < threads->count; ++i)
        memset(threads[i].pawnCache,      0, sizeof(PawnCache)),
        memset(threads[i].history,        0, sizeof(threads[i].history)),
        memset(threads[i].captureHistory, 0, sizeof(threads[i].captureHistory)),
        memset(threads[i].continuation,   0, sizeof(threads[i].continuation));
}

// Run the given function once in each thread
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
