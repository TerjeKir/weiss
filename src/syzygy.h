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

#include "onlinesyzygy/onlinesyzygy.h"
#include "pyrrhic/tbprobe.h"
#include "bitboard.h"
#include "move.h"
#include "transposition.h"
#include "types.h"


// Converts a tbresult into a score
static int TBScore(const unsigned result, const int distance) {
    return result == TB_WIN  ?  TBWIN - distance
         : result == TB_LOSS ? -TBWIN + distance
                             :  0;
}

// Probe local Syzygy files using Pyrrhic to get score
bool ProbeWDL(const Position *pos, int *score, int *bound, int ply) {

    // Don't probe at root, when castling is possible, or when 50 move rule
    // was not reset by the last move. Finally, there is obviously no point
    // if there are more pieces than we have TBs for.
    if (  !ply
        || pos->castlingRights
        || pos->rule50
        || PopCount(pieceBB(ALL)) > TB_LARGEST)
        return false;

    // Call Pyrrhic
    unsigned result = tb_probe_wdl(
        colorBB(WHITE),  colorBB(BLACK),
        pieceBB(KING),   pieceBB(QUEEN),
        pieceBB(ROOK),   pieceBB(BISHOP),
        pieceBB(KNIGHT), pieceBB(PAWN),
        pos->epSquare, sideToMove);

    // Probe failed
    if (result == TB_RESULT_FAILED)
        return false;

    *score = TBScore(result, ply);

    *bound = result == TB_WIN  ? BOUND_LOWER
           : result == TB_LOSS ? BOUND_UPPER
                               : BOUND_EXACT;

    return true;
}

// Probe local Syzygy files using Pyrrhic to get an optimal move
bool ProbeRoot(Position *pos, Move *move, unsigned *wdl, unsigned *dtz) {

    // Call Pyrrhic
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

    // Extract information
    unsigned from  = TB_GET_FROM(result),
             to    = TB_GET_TO(result),
             promo = TB_GET_PROMOTES(result);

    *move = MOVE(from, to, 0, promo ? 6 - promo : 0, 0);
    *wdl = TB_GET_WDL(result);
    *dtz = TB_GET_DTZ(result);

    return true;
}

// Get optimal move from Syzygy tablebases
bool SyzygyMove(Position *pos) {

    Move move;
    unsigned wdl, dtz;
    int pieces = PopCount(pieceBB(ALL));

    // Probe local or online Syzygy if possible
    bool success =
          pos->castlingRights         ? false
        : pieces <= TB_LARGEST        ? ProbeRoot(pos, &move, &wdl, &dtz)
        : onlineSyzygy && pieces <= 7 ? QueryRoot(pos, &move, &wdl, &dtz)
                                      : false;

    if (!success) return false;

    // Print thinking info
    printf("info depth %d seldepth %d score cp %d "
           "time 0 nodes 0 nps 0 tbhits 1 pv %s\n",
           MAX_PLY, MAX_PLY, TBScore(wdl, dtz), MoveToStr(move));
    fflush(stdout);

    // Set move to be printed as conclusion
    threads->rootMoves[0].move = move;

    return true;
}
