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


extern uint8_t SqDistance[64][64];

extern const int NonPawn[PIECE_NB];
extern const int PiecePawn[PIECE_NB];
extern const int PhaseValue[PIECE_NB];

// Zobrist keys
extern uint64_t PieceKeys[PIECE_NB][64];
extern uint64_t CastleKeys[16];
extern uint64_t SideKey;


void ParseFen(const char *fen, Position *pos);
Key KeyAfter(const Position *pos, int move);
#ifndef NDEBUG
void PrintBoard(const Position *pos);
bool CheckBoard(const Position *pos);
#endif
#ifdef DEV
void PrintBoard(const Position *pos);
void MirrorBoard(Position *pos);
#endif

// Mirrors a square horizontally
INLINE int MirrorSquare(const int sq) {
    return sq ^ 56;
}

// Returns distance between sq1 and sq2
INLINE int Distance(const int sq1, const int sq2) {
    return SqDistance[sq1][sq2];
}

INLINE int RelativeRank(const int side, const int rank) {
    return side == WHITE ? rank : RANK_8 - rank;
}