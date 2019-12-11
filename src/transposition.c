// transposition.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


// Clears the hash table
void ClearHashTable(HashTable *table) {

    memset(table->TT, 0, table->numEntries * sizeof(HashEntry));
}

// Initializes the hash table
void InitHashTable(HashTable *table, uint64_t MB) {

    // Ignore if already initialized with this size
    if (table->TT != NULL && table->MB == MB) {
        printf("HashTable already initialized to %" PRIu64 ".\n", MB);
        fflush(stdout);
        return;
    }

    uint64_t HashSize = 0x100000LL * MB;
    table->numEntries = HashSize / sizeof(HashEntry);

    MB = MB < MINHASH ? MINHASH : MB; // Don't go under minhash
    MB = MB > MAXHASH ? MAXHASH : MB; // Don't go over maxhash

    // Free memory if we have already allocated
    if (table->TT != NULL)
        free(table->TT);

    // Allocate memory
    table->TT = (HashEntry *)calloc(table->numEntries, sizeof(HashEntry));

    // If allocation fails, try half the size
    if (table->TT == NULL) {
        printf("Hash Allocation Failed, trying %" PRIu64 "MB...\n", MB / 2);
        fflush(stdout);
        InitHashTable(table, MB / 2);

    // Success
    } else {
        table->MB = MB;
        printf("HashTable init complete with %d entries, using %" PRIu64 "MB.\n", table->numEntries, MB);
        fflush(stdout);
    }
}

// Mate scores are stored as mate in 0 as they depend on the current ply
INLINE int ScoreToTT (int score, const int ply) {
    return score >=  ISMATE ? score + ply
         : score <= -ISMATE ? score - ply
                            : score;
}

// Translates from mate in 0 to the proper mate score at current ply
INLINE int ScoreFromTT (int score, const int ply) {
    return score >=  ISMATE ? score - ply
         : score <= -ISMATE ? score + ply
                            : score;
}

// Probe the hash table
bool ProbeHashEntry(const Position *pos, int *move, int *score, const int alpha, const int beta, const int depth) {

    assert(alpha < beta);
    assert(alpha >= -INFINITE && alpha <= INFINITE);
    assert(beta  >= -INFINITE && beta  <= INFINITE);
    assert(depth >= 1 && depth < MAXDEPTH);
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

    const int index = pos->posKey % pos->hashTable->numEntries;

    // Look for an entry at the index
    if (pos->hashTable->TT[index].posKey == pos->posKey) {

        // Use the move as PV regardless of depth
        *move = pos->hashTable->TT[index].move;

        // The score is only usable if the depth is equal or greater than current
        if (pos->hashTable->TT[index].depth >= depth) {

            assert(pos->hashTable->TT[index].depth >= 1 && pos->hashTable->TT[index].depth < MAXDEPTH);
            assert(pos->hashTable->TT[index].flag >= BOUND_UPPER && pos->hashTable->TT[index].flag <= BOUND_EXACT);

            *score = ScoreFromTT(pos->hashTable->TT[index].score, pos->ply);

            assert(-INFINITE <= *score && *score <= INFINITE);

            // Return true if the score is usable
            uint8_t flag = pos->hashTable->TT[index].flag;
            if (   (flag == BOUND_UPPER && *score <= alpha)
                || (flag == BOUND_LOWER && *score >= beta)
                ||  flag == BOUND_EXACT)
                return true;
        }
    }
    return false;
}

// Store an entry in the hash table
void StoreHashEntry(Position *pos, const int move, const int score, const int flag, const int depth) {

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