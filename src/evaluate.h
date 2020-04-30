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


// Check for (likely) material draw
#define CHECK_MAT_DRAW


#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define S(mg, eg) MakeScore((mg), (eg))
#define MgScore(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define EgScore(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

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


int EvalPosition(const Position *pos);
