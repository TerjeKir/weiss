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
#define ContEntry(prev, move) &thread->continuation[pieceOn(toSq(prev))][toSq(prev)][pieceOn(fromSq(move))][toSq(move)]


INLINE void HistoryBonus(int16_t *cur, int bonus) {
    *cur += 32 * bonus - *cur * abs(bonus) / 512;
}

// Updates various history heuristics when a move causes a beta cutoff
INLINE void UpdateQuietHistory(Thread *thread, Stack *ss, Move bestMove, Depth depth, Move quiets[], int qCount) {

    const Position *pos = &thread->pos;

    Move prevMove  = pos->histPly >= 1 ? history(-1).move : NOMOVE;
    Move prevMove2 = pos->histPly >= 2 ? history(-2).move : NOMOVE;

    // Update killers
    if (ss->killers[0] != bestMove) {
        ss->killers[1] = ss->killers[0];
        ss->killers[0] = bestMove;
    }

    int bonus = depth * depth;

    // Bonus to the move that caused the beta cutoff
    if (depth > 2) {
        HistoryBonus(QuietEntry(bestMove), bonus);
        if (prevMove)
            HistoryBonus(ContEntry(prevMove, bestMove), bonus);
        if (prevMove2)
            HistoryBonus(ContEntry(prevMove2, bestMove), bonus);
    }

    // Penalize quiet moves that failed to produce a cut
    for (int i = 0; i < qCount; ++i) {
        Move move = quiets[i];
        HistoryBonus(QuietEntry(move), -bonus);
        if (prevMove)
            HistoryBonus(ContEntry(prevMove, move), -bonus);
        if (prevMove2)
            HistoryBonus(ContEntry(prevMove2, move), -bonus);
    }
}

// Updates various history heuristics when a move causes a beta cutoff
INLINE void UpdateHistory(Thread *thread, Stack *ss, Move bestMove, Depth depth, Move quiets[], int qCount, Move noisys[], int nCount) {

    const Position *pos = &thread->pos;

    int bonus = depth * depth;

    // Update quiet history if bestMove is quiet
    if (moveIsQuiet(bestMove))
        UpdateQuietHistory(thread, ss, bestMove, depth, quiets, qCount);

    // Bonus to the move that caused the beta cutoff
    if (depth > 2 && !moveIsQuiet(bestMove))
        HistoryBonus(NoisyEntry(bestMove), bonus);

    // Penalize noisy moves that failed to produce a cut
    for (int i = 0; i < nCount; ++i)
        HistoryBonus(NoisyEntry(noisys[i]), -bonus);
}

INLINE int GetQuietHistory(const Thread *thread, Move move) {

    const Position *pos = &thread->pos;

    Move prevMove  = pos->histPly >= 1 ? history(-1).move : NOMOVE;
    Move prevMove2 = pos->histPly >= 2 ? history(-2).move : NOMOVE;

    int cmh = prevMove  ? *ContEntry(prevMove,  move) : 0;
    int fmh = prevMove2 ? *ContEntry(prevMove2, move) : 0;

    return *QuietEntry(move) + cmh + fmh;
}

INLINE int GetCaptureHistory(const Thread *thread, Move move) {

    const Position *pos = &thread->pos;

    return *NoisyEntry(move);
}

INLINE int GetHistory(const Thread *thread, Move move) {
    return moveIsQuiet(move) ? GetQuietHistory(thread, move) : GetCaptureHistory(thread, move);
}
