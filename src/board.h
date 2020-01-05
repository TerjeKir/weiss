// board.h

#pragma once

#include "types.h"


uint8_t distance[64][64];


const int pieceBig [PIECE_NB];
const int piecePawn[PIECE_NB];
const int pieceKing[PIECE_NB];

const int phaseValue[PIECE_NB];

const int mirror_square[64];

bool CheckBoard(const Position *pos);
void ParseFen(const char *fen, Position *pos);
void PrintBoard(const Position *pos);
void MirrorBoard(Position *pos);

// Returns distance between sq1 and sq2
INLINE int Distance(const int sq1, const int sq2) {
    return distance[sq1][sq2];
}

INLINE int relativeRank(const int side, const int rank) {
    return side == WHITE ? rank : RANK_8 - rank;
}