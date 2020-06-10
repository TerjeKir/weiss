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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
    #include <sys/mman.h>
#endif

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


TranspositionTable TT = {
    .mem = NULL,
    .table = NULL,
    .count = 0,
    .currentMB = 0,
    .requestedMB = DEFAULTHASH,
    .dirty = false,
};


// Probe the transposition table
TTEntry* ProbeTT(const Key posKey, bool *ttHit) {

    TTEntry* tte = GetEntry(posKey);

    *ttHit = tte->posKey == posKey;

    return tte;
}

// Store an entry in the transposition table
void StoreTTEntry(TTEntry *tte, const Key posKey,
                                const Move move,
                                const int score,
                                const Depth depth,
                                const int bound) {

    assert(ValidBound(bound));
    assert(ValidDepth(depth));
    assert(ValidScore(score));

    // Store new data unless it would overwrite data about the same
    // position searched to a higher depth.
    if (posKey != tte->posKey || depth >= tte->depth || bound == BOUND_EXACT)
        tte->posKey = posKey,
        tte->move   = move,
        tte->score  = score,
        tte->depth  = depth,
        tte->bound  = bound;
}

// Estimates the load factor of the transposition table (1 = 0.1%)
int HashFull() {

    int used = 0;
    const int samples = 1000;

    for (int i = 0; i < samples; ++i)
        if (TT.table[i].move != NOMOVE)
            used++;

    return used / (samples / 1000);
}

static void *ThreadClearTT(void *voidThread) {

    Thread *thread = (Thread *)voidThread;

    // Logic for dividing the work taken from CFish
    size_t twoMB  = 2 * 1024 * 1024;
    size_t total  = TT.count * sizeof(TTEntry);
    size_t slice  = (total + thread->count - 1) / thread->count;
    size_t blocks = (slice + twoMB - 1) / twoMB;
    size_t begin  = thread->index * blocks * twoMB;
    size_t end    = begin + blocks * twoMB;
    begin = MIN(begin, total);
    end   = MIN(end, total);

    memset(TT.table + begin / sizeof(TTEntry), 0, end - begin);

    return NULL;
}

// Clears the transposition table
void ClearTT(Thread *threads) {

    if (!TT.dirty) return;

    // Spawn each thread to clear a part of the TT each
    for (int i = 0; i < threads->count; ++i)
        pthread_create(&threads->pthreads[i], NULL, &ThreadClearTT, &threads[i]);

    // Wait for them to finish
    for (int i = 0; i < threads->count; ++i)
        pthread_join(threads->pthreads[i], NULL);

    TT.dirty = false;
}

// Allocates memory for the transposition table
void InitTT(Thread *threads) {

    // Ignore if already correct size
    if (TT.currentMB == TT.requestedMB)
        return;

    size_t MB = TT.requestedMB;

    size_t size = MB * 1024 * 1024;
    TT.count = size / sizeof(TTEntry);

    // Free memory if already allocated
    if (TT.mem)
        free(TT.mem);

#if defined(__linux__)
    // Align on 2MB boundaries and request Huge Pages
    TT.mem = aligned_alloc(2 * 1024 * 1024, size);
    TT.table = (TTEntry *)TT.mem;
    madvise(TT.table, size, MADV_HUGEPAGE);
#else
    // Align on cache line
    TT.mem = malloc(size + 64 - 1);
    TT.table = (TTEntry *)(((uintptr_t)TT.mem + 64 - 1) & ~(64 - 1));
#endif

    // Allocation failed
    if (!TT.mem) {
        printf("Allocating %" PRI_SIZET "MB for the transposition table failed.\n", MB);
        exit(EXIT_FAILURE);
    }

    TT.currentMB = MB;

    // Ensure the memory is 0'ed out
    TT.dirty = true;
    ClearTT(threads);

    printf("HashTable init complete with %" PRI_SIZET " entries, using %" PRI_SIZET "MB.\n", TT.count, MB);
    fflush(stdout);
}
