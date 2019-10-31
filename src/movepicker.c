// movepicker.c

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

	assert(moveNum < list->count);
	assert(bestNum < list->count);
	assert(bestNum >= moveNum);

	bestMove = list->moves[bestNum].move;
	list->moves[bestNum] = list->moves[moveNum];

	if (bestMove == ttMove)
		return PickNextMove(list, ttMove);

	return bestMove;
}

// Returns the next move to try in a position
int NextMove(MovePicker *mp) {

    int move;

    // Switch on stage, falls through to the next stage
    // if a move isn't returned in the current stage.
    switch (mp->stage) {

        case TTMOVE:
            mp->stage++;
            if (MoveIsPsuedoLegal(mp->pos, mp->ttMove))
                return mp->ttMove;

            // fall through
        case GEN_NOISY:
            GenNoisyMoves(mp->pos, mp->list);
            mp->stage++;

            // fall through
        case NOISY:
            if (mp->list->next < mp->list->count) {
                move = PickNextMove(mp->list, mp->ttMove);
                if (move)
                    return move;
            }
            mp->stage++;

            // fall through
        case GEN_QUIET:
            if (mp->onlyNoisy) {
                mp->stage = DONE;
                return NOMOVE;
            }
            GenQuietMoves(mp->pos, mp->list);
            mp->stage++;

            // fall through
        case QUIET:

            if (mp->list->next < mp->list->count) {
                move = PickNextMove(mp->list, mp->ttMove);
                if (move)
                    return move;
            }
            mp->stage = DONE;
            return NOMOVE;

        default:
            assert(0);
            return NOMOVE;
        }
}