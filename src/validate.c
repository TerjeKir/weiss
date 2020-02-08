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

#include "board.h"
#include "move.h"

#ifndef NDEBUG

bool ValidSquare(const int sq) {
    return A1 <= sq
        && sq <= H8;
}

bool ValidSide(const int side) {
    return side == WHITE
        || side == BLACK;
}

bool ValidPiece(const int piece) {
    return (wP <= piece && piece <= wK)
        || (bP <= piece && piece <= bK);
}

bool MoveListOk(const MoveList *list, const Position *pos) {

    if (list->count >= MAXPOSITIONMOVES)
        return false;

    for (unsigned int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

        if (!MoveIsPseudoLegal(pos, list->moves[MoveNum].move)) {
            PrintBoard(pos);
            return false;
        }
    }

    return true;
}
#endif