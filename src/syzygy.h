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

#include <stdio.h>

#include "fathom/tbprobe.h"
#include "bitboard.h"
#include "move.h"
#include "types.h"


// Converts a tbresult into a score
static int TBScore(const unsigned result, const int distance) {

    return result == TB_WIN  ?  TBWIN - distance
         : result == TB_LOSS ? -TBWIN + distance
                             :  0;
}

// Calls fathom to probe syzygy tablebases
bool ProbeWDL(const Position *pos, int *score, int *bound) {

    // Don't probe at root, when en passant is possible, when castling is
    // possible, or when 50 move rule was not reset by the last move.
    // Finally, there is obviously no point if there are more pieces than
    // we have TBs for.
    if (   !pos->ply
        ||  pos->epSquare
        ||  pos->castlingRights
        ||  pos->rule50
        || (unsigned)PopCount(pieceBB(ALL)) > TB_LARGEST)
        return false;

    // Call fathom
    unsigned result = tb_probe_wdl(
        colorBB(WHITE),  colorBB(BLACK),
        pieceBB(KING),   pieceBB(QUEEN),
        pieceBB(ROOK),   pieceBB(BISHOP),
        pieceBB(KNIGHT), pieceBB(PAWN),
        0, sideToMove);

    // Probe failed
    if (result == TB_RESULT_FAILED)
        return false;

    *score = TBScore(result, pos->ply);

    *bound = result == TB_WIN  ? BOUND_LOWER
           : result == TB_LOSS ? BOUND_UPPER
                               : BOUND_EXACT;

    return true;
}

// Calls fathom to get optimal moves in tablebase positions in root
bool RootProbe(Position *pos, Thread *thread) {

    // Tablebases contain no positions with castling legal,
    // and if there are too many pieces a probe will fail
    if (    pos->castlingRights
        || (unsigned)PopCount(pieceBB(ALL)) > TB_LARGEST)
        return false;

    // Call fathom
    unsigned result = tb_probe_root(
        colorBB(WHITE),  colorBB(BLACK),
        pieceBB(KING),   pieceBB(QUEEN),
        pieceBB(ROOK),   pieceBB(BISHOP),
        pieceBB(KNIGHT), pieceBB(PAWN),
        pos->rule50, pos->epSquare, sideToMove);

    // Probe failed
    if (   result == TB_RESULT_FAILED
        || result == TB_RESULT_CHECKMATE
        || result == TB_RESULT_STALEMATE)
        return false;

    unsigned wdl, dtz, from, to, promo;

    // Extract information
    wdl   = TB_GET_WDL(result);
    dtz   = TB_GET_DTZ(result);
    from  = TB_GET_FROM(result);
    to    = TB_GET_TO(result);
    promo = TB_GET_PROMOTES(result);

    // Calculate score to print
    int score = TBScore(wdl, dtz);

    // Construct the move to print, ignoring capture and
    // flag fields as they aren't needed for printing
    Move move = MOVE(from, to, 0, promo ? 6 - promo : 0, 0);

    // Print thinking info
    printf("info depth %d seldepth %d score cp %d "
           "time 0 nodes 0 nps 0 tbhits 1 pv %s\n",
           MAXDEPTH-1, MAXDEPTH-1, score, MoveToStr(move));
    fflush(stdout);

    // Set move to be printed as conclusion
    thread->bestMove = move;

    return true;
}
