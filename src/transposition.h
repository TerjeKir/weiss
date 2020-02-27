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

#pragma once

#include "types.h"


// 2MB hash is a reasonable expectation.
#define MINHASH 2
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
INLINE int ScoreToTT (const int score, const uint8_t ply) {
    return score >=  ISMATE ? score + ply
         : score <= -ISMATE ? score - ply
                            : score;
}

// Translates from mate in 0 to the proper mate score at current ply
INLINE int ScoreFromTT (const int score, const uint8_t ply) {
    return score >=  ISMATE ? score - ply
         : score <= -ISMATE ? score + ply
                            : score;
}

INLINE TTEntry *GetEntry(Key posKey) {

    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    return &TT.table[((uint32_t)posKey * (uint64_t)TT.count) >> 32];
}

void ClearTT();
void InitTT();
TTEntry* ProbeTT(Key posKey, bool *ttHit);
void StoreTTEntry(TTEntry *tte, Key posKey, Move move, int score, Depth depth, int flag);
int HashFull();