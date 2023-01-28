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

// #pragma once

#include <stdlib.h>
#include <string.h>

#include "material.h"


Endgame endgameTable[ENDGAME_TABLE_SIZE] = { 0 };


// Generates a material key from a string like "KRPkr"
static Key GenMaterialKey(const char *white, const char *black) {

    assert(strlen(white) < 8);
    assert(strlen(black) < 8);

    char fen[64];
    sprintf(fen, "%s%c/8/8/8/8/8/8/%s%c w - - 0 1", black, (char)(8 - strlen(black) + '0'), white, (char)(8 - strlen(white) + '0'));

    printf("%s\n", fen);

    Position pos;
    ParseFen(fen, &pos);

    return pos.materialKey;
}

static int TrivialDraw(__attribute__((unused)) const Position *pos, __attribute__((unused)) Color color) {
    return 0;
}

static void AddEndgame(const char *white, const char *black, SpecializedEval ef) {

    Key key = GenMaterialKey(white, black);

    Endgame *eg = &endgameTable[EndgameIndex(key)];

    if (eg->evalFunc != NULL) {
        puts("Collision in endgame table.");
        exit(0);
    }

    *eg = (Endgame) { key, ef };
}

CONSTR(3) InitEndgames() {
    // King vs king
    AddEndgame("K", "k", &TrivialDraw);

    // Single minor vs lone king
    AddEndgame("KN", "k", &TrivialDraw);
    AddEndgame("KB", "k", &TrivialDraw);
    AddEndgame("K", "kn", &TrivialDraw);
    AddEndgame("K", "kb", &TrivialDraw);

    // Single minor or rook vs single minor or rook
    AddEndgame("KN", "kn", &TrivialDraw);
    AddEndgame("KN", "kb", &TrivialDraw);
    AddEndgame("KB", "kn", &TrivialDraw);
    AddEndgame("KB", "kb", &TrivialDraw);

    // 2 knights vs lone king
    AddEndgame("KNN", "k", &TrivialDraw);
    AddEndgame("K", "knn", &TrivialDraw);
}
