// transposition.h

#pragma once

#include "types.h"


#define MINHASH 4
#define MAXHASH 16384
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

void ClearTT();
void InitTT();
TTEntry* ProbeTT(const Position *pos, const uint64_t key, bool *ttHit);
void StoreTTEntry(TTEntry *tte, const uint64_t posKey, const int move, const int score, const int depth, const int flag);
int HashFull();