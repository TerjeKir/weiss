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

// Initializes the transposition table
void InitTT(uint64_t MB) {

    // Ignore if already initialized with this size
    if (TT.MB && TT.MB == MB) {
        printf("HashTable already initialized to %" PRIu64 ".\n", MB);
        fflush(stdout);
        return;
    }

    uint64_t HashSize = MB * 1024 * 1024;
    TT.count = HashSize / sizeof(TTEntry);

    MB = MAX(MINHASH, MB); // Don't go under minhash
    MB = MIN(MAXHASH, MB); // Don't go over maxhash

    // Free memory if we have already allocated
    if (TT.table != NULL)
        free(TT.table);

    // Allocate memory
    TT.table = (TTEntry *)calloc(TT.count, sizeof(TTEntry));

    // If allocation fails, try half the size
    if (!TT.table) {
        printf("Hash Allocation Failed, trying %" PRIu64 "MB...\n", MB / 2);
        fflush(stdout);
        InitTT(MB / 2);

    // Success
    } else {
        TT.MB = MB;
        TT.dirty = false;
        printf("HashTable init complete with %d entries, using %" PRIu64 "MB.\n", TT.count, MB);
        fflush(stdout);
    }
}

// Probe the transposition table
TTEntry* ProbeTT(const Position *pos, const uint64_t posKey, bool *ttHit) {

    TTEntry* tte = &TT.table[pos->posKey % TT.count];

    *ttHit = tte->posKey == posKey;

    return tte;
}

// Store an entry in the transposition table
void StoreTTEntry(TTEntry *tte, const uint64_t posKey, const int move, const int score, const int depth, const int flag) {

    assert(-INFINITE <= score && score <= INFINITE);
    assert(flag >= BOUND_UPPER && flag <= BOUND_EXACT);
    assert(depth >= 1 && depth < MAXDEPTH);

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