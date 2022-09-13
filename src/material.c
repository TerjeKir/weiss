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

// #pragma once

#include <stdlib.h>
#include <string.h>

#include "material.h"


Endgame endgameTable[ENDGAME_TABLE_SIZE] = { 0 };


// Generates a material key from a string like "KRPKR"
static Key GenMaterialKey(const char *str) {

    assert(strlen(str) < 8);

    char fen[64];
    sprintf(fen, "%s%c/8/8/8/8/8/8/8 w - - 0 1", str, (char)(8 - strlen(str) + '0'));

    Position pos;
    ParseFen(fen, &pos);

    return pos.materialKey;
}

int TrivialDraw(__attribute__((unused)) const Position *pos, __attribute__((unused)) Color color) {
    return 0;
}

static void AddEndgame(const char *pieces, SpecializedEval ef) {

    Key key = GenMaterialKey(pieces);

    Endgame *eg = &endgameTable[EndgameIndex(key)];

    if (eg->evalFunc != NULL) {
        printf("Collision in endgame table.\n");
        exit(0);
    }

    *eg = (Endgame) { key, ef };
}

void InitEndgames() {
    // King vs king
    AddEndgame("Kk",  &TrivialDraw);

    // Single minor vs lone king
    AddEndgame("KNk", &TrivialDraw);
    AddEndgame("KBk", &TrivialDraw);
    AddEndgame("Kkn", &TrivialDraw);
    AddEndgame("Kkb", &TrivialDraw);

    // Single minor or rook vs single minor or rook
    AddEndgame("KNkn", &TrivialDraw);
    AddEndgame("KNkb", &TrivialDraw);
    AddEndgame("KBkn", &TrivialDraw);
    AddEndgame("KBkb", &TrivialDraw);
    AddEndgame("KRkr", &TrivialDraw);

    // 2 knights vs lone king
    AddEndgame("KNNk", &TrivialDraw);
    AddEndgame("Kknn", &TrivialDraw);
}
