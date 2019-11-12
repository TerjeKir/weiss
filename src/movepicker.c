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

	bestMove = list->moves[bestNum].move;
	list->moves[bestNum] = list->moves[moveNum];

    // Avoid returning the ttMove again
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