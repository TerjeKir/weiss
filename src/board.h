// board.h

#pragma once

#include "types.h"


uint8_t distance[64][64];


bool CheckBoard(const Position *pos);
void ParseFen(const char *fen, Position *pos);
void PrintBoard(const Position *pos);
void MirrorBoard(Position *pos);

// Returns distance between sq1 and sq2
static inline int Distance(const int sq1, const int sq2) {
    return distance[sq1][sq2];
}