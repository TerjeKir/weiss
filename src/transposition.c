// transposition.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


TranspositionTable TT;


// Clears the transposition table
void ClearTT() {

    if (!TT.dirty) return;

    memset(TT.table, 0, TT.count * sizeof(TTEntry));

    TT.dirty = false;
}

// Allocates memory for the transposition table
void InitTT() {

    // Ignore if already correct size
    if (TT.currentMB == TT.requestedMB)
        return;

    size_t MB = TT.requestedMB;

    TT.count = MB * 1024 * 1024 / sizeof(TTEntry);

    // Free memory if already allocated
    if (TT.currentMB > 0)
        free(TT.mem);

    // Allocate memory
    TT.mem = malloc(TT.count * sizeof(TTEntry) + 64 - 1);

    // Allocation failed
    if (!TT.mem) {
        printf("Allocating %" PRI_SIZET "MB for the transposition table failed.\n", MB);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    TT.table = (TTEntry *)(((uintptr_t)TT.mem + 64 - 1) & ~(64 - 1));
    TT.currentMB = MB;

    // Ensure the memory is 0'ed out
    TT.dirty = true;
    ClearTT();

    printf("HashTable init complete with %" PRI_SIZET " entries, using %" PRI_SIZET "MB.\n", TT.count, MB);
    fflush(stdout);
}

// Probe the transposition table
TTEntry* ProbeTT(const Key posKey, bool *ttHit) {

    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    TTEntry* tte = &TT.table[((uint32_t)posKey * (uint64_t)TT.count) >> 32];

    *ttHit = tte->posKey == posKey;

    return tte;
}

// Store an entry in the transposition table
void StoreTTEntry(TTEntry *tte, const Key posKey, const int move, const int score, const int depth, const int flag) {

    assert(BOUND_UPPER <= flag && flag <= BOUND_EXACT);
    assert( -INFINITE <= score && score <= INFINITE);
    assert(         1 <= depth && depth < MAXDEPTH);

    // Store new data unless it would overwrite data about the same
    // position searched to a higher depth.
    if (posKey != tte->posKey || depth >= tte->depth || flag == BOUND_EXACT)
        tte->posKey = posKey,
        tte->move   = move,
        tte->score  = score,
        tte->depth  = depth,
        tte->flag   = flag;
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