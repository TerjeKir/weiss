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


#ifdef DEV
#define tuneable_const
#define tuneable_static_const
#else
#define tuneable_const const
#define tuneable_static_const static const
#endif


extern tuneable_const int Tempo;

extern tuneable_const int PieceTypeValue[7];
extern tuneable_const int PieceValue[2][PIECE_NB];


typedef struct EvalInfo {

    Bitboard mobilityArea[2];

} EvalInfo;


// Tapered Eval

// The importance of many features of a chess position vary based
// on what part of the game we are in. King safety is extremely
// valuable while there are many pieces on the board, but as pieces
// are traded off it is often extremely important for the king to
// get involved in the battle. To account for this we need to assign
// different values to the evaluation terms depending on the phase
// the game is in.

// This is accomplished by having a gliding transition from midgame
// to endgame based on the number of and types of pieces left. The
// final phase value ranges from 256 (midgame) to 0 (endgame). Each
// term is given two values, one for each phase, stored in a single
// integer.

enum Phase {
    MG, EG
};

static const int MidGame = 256;

extern const int PhaseValue[PIECE_NB];

#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define S(mg, eg) MakeScore((mg), (eg))
#define MgScore(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define EgScore(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

// Calculates the phase from the sum of values of the pieces left
static inline int UpdatePhase(int base) {

    return (base * MidGame + 12) / 24;
}

// Returns a static evaluation of the position
int EvalPosition(const Position *pos);
