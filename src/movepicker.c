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

#include "move.h"
#include "movegen.h"
#include "movepicker.h"


// Return the next best move
static Move PickNextMove(MoveList *list, const Move ttMove) {

    int bestIdx = list->next;
    int bestScore = list->moves[bestIdx].score;

    for (int i = list->next + 1; i < list->count; ++i)
        if (list->moves[i].score > bestScore)
            bestScore = list->moves[i].score,
            bestIdx = i;

    Move bestMove = list->moves[bestIdx].move;
    list->moves[bestIdx] = list->moves[list->next++];

    // Avoid returning the ttMove again
    if (bestMove == ttMove)
        return list->next < list->count ? PickNextMove(list, ttMove)
                                        : NOMOVE;

    return bestMove;
}

// Returns the next move to try in a position
Move NextMove(MovePicker *mp) {

    Move move;

    // Switch on stage, falls through to the next stage
    // if a move isn't returned in the current stage.
    switch (mp->stage) {

        case TTMOVE:
            mp->stage++;
            if (MoveIsPseudoLegal(mp->pos, mp->ttMove))
                return mp->ttMove;

            // fall through
        case GEN_NOISY:
            GenNoisyMoves(mp->pos, mp->list);
            mp->stage++;

            // fall through
        case NOISY:
            if (mp->list->next < mp->list->count)
                if ((move = PickNextMove(mp->list, mp->ttMove)))
                    return move;

            mp->stage++;

            // fall through
        case GEN_QUIET:
            if (mp->onlyNoisy)
                return NOMOVE;

            GenQuietMoves(mp->pos, mp->list);
            mp->stage++;

            // fall through
        case QUIET:
            if (mp->list->next < mp->list->count)
                return PickNextMove(mp->list, mp->ttMove);

            return NOMOVE;

        default:
            assert(0);
            return NOMOVE;
        }
}

// Init normal movepicker
void InitNormalMP(MovePicker *mp, MoveList *list, Position *pos, Move ttMove) {
    list->count   = list->next = 0;
    mp->list      = list;
    mp->onlyNoisy = false;
    mp->pos       = pos;
    mp->stage     = TTMOVE;
    mp->ttMove    = ttMove;
}

// Init noisy movepicker
void InitNoisyMP(MovePicker *mp, MoveList *list, Position *pos) {
    list->count   = list->next = 0;
    mp->list      = list;
    mp->onlyNoisy = true;
    mp->pos       = pos;
    mp->stage     = GEN_NOISY;
    mp->ttMove    = NOMOVE;
}