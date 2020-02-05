// transposition.h

#pragma once

#include "types.h"


// 1MB hash is a reasonable expectation.
#define MINHASH 1
// 65536MB = 2^32 * 16B / (1024 * 1024)
// is the limit current indexing is able
// to use given the 16B size of entries
#define MAXHASH 65536
#define DEFAULTHASH 32


enum { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };

typedef struct {

    void *mem;
    TTEntry *table;
    size_t count;
    size_t currentMB;
    size_t requestedMB;
    bool dirty;

} TranspositionTable;


extern TranspositionTable TT;


// Mate scores are stored as mate in 0 as they depend on the current ply
INLINE int ScoreToTT (const int score, const int ply) {
    return score >=  ISMATE ? score + ply
         : score <= -ISMATE ? score - ply
                            : score;
}

// Translates from mate in 0 to the proper mate score at current ply
INLINE int ScoreFromTT (const int score, const int ply) {
    return score >=  ISMATE ? score - ply
         : score <= -ISMATE ? score + ply
                            : score;
}

void ClearTT();
void InitTT();
TTEntry* ProbeTT(Key posKey, bool *ttHit);
void StoreTTEntry(TTEntry *tte, Key posKey, int move, int score, int depth, int flag);
int HashFull();