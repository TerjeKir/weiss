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

#include "board.h"
#include "history.h"
#include "move.h"
#include "movepicker.h"


// Return the next best move
static Move PickNextMove(MovePicker *mp) {

    MoveList *list = &mp->list;

    if (list->next == list->count)
        return NOMOVE;

    Move bestMove = list->moves[list->next++].move;

    // Avoid returning the TT or killer moves again
    if (bestMove == mp->ttMove || bestMove == mp->killer)
        return PickNextMove(mp);

    return bestMove;
}

// Partial insertion sort
static void SortMoves(MoveList *list, int threshold) {

    MoveListEntry *begin = &list->moves[list->next];
    MoveListEntry *end = &list->moves[list->count];

    for (MoveListEntry *sortedEnd = begin, *p = begin + 1; p < end; ++p) {
        if (p->score > threshold) {
            MoveListEntry tmp = *p, *q;
            *p = *++sortedEnd;
            for (q = sortedEnd; q != begin && (q-1)->score < tmp.score; --q)
                *q = *(q - 1);
            *q = tmp;
        }
    }
}

// Gives a score to each move left in the list
static void ScoreMoves(MovePicker *mp, const int stage) {

    const Thread *thread = mp->thread;
    MoveList *list = &mp->list;

    for (int i = list->next; i < list->count; ++i) {
        Move move = list->moves[i].move;
        list->moves[i].score =
            stage == GEN_QUIET ? GetQuietHistory(thread, mp->ss, move)
                               : GetCaptureHistory(thread, move) + PieceValue[MG][capturing(move)];
    }

    SortMoves(list, -750 * mp->depth);
}

// Returns the next move to try in a position
Move NextMove(MovePicker *mp) {

    Move move;
    Position *pos = &mp->thread->pos;

    // Switch on stage, falls through to the next stage
    // if a move isn't returned in the current stage.
    switch (mp->stage) {

        case TTMOVE:
            mp->stage++;
            return mp->ttMove;

            // fall through
        case GEN_NOISY:
            GenNoisyMoves(pos, &mp->list);
            ScoreMoves(mp, GEN_NOISY);
            mp->stage++;

            // fall through
        case NOISY_GOOD:
            // Save seemingly bad noisy moves for later
            while ((move = PickNextMove(mp)))
                if (    mp->list.moves[mp->list.next-1].score >  11046
                    || (mp->list.moves[mp->list.next-1].score > - 9543 && SEE(pos, move, mp->threshold)))
                    return move;
                else
                    mp->list.moves[mp->bads++].move = move;

            mp->stage++;

            // fall through
        case KILLER:
            mp->stage++;
            if (   mp->killer != mp->ttMove
                && MoveIsPseudoLegal(pos, mp->killer))
                return mp->killer;

            // fall through
        case GEN_QUIET:
            if (!mp->onlyNoisy)
                GenQuietMoves(pos, &mp->list),
                ScoreMoves(mp, GEN_QUIET);

            mp->stage++;

            // fall through
        case QUIET:
            if (!mp->onlyNoisy)
                if ((move = PickNextMove(mp)))
                    return move;

            mp->stage++;
            mp->list.next = 0;
            mp->list.moves[mp->bads].move = NOMOVE;

            // fall through
        case NOISY_BAD:
            return mp->list.moves[mp->list.next++].move;

        default:
            assert(0);
            return NOMOVE;
    }
}

// Init normal movepicker
void InitNormalMP(MovePicker *mp, Thread *thread, Stack *ss, Depth depth, Move ttMove, Move killer) {
    mp->list.count = mp->list.next = 0;
    mp->thread    = thread;
    mp->ss        = ss;
    mp->ttMove    = ttMove;
    mp->stage     = ttMove ? TTMOVE : GEN_NOISY;
    mp->depth     = depth;
    mp->killer    = killer;
    mp->bads      = 0;
    mp->threshold = 0;
    mp->onlyNoisy = false;
}

// Init noisy movepicker
void InitNoisyMP(MovePicker *mp, Thread *thread, Stack *ss, Move ttMove) {
    InitNormalMP(mp, thread, ss, 0, ttMove, NOMOVE);
    mp->onlyNoisy = true;
}

void InitProbcutMP(MovePicker *mp, Thread *thread, Stack *ss, int threshold) {
    InitNoisyMP(mp, thread, ss, NOMOVE);
    mp->threshold = threshold;
}
