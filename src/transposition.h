/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2022 Terje Kirstihagen

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

#include "threads.h"
#include "types.h"


// 2MB hash is a reasonable expectation.
#define MINHASH 2
// 65536MB = 2^32 * 16B / (1024 * 1024)
// is the limit current indexing is able
// to use given the 16B size of entries
#define MAXHASH 65536
#define DEFAULTHASH 32

#define BUCKETSIZE 2

#define ValidBound(bound) (bound >= BOUND_UPPER && bound <= BOUND_EXACT)
#define ValidScore(score) (score >= -MATE && score <= MATE)


enum { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };

// Constants used for operating on the combined bound + age field
enum {
    TT_BOUND_BITS = 2,                              // Number of bits representing bound
    TT_BOUND_MASK = (1 << TT_BOUND_BITS) - 1,       // Mask to pull out bound
    TT_AGE_OFFSET = TT_BOUND_BITS,                  // Number of bits reserved for other things
    TT_AGE_DELTA  = 1 << TT_AGE_OFFSET,             // Increment for age each turn
    TT_AGE_CYCLE  = 255 + (1 << TT_AGE_OFFSET),     // Cycle length
    TT_AGE_MASK   = (0xFF << TT_AGE_OFFSET) & 0xFF, // Mask to pull out generation number
};

typedef struct {
    Key key;
    Move move;
    int16_t score;
    uint8_t depth;
    uint8_t ageBound;
} TTEntry;

typedef struct {
    TTEntry entries[2];
} TTBucket;

typedef struct {
    void *mem;
    TTBucket *table;
    uint64_t count;
    uint64_t currentMB;
    uint64_t requestedMB;
    uint8_t age;
    bool dirty;
} TranspositionTable;


extern TranspositionTable TT;


// Mate scores are stored as mate in 0 as they depend on the current ply
INLINE int ScoreToTT (const int score, const uint8_t ply) {
    return score >=  TBWIN_IN_MAX ? score + ply
         : score <= -TBWIN_IN_MAX ? score - ply
                                  : score;
}

// Translates from mate in 0 to the proper mate score at current ply
INLINE int ScoreFromTT (const int score, const uint8_t ply) {
    return score >=  TBWIN_IN_MAX ? score - ply
         : score <= -TBWIN_IN_MAX ? score + ply
                                  : score;
}

INLINE TTBucket *GetTTBucket(Key key) {
    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    return &TT.table[((uint32_t)key * (uint64_t)TT.count) >> 32];
}

INLINE void RequestTTSize(int megabytes) {
    TT.requestedMB = megabytes;
    puts("info string Hash will resize after next 'isready'.");
}

INLINE void AgeTT() {
    TT.age += TT_AGE_DELTA;
}

TTEntry* ProbeTT(Key key, bool *ttHit);
void StoreTTEntry(TTEntry *tte, Key key, Move move, int score, Depth depth, int bound);
int HashFull();
void ClearTT();
void InitTT();
