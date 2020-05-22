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


static int MvvLvaScores[PIECE_NB][PIECE_NB];


// Initializes the MostValuableVictim-LeastValuableAttacker scores used for ordering captures
CONSTR InitMvvLva() {

    const int VictimScore[PIECE_NB]   = {0, 106, 206, 306, 406, 506, 606, 0, 0, 106, 206, 306, 406, 506, 606, 0};
    const int AttackerScore[PIECE_NB] = {0,   1,   2,   3,   4,   5,   6, 0, 0,   1,   2,   3,   4,   5,   6, 0};

    for (Piece Attacker = EMPTY; Attacker < PIECE_NB; ++Attacker)
        for (Piece Victim = EMPTY; Victim < PIECE_NB; ++Victim)
            MvvLvaScores[Victim][Attacker] = VictimScore[Victim] - AttackerScore[Attacker];
}

// Return the next best move
static Move PickNextMove(MoveList *list, const Move ttMove, const Move kill1, const Move kill2) {

    if (list->next == list->count)
        return NOMOVE;

    int bestIdx = list->next;
    int bestScore = list->moves[bestIdx].score;

    for (int i = list->next + 1; i < list->count; ++i)
        if (list->moves[i].score > bestScore)
            bestScore = list->moves[i].score,
            bestIdx = i;

    Move bestMove = list->moves[bestIdx].move;
    list->moves[bestIdx] = list->moves[list->next++];

    // Avoid returning the TT or killer moves again
    if (bestMove == ttMove || bestMove == kill1 || bestMove == kill2)
        return PickNextMove(list, ttMove, kill1, kill2);

    return bestMove;
}

// Gives a score to each move left in the list
static void ScoreMoves(MoveList *list, const Thread *thread, const int stage) {

    const Position *pos = &thread->pos;

    for (int i = list->next; i < list->count; ++i) {

        Move move = list->moves[i].move;

        if (stage == GEN_NOISY)
            list->moves[i].score = moveIsEnPas(move) ? 105
                                 : MvvLvaScores[pieceOn(toSq(move))][pieceOn(fromSq(move))];

        if (stage == GEN_QUIET)
            list->moves[i].score = thread->history[pieceOn(fromSq(move))][toSq(move)];
    }
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
            GenNoisyMoves(pos, mp->list);
            ScoreMoves(mp->list, mp->thread, GEN_NOISY);
            mp->stage++;

            // fall through
        case NOISY:
            if ((move = PickNextMove(mp->list, mp->ttMove, NOMOVE, NOMOVE)))
                return move;

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
            if (mp->onlyNoisy)
                return NOMOVE;

            GenQuietMoves(pos, mp->list);
            ScoreMoves(mp->list, mp->thread, GEN_QUIET);
            mp->stage++;

            // fall through
        case QUIET:
            return PickNextMove(mp->list, mp->ttMove, mp->kill1, mp->kill2);

        default:
            assert(0);
            return NOMOVE;
        }
}

// Init normal movepicker
void InitNormalMP(MovePicker *mp, MoveList *list, Thread *thread, Move ttMove, Move kill1, Move kill2) {
    list->count   = list->next = 0;
    mp->list      = list;
    mp->thread    = thread;
    mp->ttMove    = MoveIsPseudoLegal(&thread->pos, ttMove) ? ttMove : NOMOVE;
    mp->stage     = mp->ttMove ? TTMOVE : GEN_NOISY;
    mp->kill1     = kill1;
    mp->kill2     = kill2;
    mp->onlyNoisy = false;
}

// Init noisy movepicker
void InitNoisyMP(MovePicker *mp, MoveList *list, Thread *thread) {
    list->count   = list->next = 0;
    mp->list      = list;
    mp->thread    = thread;
    mp->ttMove    = NOMOVE;
    mp->kill1     = NOMOVE;
    mp->kill2     = NOMOVE;
    mp->stage     = GEN_NOISY;
    mp->onlyNoisy = true;
}
