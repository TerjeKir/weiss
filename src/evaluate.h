/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2023 Terje Kirstihagen

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

#include "board.h"
#include "types.h"


#define PAWN_CACHE_SIZE 128 * 1024

typedef struct PawnEntry {
    Key key;
    Bitboard passedPawns;
    int eval;
} PawnEntry;

typedef PawnEntry PawnCache[PAWN_CACHE_SIZE];


extern const int Tempo;
extern const int PieceValue[COLOR_NB][PIECE_NB];


// Tapered Eval
enum Phase { MG, EG };

static const int MidGame = 256;
extern const int PhaseValue[TYPE_NB];

#define S(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define MgScore(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define EgScore(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

// Calculates the phase from the phase values of the pieces left
static inline int UpdatePhase(int value) {
    return (value * MidGame + 12) / 24;
}

// Returns a static evaluation of the position
int EvalPosition(const Position *pos, PawnCache pc);
