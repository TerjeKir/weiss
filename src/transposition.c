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

#if defined(__linux__)
    #include <sys/mman.h>
#endif

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


TranspositionTable TT = { .requestedMB = DEFAULTHASH };


// Probe the transposition table
TTEntry* ProbeTT(const Key key, bool *ttHit) {

    TTEntry* tte = GetEntry(key);

    *ttHit = tte->key == key;

    return tte;
}

// Store an entry in the transposition table
void StoreTTEntry(TTEntry *tte, const Key key,
                                const Move move,
                                const int score,
                                const Depth depth,
                                const int bound) {

    assert(ValidBound(bound));
    assert(ValidDepth(depth));
    assert(ValidScore(score));

    // Store new data unless it would overwrite data about the same
    // position searched to a higher depth.
    if (key != tte->key || depth * 2 >= tte->depth || bound == BOUND_EXACT)
        tte->key   = key,
        tte->move  = move,
        tte->score = score,
        tte->depth = depth,
        tte->bound = bound;
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

    Thread *thread = voidThread;
    int index = thread->index;
    int count = thread->count;

    // Logic for dividing the work taken from CFish
    uint64_t twoMB  = 2 * 1024 * 1024;
    uint64_t size   = TT.count * sizeof(TTEntry);
    uint64_t slice  = (size + count - 1) / count;
    uint64_t blocks = (slice + twoMB - 1) / twoMB;
    uint64_t begin  = MIN(size, index * blocks * twoMB);
    uint64_t end    = MIN(size, begin + blocks * twoMB);

    memset(TT.table + begin / sizeof(TTEntry), 0, end - begin);

    return NULL;
}

// Clears the transposition table
void ClearTT() {
    if (!TT.dirty) return;
    RunWithAllThreads(ThreadClearTT);
    TT.dirty = false;
}

// Allocates memory for the transposition table
void InitTT() {

    // Skip if already correct size
    if (TT.currentMB == TT.requestedMB)
        return;

    // Free memory if previously allocated
    if (TT.mem)
        free(TT.mem);

    uint64_t size = TT.requestedMB * 1024 * 1024;

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
        printf("Failed to allocate %" PRIu64 "MB for transposition table.\n", TT.requestedMB);
        exit(EXIT_FAILURE);
    }

    TT.currentMB = TT.requestedMB;
    TT.count = size / sizeof(TTEntry);

    // Zero out the memory
    TT.dirty = true;
    ClearTT();
}
