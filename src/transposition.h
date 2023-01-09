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

#include <math.h>

#include "threads.h"
#include "types.h"


#define HASH_MIN 2
#define HASH_MAX ((int)(pow(2, 32) * sizeof(TTBucket) / (1024 * 1024)))
#define HASH_DEFAULT 32

#define BUCKET_SIZE 2

#define ValidBound(bound) (bound >= BOUND_UPPER && bound <= BOUND_EXACT)
#define ValidScore(score) (score >= -MATE && score <= MATE)


enum { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };

// Constants used for operating on the combined bound + generation field
enum {
    TT_BOUND_BITS = 2,                              // Number of bits representing bound
    TT_BOUND_MASK = (1 << TT_BOUND_BITS) - 1,       // Mask to pull out bound
    TT_GEN_OFFSET = TT_BOUND_BITS + 1,              // Number of bits reserved for other things
    TT_GEN_DELTA  = 1 << TT_GEN_OFFSET,             // Increment for generation each turn
    TT_GEN_CYCLE  = 255 + (1 << TT_GEN_OFFSET),     // Cycle length
    TT_GEN_MASK   = (0xFF << TT_GEN_OFFSET) & 0xFF, // Mask to pull out generation number
};

typedef struct {
    Key key;
    Move move;
    int16_t score;
    uint8_t depth;
    uint8_t genBound;
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
    uint8_t generation;
    bool dirty;
} TranspositionTable;


extern TranspositionTable TT;


INLINE uint8_t      Bound(TTEntry *entry) { return entry->genBound & TT_BOUND_MASK; }
INLINE uint8_t Generation(TTEntry *entry) { return entry->genBound & TT_GEN_MASK; }
INLINE uint8_t        Age(TTEntry *entry) { return (TT_GEN_CYCLE + TT.generation - entry->genBound) & TT_GEN_MASK; }

INLINE int EntryValue(TTEntry *entry) { return entry->depth - Age(entry); }

// Store terminal scores as distance from the current position to mate/TB
INLINE int ScoreToTT (const int score, const uint8_t ply) {
    return score >=  TBWIN_IN_MAX ? score + ply
         : score <= -TBWIN_IN_MAX ? score - ply
                                  : score;
}

// Add the distance from root to terminal scores get the total distance to mate/TB
INLINE int ScoreFromTT (const int score, const uint8_t ply) {
    return score >=  TBWIN_IN_MAX ? score - ply
         : score <= -TBWIN_IN_MAX ? score + ply
                                  : score;
}

INLINE uint64_t TTIndex(Key key) {
    return ((unsigned __int128)key * (unsigned __int128)TT.count) >> 64;
}

INLINE TTBucket *GetTTBucket(Key key) {
    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    return &TT.table[TTIndex(key)];
}

INLINE void TTPrefetch(Key key) {
    __builtin_prefetch(GetTTBucket(key));
}

INLINE void RequestTTSize(int megabytes) {
    TT.requestedMB = megabytes;
    puts("info string Hash will resize after next 'isready'.");
}

INLINE void TTNewSearch() {
    TT.generation += TT_GEN_DELTA;
    TT.dirty = true;
}

TTEntry* ProbeTT(Key key, bool *ttHit);
void StoreTTEntry(TTEntry *tte, Key key, Move move, int score, Depth depth, int bound);
int HashFull();
void ClearTT();
void InitTT();
