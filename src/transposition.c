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

#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
    #include <sys/mman.h>
#endif

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


TranspositionTable TT = { .requestedMB = HASH_DEFAULT };


// Probe the transposition table
TTEntry* ProbeTT(const Key key, bool *ttHit) {

    TTEntry* first = GetTTBucket(key)->entries;

    for (TTEntry *entry = first; entry < first + BUCKET_SIZE; ++entry)
        if (entry->key == (int32_t)key || !Bound(entry)) {
            entry->genBound = TT.generation | Bound(entry);
            return *ttHit = Bound(entry), entry;
        }

    TTEntry *replace = first;
    for (TTEntry *entry = first + 1; entry < first + BUCKET_SIZE; ++entry)
        if (EntryValue(replace) > EntryValue(entry))
            replace = entry;

    return *ttHit = false, replace;
}

// Store an entry in the transposition table
void StoreTTEntry(TTEntry *tte, const Key key,
                                const Move move,
                                const int score,
                                const int eval,
                                const Depth depth,
                                const int bound) {

    assert(ValidBound(bound));
    assert(ValidScore(score));

    if (move || (int32_t)key != tte->key)
        tte->move = move;

    // Store new data unless it would overwrite data about the same
    // position searched to a higher depth.
    if ((int32_t)key != tte->key || depth + 4 >= tte->depth || bound == BOUND_EXACT)
        tte->key   = key,
        tte->score = score,
        tte->eval  = eval,
        tte->depth = depth,
        tte->genBound = TT.generation | bound;
}

// Estimates the load factor of the transposition table (1 = 0.1%)
int HashFull() {

    int used = 0;

    for (TTBucket *bucket = TT.table; bucket < TT.table + 1000; ++bucket)
        for (TTEntry *entry = bucket->entries; entry < bucket->entries + BUCKET_SIZE; ++entry)
            used += (        Bound(entry) != 0
                     && Generation(entry) == TT.generation);

    return used / BUCKET_SIZE;
}

static void *ThreadClearTT(void *voidThread) {

    Thread *thread = voidThread;
    int index = thread->index;
    int count = thread->count;

    // Logic for dividing the work taken from CFish
    uint64_t twoMB  = 2 * 1024 * 1024;
    uint64_t size   = TT.count * sizeof(TTBucket);
    uint64_t slice  = (size + count - 1) / count;
    uint64_t blocks = (slice + twoMB - 1) / twoMB;
    uint64_t begin  = MIN(size, index * blocks * twoMB);
    uint64_t end    = MIN(size, begin + blocks * twoMB);

    memset(TT.table + begin / sizeof(TTBucket), 0, end - begin);

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

    uint64_t bytes = TT.requestedMB * 1024 * 1024;

#if defined(__linux__)
    // Align on 2MB boundaries and request Huge Pages
    TT.mem = aligned_alloc(2 * 1024 * 1024, bytes);
    TT.table = (TTBucket *)TT.mem;
    madvise(TT.table, bytes, MADV_HUGEPAGE);
#else
    // Align on cache line
    TT.mem = malloc(bytes + 64 - 1);
    TT.table = (TTBucket *)(((uintptr_t)TT.mem + 64 - 1) & ~(64 - 1));
#endif

    // Allocation failed
    if (!TT.mem) {
        printf("Failed to allocate %" PRIu64 "MB for transposition table.\n", TT.requestedMB);
        exit(EXIT_FAILURE);
    }

    TT.currentMB = TT.requestedMB;
    TT.count = bytes / sizeof(TTBucket);

    // Zero out the memory
    TT.dirty = true;
    ClearTT();
}
