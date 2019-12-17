// transposition.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


// Clears the hash table
void ClearTT(TT *table) {

    memset(table->TT, 0, table->numEntries * sizeof(TTEntry));
}

// Initializes the hash table
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

// Probe the TT
TTEntry* ProbeTT(const Position *pos, const uint64_t posKey, bool *ttHit) {

    TTEntry* tte = &pos->hashTable->TT[pos->posKey % pos->hashTable->numEntries];

    *ttHit = tte->posKey == posKey;

    return tte;
}

// Store an entry in the hash table
void StoreTTEntry(Position *pos, const int move, const int score, const int flag, const int depth) {

    assert(-INFINITE <= score && score <= INFINITE);
    assert(flag >= BOUND_UPPER && flag <= BOUND_EXACT);
    assert(depth >= 1 && depth < MAXDEPTH);
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

    const int index = pos->posKey % pos->hashTable->numEntries;

    pos->hashTable->TT[index].posKey = pos->posKey;
    pos->hashTable->TT[index].move   = move;
    pos->hashTable->TT[index].score  = ScoreToTT(score, pos->ply);
    pos->hashTable->TT[index].depth  = depth;
    pos->hashTable->TT[index].flag   = flag;

    assert(pos->hashTable->TT[index].score >= -INFINITE);
    assert(pos->hashTable->TT[index].score <=  INFINITE);
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