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

#ifdef DEV

#include <stdio.h>
#include <string.h>

#include "evaluate.h"


enum { NAME_MAX_CHAR = 64, PARSER_ENTRY_NB = 20 };

typedef struct ParserEntry {
    char name[NAME_MAX_CHAR];
    int *values, count;
} ParserEntry;

ParserEntry ParserEntries[PARSER_ENTRY_NB];


// Piece values
extern int PieceTypeValue[7];

// PSQT values
extern int PieceSqValue[7][64];

// Misc
extern int PawnDoubled;
extern int PawnIsolated;
extern int BishopPair;
extern int KingLineDanger;

// Passed pawn [rank]
extern int PawnPassed[8];

// (Semi) open file for rook and queen [pt-4]
extern int OpenFile[2];
extern int SemiOpenFile[2];

// Mobility
extern int Mobility[4][28];


static void Declare(ParserEntry *pe, const char *name, void *buffer, int count) {

    const int *values = buffer;

    // Declare UCI options for all array elements
    for (int i = 0; i < count; i++)
        printf("option name %s_mg_%d type spin default %d min %d max %d\n"
               "option name %s_eg_%d type spin default %d min %d max %d\n",
                name, i, MgScore(values[i]), -10000, 10000,
                name, i, EgScore(values[i]), -10000, 10000);

    // Remember array information for parsing
    strcpy(pe->name, name);
    pe->values = buffer;
    pe->count = count;
}

void TuneDeclareAll() {

    ParserEntry *pe = ParserEntries;

    // Piece values
    Declare(pe++, "Piece", &PieceTypeValue[1], 5);
    // PSQT
    Declare(pe++, "Ppsq", &PieceSqValue[PAWN][8], 48);
    Declare(pe++, "Npsq", PieceSqValue[KNIGHT], 64);
    Declare(pe++, "Bpsq", PieceSqValue[BISHOP], 64);
    Declare(pe++, "Rpsq", PieceSqValue[  ROOK], 64);
    Declare(pe++, "Qpsq", PieceSqValue[ QUEEN], 64);
    Declare(pe++, "Kpsq", PieceSqValue[  KING], 64);
    // Mobility
    Declare(pe++, "NMob", Mobility[KNIGHT-2],  9);
    Declare(pe++, "BMob", Mobility[BISHOP-2], 14);
    Declare(pe++, "RMob", Mobility[  ROOK-2], 15);
    Declare(pe++, "QMob", Mobility[ QUEEN-2], 28);
    // Open file
    Declare(pe++, "ROpen", &OpenFile[ ROOK-4], 1);
    Declare(pe++, "QOpen", &OpenFile[QUEEN-4], 1);
    // Semi-open file
    Declare(pe++, "RSemi", &SemiOpenFile[ ROOK-4], 1);
    Declare(pe++, "QSemi", &SemiOpenFile[QUEEN-4], 1);
    // Passed pawn
    Declare(pe++, "Passed", &PawnPassed[1], 6);
    // Misc
    Declare(pe++, "Doubled",  &PawnDoubled,   1);
    Declare(pe++, "Isolated", &PawnIsolated,   1);
    Declare(pe++, "BPair",    &BishopPair,     1);
    Declare(pe++, "KLDanger", &KingLineDanger, 1);

    assert(pe == &ParserEntries[PARSER_ENTRY_NB]);
}

void TuneParseAll(const char *line, int value) {

    char name[NAME_MAX_CHAR], phase[3];
    int idx;

    if (sscanf(line, "%[a-zA-Z]_%[a-zA-Z]_%d", name, phase, &idx) != 3)
        return;

    bool mg = strstr(phase, "mg");

    // Test against each Parser Entry, and set array element on match
    for (int i = 0; i < PARSER_ENTRY_NB; i++)
        if (!strcmp(name, ParserEntries[i].name)) {
            ParserEntries[i].values[idx] =
                mg ? S(value, EgScore(ParserEntries[i].values[idx]))
                   : S(MgScore(ParserEntries[i].values[idx]), value);
        }
}
#endif
