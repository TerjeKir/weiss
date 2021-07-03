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

#include <stdlib.h>

#include "board.h"
#include "move.h"
#include "threads.h"
#include "types.h"


#define QuietEntry(move) &thread->history[sideToMove][fromSq(move)][toSq(move)]
#define NoisyEntry(move) &thread->captureHistory[pieceOn(fromSq(move))][toSq(move)][PieceTypeOf(capturing(move))]
#define ContEntry(move) &thread->continuation[prevPiece][prevTo][pieceOn(fromSq(move))][toSq(move)]


INLINE void HistoryBonus(int16_t *cur, int bonus) {
    *cur += 32 * bonus - *cur * abs(bonus) / 512;
}

// Updates various history heuristics when a move causes a beta cutoff
INLINE void UpdateHistory(Thread *thread, Stack *ss, Move bestMove, Depth depth, Move quiets[], int qCount, Move noisys[], int nCount) {

    const Position *pos = &thread->pos;

    Move prevMove = pos->histPly > 0 ? history(-1).move : NOMOVE;
    Square prevTo = toSq(prevMove);
    Piece prevPiece = pieceOn(prevTo);

    // Update killers
    if (moveIsQuiet(bestMove) && ss->killers[0] != bestMove) {
        ss->killers[1] = ss->killers[0];
        ss->killers[0] = bestMove;
    }

    int bonus = depth * depth;

    // Bonus to the move that caused the beta cutoff
    if (depth > 1) {
        HistoryBonus(moveIsQuiet(bestMove) ? QuietEntry(bestMove) : NoisyEntry(bestMove), bonus);
        if (prevMove && moveIsQuiet(bestMove))
            HistoryBonus(ContEntry(bestMove), bonus);
    }

    // If bestMove is quiet, penalize quiet moves that failed to produce a cut
    if (moveIsQuiet(bestMove))
        for (int i = 0; i < qCount; ++i) {
            Move move = quiets[i];
            if (move == bestMove) continue;
            HistoryBonus(QuietEntry(move), -bonus);
            if (prevMove)
                HistoryBonus(ContEntry(move), -bonus);
        }

    // Penalize noisy moves that failed to produce a cut
    for (int i = 0; i < nCount; ++i) {
        Move move = noisys[i];
        if (move == bestMove) continue;
        HistoryBonus(NoisyEntry(move), -bonus);
    }
}
