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

#pragma once

#include <stdlib.h>

#include "board.h"
#include "move.h"
#include "threads.h"
#include "types.h"


#define QuietEntry(move)        (&thread->history[thread->pos.stm][fromSq(move)][toSq(move)])
#define PawnEntry(move)         (&thread->pawnHistory[PawnStructure(&thread->pos)][piece(move)][toSq(move)])
#define NoisyEntry(move)        (&thread->captureHistory[piece(move)][toSq(move)][PieceTypeOf(capturing(move))])
#define ContEntry(offset, move) (&(*(ss-offset)->continuation)[piece(move)][toSq(move)])
#define PawnCorrEntry()         (&thread->pawnCorrHistory[thread->pos.stm][PawnCorrIndex(&thread->pos)])
#define MinorCorrEntry()        (&thread->minorCorrHistory[thread->pos.stm][MinorCorrIndex(&thread->pos)])
#define MajorCorrEntry()        (&thread->majorCorrHistory[thread->pos.stm][MajorCorrIndex(&thread->pos)])
#define ContCorrEntry(offset)   (&(*(ss-offset)->contCorr)[piece((ss-1)->move)][toSq((ss-1)->move)])
#define NonPawnCorrEntry(color) (&thread->nonPawnCorrHistory[color][thread->pos.stm][NonPawnCorrIndex(&thread->pos, color)])

#define QuietHistoryUpdate(move, bonus)        (HistoryBonus(QuietEntry(move),        bonus,  4390))
#define PawnHistoryUpdate(move, bonus)         (HistoryBonus(PawnEntry(move),         bonus,  9410))
#define NoisyHistoryUpdate(move, bonus)        (HistoryBonus(NoisyEntry(move),        bonus, 13875))
#define ContHistoryUpdate(offset, move, bonus) (HistoryBonus(ContEntry(offset, move), bonus, 17850))
#define PawnCorrHistoryUpdate(bonus)           (HistoryBonus(PawnCorrEntry(),         bonus,  1670))
#define MinorCorrHistoryUpdate(bonus)          (HistoryBonus(MinorCorrEntry(),        bonus,  1190))
#define MajorCorrHistoryUpdate(bonus)          (HistoryBonus(MajorCorrEntry(),        bonus,  1070))
#define ContCorrHistoryUpdate(offset, bonus)   (HistoryBonus(ContCorrEntry(offset),   bonus,  1335))
#define NonPawnCorrHistoryUpdate(bonus, color) (HistoryBonus(NonPawnCorrEntry(color), bonus,  1165))


INLINE int PawnStructure(const Position *pos) { return pos->pawnKey & (PAWN_HISTORY_SIZE - 1); }
INLINE int PawnCorrIndex(const Position *pos) { return pos->pawnKey & (CORRECTION_HISTORY_SIZE - 1); }
INLINE int MinorCorrIndex(const Position *pos) { return pos->minorKey & (CORRECTION_HISTORY_SIZE - 1); }
INLINE int MajorCorrIndex(const Position *pos) { return pos->majorKey & (CORRECTION_HISTORY_SIZE - 1); }
INLINE int NonPawnCorrIndex(const Position *pos, Color c) { return pos->nonPawnKey[c] & (CORRECTION_HISTORY_SIZE - 1); }


INLINE void HistoryBonus(int16_t *entry, int bonus, int div) {
    assert(abs(bonus) <= div);
    *entry += bonus - *entry * abs(bonus) / div;
    assert(abs(*entry) <= div);
}

INLINE int Bonus(Depth depth) {
    return MIN(2405, 253 * depth - 271);
}

INLINE int Malus(Depth depth) {
    return -MIN(712, 520 * depth - 142);
}

INLINE int CorrectionBonus(int score, int eval, Depth depth) {
    return CLAMP((score - eval) * depth / 4, -175, 319);
}

INLINE void UpdateContHistories(Stack *ss, Move move, int bonus) {
    ContHistoryUpdate(1, move, bonus);
    ContHistoryUpdate(2, move, bonus);
    ContHistoryUpdate(4, move, bonus);
}

// Updates history heuristics when a quiet move is the best move
INLINE void UpdateQuietHistory(Thread *thread, Stack *ss, Move bestMove, Depth depth, Move quiets[], int qCount) {

    int bonus = Bonus(depth);
    int malus = Malus(depth);

    // Update killer
    ss->killer = bestMove;

    // Bonus to the move that caused the beta cutoff
    if (depth > 2) {
        QuietHistoryUpdate(bestMove, bonus);
        PawnHistoryUpdate(bestMove, bonus);
        UpdateContHistories(ss, bestMove, bonus);
    }

    // Penalize quiet moves that failed to produce a cut
    for (Move *move = quiets; move < quiets + qCount; ++move) {
        QuietHistoryUpdate(*move, malus);
        PawnHistoryUpdate(*move, malus);
        UpdateContHistories(ss, *move, malus);
    }
}

// Updates history heuristics
INLINE void UpdateHistory(Thread *thread, Stack *ss, Move bestMove, Depth depth, Move quiets[], int qCount, Move noisys[], int nCount) {

    int bonus = Bonus(depth);
    int malus = Malus(depth);

    // Update quiet history if bestMove is quiet
    if (moveIsQuiet(bestMove))
        UpdateQuietHistory(thread, ss, bestMove, depth, quiets, qCount);

    // Bonus to the move that caused the beta cutoff
    if (depth > 2 && !moveIsQuiet(bestMove))
        NoisyHistoryUpdate(bestMove, bonus);

    // Penalize noisy moves that failed to produce a cut
    for (Move *move = noisys; move < noisys + nCount; ++move)
        NoisyHistoryUpdate(*move, malus);
}

INLINE void UpdateCorrectionHistory(Thread *thread, Stack *ss, int bestScore, int eval, Depth depth) {
    int bonus = CorrectionBonus(bestScore, eval, depth);
    PawnCorrHistoryUpdate(bonus);
    MinorCorrHistoryUpdate(bonus);
    MajorCorrHistoryUpdate(bonus);
    NonPawnCorrHistoryUpdate(bonus, WHITE);
    NonPawnCorrHistoryUpdate(bonus, BLACK);
    ContCorrHistoryUpdate(2, bonus);
    ContCorrHistoryUpdate(3, bonus);
    ContCorrHistoryUpdate(4, bonus);
    ContCorrHistoryUpdate(5, bonus);
    ContCorrHistoryUpdate(6, bonus);
    ContCorrHistoryUpdate(7, bonus);
}

INLINE int GetQuietHistory(const Thread *thread, Stack *ss, Move move) {
    return  *QuietEntry(move)
          + *PawnEntry(move)
          + *ContEntry(1, move)
          + *ContEntry(2, move)
          + *ContEntry(4, move);
}

INLINE int GetCaptureHistory(const Thread *thread, Move move) {
    return *NoisyEntry(move);
}

INLINE int GetHistory(const Thread *thread, Stack *ss, Move move) {
    return moveIsQuiet(move) ? GetQuietHistory(thread, ss, move) : GetCaptureHistory(thread, move);
}

INLINE int GetCorrectionHistory(const Thread *thread, const Stack *ss) {
    int c =  6088 * *PawnCorrEntry()
           + 6904 * *MinorCorrEntry()
           + 4292 * *MajorCorrEntry()
           + 7228 * (*NonPawnCorrEntry(WHITE) + *NonPawnCorrEntry(BLACK))
           + 3475 * *ContCorrEntry(2)
           + 3211 * *ContCorrEntry(3)
           + 2385 * *ContCorrEntry(4)
           + 3590 * *ContCorrEntry(5)
           + 3188 * *ContCorrEntry(6)
           + 2787 * *ContCorrEntry(7);

    return c / 131072;
}
