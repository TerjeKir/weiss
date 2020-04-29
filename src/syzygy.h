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
#include "move.h"
#include "types.h"


// Calls fathom to probe syzygy tablebases
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

    *score = tbresult == TB_WIN  ?  TBWIN - pos->ply
           : tbresult == TB_LOSS ? -TBWIN + pos->ply
                                 :  0;

    *bound = tbresult == TB_WIN  ? BOUND_LOWER
           : tbresult == TB_LOSS ? BOUND_UPPER
                                 : BOUND_EXACT;

    return true;
}

// Calls fathom to get optimal moves in tablebase positions in root
bool RootProbe(Position *pos, SearchInfo *info) {

    if (pos->castlingRights || (unsigned)PopCount(pieceBB(ALL)) > TB_LARGEST)
        return false;

    unsigned result = tb_probe_root(
        colorBB(WHITE), colorBB(BLACK),
        pieceBB(KING), pieceBB(QUEEN),
        pieceBB(ROOK), pieceBB(BISHOP),
        pieceBB(KNIGHT), pieceBB(PAWN),
        pos->rule50, 0,
        pos->epSquare != NO_SQ ? pos->epSquare : 0,
        pos->stm, NULL);

    if (   result == TB_RESULT_FAILED
        || result == TB_RESULT_CHECKMATE
        || result == TB_RESULT_STALEMATE)
        return false;

    unsigned wdl, dtz, from, to, ep, promo;

    wdl = TB_GET_WDL(result);
    dtz = TB_GET_DTZ(result);

    int score = wdl == TB_WIN  ?  TBWIN - dtz
              : wdl == TB_LOSS ? -TBWIN + dtz
                               :  0;

    from  = TB_GET_FROM(result);
    to    = TB_GET_TO(result);
    promo = TB_GET_PROMOTES(result);

    Move move = MOVE(from, to, 0, promo ? 6 - promo : 0, 0);

    printf("info depth %d seldepth %d score cp %d "
           "time 0 nodes 0 nps 0 tbhits 1 pv %s\n",
           MAXDEPTH-1, MAXDEPTH-1, score, MoveToStr(move));
    fflush(stdout);

    info->bestMove = move;

    return true;
}