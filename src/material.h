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

#include "board.h"
#include "types.h"


#define ENDGAME_TABLE_SIZE 64


typedef int (*SpecializedEval) (const Position *pos, Color color);

typedef struct Endgame {
    Key key;
    SpecializedEval evalFunc;
} Endgame;


extern Endgame endgameTable[ENDGAME_TABLE_SIZE];


INLINE int EndgameIndex(Key materialKey) {
    return materialKey & (ENDGAME_TABLE_SIZE - 1);
}

void InitEndgames();
