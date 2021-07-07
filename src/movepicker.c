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
    if (bestMove == mp->ttMove || bestMove == mp->kill1 || bestMove == mp->kill2)
        return PickNextMove(mp);

    return bestMove;
}

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
static void ScoreMoves(MoveList *list, const Thread *thread, const int stage, Depth depth) {

    const Position *pos = &thread->pos;

    for (int i = list->next; i < list->count; ++i) {
        Move move = list->moves[i].move;
        list->moves[i].score =
            stage == GEN_QUIET ? GetQuietHistory(thread, move)
                               : GetCaptureHistory(thread, move) + PieceValue[MG][pieceOn(toSq(move))];
    }

    SortMoves(list, -1000 * depth);
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
            ScoreMoves(&mp->list, mp->thread, GEN_NOISY, mp->depth);
            mp->stage++;

            // fall through
        case NOISY_GOOD:
            // Save seemingly bad noisy moves for later
            while ((move = PickNextMove(mp)))
                if (    mp->list.moves[mp->list.next-1].score > 12000
                    || (mp->list.moves[mp->list.next-1].score > -8000 && SEE(pos, move, 0)))
                    return move;
                else
                    mp->list.moves[mp->bads++].move = move;

            mp->stage++;

            // fall through
        case KILLER1:
            mp->stage++;
            if (   mp->kill1 != mp->ttMove
                && MoveIsPseudoLegal(pos, mp->kill1))
                return mp->kill1;

            // fall through
        case KILLER2:
            mp->stage++;
            if (   mp->kill2 != mp->ttMove
                && MoveIsPseudoLegal(pos, mp->kill2))
                return mp->kill2;

            // fall through
        case GEN_QUIET:
            if (!mp->onlyNoisy)
                GenQuietMoves(pos, &mp->list),
                ScoreMoves(&mp->list, mp->thread, GEN_QUIET, mp->depth);

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
void InitNormalMP(MovePicker *mp, Thread *thread, Depth depth, Move ttMove, Move kill1, Move kill2) {
    mp->list.count = mp->list.next = 0;
    mp->thread    = thread;
    mp->ttMove    = MoveIsPseudoLegal(&thread->pos, ttMove) ? ttMove : NOMOVE;
    mp->stage     = mp->ttMove ? TTMOVE : GEN_NOISY;
    mp->depth     = depth;
    mp->kill1     = kill1;
    mp->kill2     = kill2;
    mp->bads      = 0;
    mp->onlyNoisy = false;
}

// Init noisy movepicker
void InitNoisyMP(MovePicker *mp, Thread *thread) {
    InitNormalMP(mp, thread, 0, NOMOVE, NOMOVE, NOMOVE);
    mp->onlyNoisy = true;
}
