// board.h

#pragma once

#include "types.h"


uint8_t distance[64][64];


void InitDistance();
int CheckBoard(const S_BOARD *pos);
int ParseFen(const char *fen, S_BOARD *pos);
void PrintBoard(const S_BOARD *pos);
void MirrorBoard(S_BOARD *pos);

// Returns distance between sq1 and sq2
static inline int Distance(const int sq1, const int sq2) {
    return distance[sq1][sq2];
}