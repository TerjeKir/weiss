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
#include "types.h"


// Return the next best move
static int PickNextMove(MoveList *list, int ttMove) {

    if (list->next == list->count)
        return NOMOVE;

    int bestMove;
    int bestScore = 0;
    unsigned int moveNum = list->next++;
    unsigned int bestNum = moveNum;

    for (unsigned int i = moveNum; i < list->count; ++i)
        if (list->moves[i].score > bestScore) {
            bestScore = list->moves[i].score;
            bestNum   = i;
        }

    bestMove = list->moves[bestNum].move;
    list->moves[bestNum] = list->moves[moveNum];

    // Avoid returning the ttMove again
    if (bestMove == ttMove)
        return PickNextMove(list, ttMove);

    return bestMove;
}

// Returns the next move to try in a position
int NextMove(MovePicker *mp) {

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
                if ((move = PickNextMove(mp->list, mp->ttMove)))
                    return move;

            return NOMOVE;

        default:
            assert(0);
            return NOMOVE;
        }
}

// Init normal movepicker
void InitNormalMP(MovePicker *mp, MoveList *list, Position *pos, int ttMove) {
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