// transposition.h

#pragma once

#include "types.h"


#define MINHASH 4
#define MAXHASH 16384
#define DEFAULTHASH 32


enum { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };


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

void ClearTT(TT *table);
void  InitTT(TT *table, uint64_t MB);
TTEntry* ProbeTT(const Position *pos, const uint64_t key, bool *ttHit);
void StoreTTEntry(Position *pos, const int move, const int score, const int flag, const int depth);
int HashFull(const Position *pos);