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

#include "fathom/tbprobe.h"
#include "bitboard.h"
#include "types.h"


// Calls fathom to probe syzygy tablebases - heavily inspired by ethereal
bool ProbeWDL(const Position *pos, int *score, int *bound) {

    // Don't probe at root, when en passant is possible, when castling is
    // possible, or when 50 move rule was not reset by the last move.
    // Finally, there is obviously no point if there are more pieces than
    // we have TBs for.
    if (   (pos->ply            == 0)
        || (pos->epSquare       != NO_SQ)
        || (pos->castlingRights != 0)
        || (pos->rule50         != 0)
        || ((unsigned)PopCount(pieceBB(ALL)) > TB_LARGEST))
        return false;

    // Call fathom
    unsigned tbresult = tb_probe_wdl(colorBB(WHITE),
                                     colorBB(BLACK),
                                     pieceBB(KING),
                                     pieceBB(QUEEN),
                                     pieceBB(ROOK),
                                     pieceBB(BISHOP),
                                     pieceBB(KNIGHT),
                                     pieceBB(PAWN),
                                     0,
                                     sideToMove);

    if (tbresult == TB_RESULT_FAILED)
        return false;

    *score = tbresult == TB_LOSS ? -TBWIN + pos->ply
           : tbresult == TB_WIN  ?  TBWIN - pos->ply
                                 :  0;

    *bound = tbresult == TB_LOSS ? BOUND_UPPER
           : tbresult == TB_WIN  ? BOUND_LOWER
                                 : BOUND_EXACT;

    return true;
}