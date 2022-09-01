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

#pragma once

#include <stdlib.h>

#include "board.h"
#include "move.h"
#include "threads.h"
#include "types.h"


#define QuietEntry(move)        (&thread->history[sideToMove][fromSq(move)][toSq(move)])
#define NoisyEntry(move)        (&thread->captureHistory[piece(move)][toSq(move)][PieceTypeOf(capturing(move))])
#define ContEntry(offset, move) (&(*(ss-offset)->continuation)[piece(move)][toSq(move)])

#define QuietHistoryUpdate(move, bonus)        (HistoryBonus(QuietEntry(move),        bonus,  8192))
#define NoisyHistoryUpdate(move, bonus)        (HistoryBonus(NoisyEntry(move),        bonus, 16384))
#define ContHistoryUpdate(offset, move, bonus) (HistoryBonus(ContEntry(offset, move), bonus, 16384))


INLINE void HistoryBonus(int16_t *cur, int bonus, int div) {
    *cur += bonus - *cur * abs(bonus) / div;
}

INLINE int Bonus(Depth depth) {
    return MIN(2100, 350 * depth - 350);
}

// Updates various history heuristics when a move causes a beta cutoff
INLINE void UpdateQuietHistory(Thread *thread, Stack *ss, Move bestMove, int bonus, Depth depth, Move quiets[], int qCount) {

    const Position *pos = &thread->pos;

    // Update killers
    if (ss->killers[0] != bestMove) {
        ss->killers[1] = ss->killers[0];
        ss->killers[0] = bestMove;
    }

    // Bonus to the move that caused the beta cutoff
    if (depth > 2) {
        QuietHistoryUpdate(bestMove, bonus);
        ContHistoryUpdate(1, bestMove, bonus);
        ContHistoryUpdate(2, bestMove, bonus);
        ContHistoryUpdate(4, bestMove, bonus);
    }

    // Penalize quiet moves that failed to produce a cut
    for (Move *move = quiets; move < quiets + qCount; ++move) {
        QuietHistoryUpdate(*move, -bonus);
        ContHistoryUpdate(1, *move, -bonus);
        ContHistoryUpdate(2, *move, -bonus);
        ContHistoryUpdate(4, *move, -bonus);
    }
}

// Updates various history heuristics when a move causes a beta cutoff
INLINE void UpdateHistory(Thread *thread, Stack *ss, Move bestMove, Depth depth, Move quiets[], int qCount, Move noisys[], int nCount) {

    int bonus = Bonus(depth);

    // Update quiet history if bestMove is quiet
    if (moveIsQuiet(bestMove))
        UpdateQuietHistory(thread, ss, bestMove, bonus, depth, quiets, qCount);

    // Bonus to the move that caused the beta cutoff
    if (depth > 2 && !moveIsQuiet(bestMove))
        NoisyHistoryUpdate(bestMove, bonus);

    // Penalize noisy moves that failed to produce a cut
    for (Move *move = noisys; move < noisys + nCount; ++move)
        NoisyHistoryUpdate(*move, -bonus);
}

INLINE int GetQuietHistory(const Thread *thread, Stack *ss, Move move) {

    const Position *pos = &thread->pos;

    int cmh = *ContEntry(1, move);
    int fmh = *ContEntry(2, move);
    int amh = *ContEntry(4, move);

    return *QuietEntry(move) + cmh + fmh + amh;
}

INLINE int GetCaptureHistory(const Thread *thread, Move move) {
    return *NoisyEntry(move);
}

INLINE int GetHistory(const Thread *thread, Stack *ss, Move move) {
    return moveIsQuiet(move) ? GetQuietHistory(thread, ss, move) : GetCaptureHistory(thread, move);
}
