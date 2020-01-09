// transposition.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


// Clears the transposition table
void ClearTT(TT *table) {

    memset(table->TT, 0, table->numEntries * sizeof(TTEntry));
}

// Initializes the transposition table
void InitTT(TT *table, uint64_t MB) {

    // Ignore if already initialized with this size
    if (table->TT != NULL && table->MB == MB) {
        printf("HashTable already initialized to %" PRIu64 ".\n", MB);
        fflush(stdout);
        return;
    }

    uint64_t HashSize = 0x100000LL * MB;
    table->numEntries = HashSize / sizeof(TTEntry);

    MB = MB < MINHASH ? MINHASH : MB; // Don't go under minhash
    MB = MB > MAXHASH ? MAXHASH : MB; // Don't go over maxhash

    // Free memory if we have already allocated
    if (table->TT != NULL)
        free(table->TT);

    // Allocate memory
    table->TT = (TTEntry *)calloc(table->numEntries, sizeof(TTEntry));

    // If allocation fails, try half the size
    if (table->TT == NULL) {
        printf("Hash Allocation Failed, trying %" PRIu64 "MB...\n", MB / 2);
        fflush(stdout);
        InitTT(table, MB / 2);

    // Success
    } else {
        table->MB = MB;
        printf("HashTable init complete with %d entries, using %" PRIu64 "MB.\n", table->numEntries, MB);
        fflush(stdout);
    }
}

// Probe the transposition table
TTEntry* ProbeTT(const Position *pos, const uint64_t posKey, bool *ttHit) {

    TTEntry* tte = &pos->hashTable->TT[pos->posKey % pos->hashTable->numEntries];

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
int HashFull(const Position *pos) {

    uint64_t used = 0;
    const int samples = 1000;

    for (int i = 0; i < samples; ++i)
        if (pos->hashTable->TT[i].move != NOMOVE)
            used++;

    return used / (samples / 1000);
}