// search.c

#include <stdlib.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "attack.h"
#include "board.h"
#include "evaluate.h"
#include "io.h"
#include "makemove.h"
#include "misc.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"
#include "syzygy.h"


// Check if time up, or interrupt from GUI
static void CheckTime(S_SEARCHINFO *info) {

	if (  (info->nodes & 8192) == 0
		&& info->timeset 
		&& GetTimeMs() > info->stoptime) {
		info->stopped = true;
	}
}

// Move best move to the front of the queue
static int PickNextMove(S_MOVELIST *list) {

	int bestMove;
	int bestScore = 0;
	unsigned int moveNum = list->next++;
	unsigned int bestNum = moveNum;

	for (unsigned int index = moveNum; index < list->count; ++index)
		if (list->moves[index].score > bestScore) {
			bestScore = list->moves[index].score;
			bestNum = index;
		}

	assert(moveNum < list->count);
	assert(bestNum < list->count);
	assert(bestNum >= moveNum);

	bestMove = list->moves[bestNum].move;

	// Stable, slower
	memmove(&list->moves[list->next], &list->moves[moveNum], (bestNum - moveNum) * sizeof(S_MOVE));

	// Unstable, faster
	// list->moves[bestNum] = list->moves[moveNum];

	return bestMove;
}

// Checks whether position has already occurred
static int IsRepetition(const S_BOARD *pos) {

	for (int index = pos->hisPly - pos->fiftyMove; index < pos->hisPly - 1; ++index) {

		assert(index >= 0 && index < MAXGAMEMOVES);
		if (pos->posKey == pos->history[index].posKey)
			return true;
	}

	return false;
}

// Get ready to start a search
static void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info) {

	int i, j;

	for (i = 0; i < PIECE_NB; ++i)
		for (j = A1; j <= H8; ++j)
			pos->searchHistory[i][j] = 0;

	for (i = 0; i < 2; ++i)
		for (j = 0; j < MAXDEPTH; ++j)
			pos->searchKillers[i][j] = 0;

#ifdef SEARCH_STATS
	pos->hashTable->hit = 0;
	pos->hashTable->cut = 0;
	pos->hashTable->overWrite = 0;

	info->fh = 0;
	info->fhf = 0;
#endif
	pos->ply = 0;
	info->nodes = 0;
	info->tbhits = 0;
	info->stopped = 0;
	info->seldepth = 0;
}

// Quiescence
static int Quiescence(int alpha, const int beta, S_BOARD *pos, S_SEARCHINFO *info) {

	assert(CheckBoard(pos));
	assert(beta > alpha);
	assert(beta <=  INFINITE);
	assert(beta >= -INFINITE);
	assert(alpha <=  INFINITE);
	assert(alpha >= -INFINITE);

	// Check time situation
	CheckTime(info);

	info->nodes++;

	// Update selective depth
	if (pos->ply > info->seldepth)
		info->seldepth = pos->ply;

	// Check for draw
	if (IsRepetition(pos) || pos->fiftyMove >= 100)
		return 0;

	// If we are at max depth, return static eval
	if (pos->ply >= MAXDEPTH)
		return EvalPosition(pos);

	// Standing Pat -- If the stand-pat beats beta there is most likely also a move that beats beta
	// so we assume we have a beta cutoff. If the stand-pat beats alpha we use it as alpha.
	int score = EvalPosition(pos);
	if (score >= beta)
		return score;
	if ((score + Q_VAL * 2) < alpha) // Very pessimistic (forced by poor eval) delta pruning
		return alpha;
	if (score > alpha)
		alpha = score;

	// Generate all moves
	S_MOVELIST list[1];
	GenNoisyMoves(pos, list);

	int movesTried = 0;
	int bestScore = score;
	score = -INFINITE;

	// Move loop
	for (unsigned int moveNum = 0; moveNum < list->count; ++moveNum) {

		int move = PickNextMove(list);

		// Recursively search the positions after making the moves, skipping illegal ones
		if (!MakeMove(pos, move)) continue;
		score = -Quiescence(-beta, -alpha, pos, info);
		TakeMove(pos);

		movesTried++;

		if (info->stopped)
			return 0;

		if (score > bestScore) {

			// If score beats beta we have a cutoff
			if (score >= beta) {

	#ifdef SEARCH_STATS
				if (movesTried == 1) info->fhf++;
				info->fh++;
	#endif
				return score;
			}

			bestScore = score;

			// If score beats alpha we update alpha
			if (score > alpha)
				alpha = score;
		}
	}

	return bestScore;
}

// Alpha Beta
static int AlphaBeta(int alpha, const int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, const int doNull) {

	assert(CheckBoard(pos));
	assert(beta > alpha);
	assert(depth >= 0);
	assert(alpha <=  INFINITE);
	assert(alpha >= -INFINITE);
	assert(beta  <=  INFINITE);
	assert(beta  >= -INFINITE);

	S_MOVELIST list[1];

	// Quiescence at the end of search
	if (depth <= 0) 
		return Quiescence(alpha, beta, pos, info);

	// Check time situation
	CheckTime(info);

	info->nodes++;

	// Update selective depth
	if (pos->ply > info->seldepth) 
		info->seldepth = pos->ply;

	// Early exits (not in root node)
	if (pos->ply) {

		// Position is drawn
		if (IsRepetition(pos) || pos->fiftyMove >= 100)
			return 0;

		// Max depth reached
		if (pos->ply >= MAXDEPTH) 
			return EvalPosition(pos);

		// Mate distance pruning -- TODO doesn't work properly
		// alpha = alpha > -ISMATE + pos->ply     ? alpha : -ISMATE + pos->ply;
		// beta  = beta  <  ISMATE - pos->ply - 1 ? beta  :  ISMATE - pos->ply - 1;
		// if (alpha >= beta)
		// 	return alpha;
	}

	// Extend search if in check
	const int inCheck = SqAttacked(pos->kingSq[pos->side], !pos->side, pos);
	if (inCheck) depth++;

	int score = -INFINITE;
	int pvMove = NOMOVE;

	// Probe transposition table
	if (ProbeHashEntry(pos, &pvMove, &score, alpha, beta, depth)) {
#ifdef SEARCH_STATS
		pos->hashTable->cut++;
#endif
		return score;
	}

	// Syzygy
#ifdef USE_TBS
	unsigned tbresult;
	if ((tbresult = probeWDL(pos, depth)) != TB_RESULT_FAILED) {

		info->tbhits++;

		int val = tbresult == TB_LOSS ? -INFINITE + pos->ply + 1
				: tbresult == TB_WIN  ?  INFINITE - pos->ply - 1
									  :  0;

        int flag = tbresult == TB_LOSS ? BOUND_UPPER
                 : tbresult == TB_WIN  ? BOUND_LOWER 
									   : BOUND_EXACT;

        if (    flag == BOUND_EXACT
            || (flag == BOUND_LOWER && val >= beta)
            || (flag == BOUND_UPPER && val <= alpha)) {

            StoreHashEntry(pos, NOMOVE, val, flag, MAXDEPTH-1);
            return val;
        }
	}
#endif

	// Skip pruning while in check
	if (inCheck)
		goto standard_search;

	// Do a static evaluation for pruning consideration
	score = EvalPosition(pos);

	// Null Move Pruning
	if (doNull && score >= beta && pos->ply && (pos->bigPieces[pos->side] > 0) && depth >= 4) {

		MakeNullMove(pos);
		score = -AlphaBeta(-beta, -beta + 1, depth - 4, pos, info, false);
		TakeNullMove(pos);

		// Cutoff
		if (score >= beta) {
#ifdef SEARCH_STATS
			info->nullCut++;
#endif
			// Don't return unproven mate scores
			if (score >= ISMATE)
				score = beta;
			return score;
		}
	}

standard_search:

	// Generate all moves
	GenAllMoves(pos, list);

	unsigned int moveNum;
	unsigned int movesTried = 0;
	const int oldAlpha = alpha;
	int bestMove = NOMOVE;
	int bestScore = -INFINITE;

	score = -INFINITE;

	// Set score of PV move
	if (pvMove != NOMOVE)
		for (moveNum = 0; moveNum < list->count; ++moveNum)
			if (list->moves[moveNum].move == pvMove) {
				list->moves[moveNum].score = 2000000;
				break;
			}

	// Move loop
	for (moveNum = 0; moveNum < list->count; ++moveNum) {

		// Move the best move to front of queue
		int move = PickNextMove(list);

		// Make the next predicted best move, skipping illegal ones
		if (!MakeMove(pos, move)) continue;

		// Do zero-window searches around alpha on moves other than the PV
		if (movesTried > 0)
			score = -AlphaBeta(-alpha - 1, -alpha, depth - 1, pos, info, true);

		// Do normal search on PV, and non-PV if the zero-window indicates it beats PV
		if ((score > alpha && score < beta) || movesTried == 0)
			score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, true);

		// Undo the move
		TakeMove(pos);

		movesTried++;

		if (info->stopped)
			return 0;

		assert(-INFINITE <= score && score <=  INFINITE);

		// Found a new best move in this position
		if (score > bestScore) {

			// If score beats beta we have a cutoff
			if (score >= beta) {

				// Update killers if quiet move
				if (!(move & MOVE_IS_CAPTURE)) {
					pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
					pos->searchKillers[0][pos->ply] = move;
				}

#ifdef SEARCH_STATS
				if (movesTried == 1) info->fhf++;
				info->fh++;
#endif

				StoreHashEntry(pos, move, score, BOUND_LOWER, depth);

				return score;
			}

			bestScore = score;
			bestMove = move;

			// If score beats alpha we update alpha
			if (score > alpha) {

				alpha = score;

				// Update searchHistory if quiet move
				if (!(move & MOVE_IS_CAPTURE))
					pos->searchHistory[pos->board[FROMSQ(bestMove)]][TOSQ(bestMove)] += depth;
			}
		}
	}

	// Checkmate or stalemate
	if (!movesTried)
		return inCheck ? -INFINITE + pos->ply : 0;

	assert(alpha >= oldAlpha);
	assert(alpha <=  INFINITE);
	assert(alpha >= -INFINITE);
	assert(bestScore <=  INFINITE);
	assert(bestScore >= -INFINITE);

	const int flag = alpha != oldAlpha ? BOUND_EXACT : BOUND_UPPER;
	StoreHashEntry(pos, bestMove, bestScore, flag, depth);

	return bestScore;
}

// Aspiration window
int AspirationWindow(S_BOARD *pos, S_SEARCHINFO *info, const int depth, int previousScore) {

	// Dynamic bonus increasing initial window and delta
	const int bonus = (previousScore * previousScore) / 8;
	// Delta used for initial window and widening
	const int delta = (P_VAL / 2) + bonus;
	// Initial window
	int alpha = previousScore - delta / 4;
	int beta  = previousScore + delta / 4;
	// Counter for failed searches, bounds are relaxed more for each successive fail
	unsigned int fails = 0;

	while (true) {
		int result = AlphaBeta(alpha, beta, depth, pos, info, true);
		// Result within the bounds is accepted as correct
		if ((result >= alpha && result <= beta) || info->stopped)
			return result;
		// Failed low, relax lower bound and search again
		else if (result < alpha) {
			alpha -= delta << fails++;
			alpha = alpha < -INFINITE ? -INFINITE : alpha;
		// Failed high, relax upper bound and search again
		} else if (result > beta) {
			beta += delta << fails++;
			beta = beta > INFINITE ? INFINITE : beta;
		}
	}
}

// Root of search
void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {

	int bestScore;
	unsigned int currentDepth;

	ClearForSearch(pos, info);

	// Iterative deepening
	for (currentDepth = 1; currentDepth <= info->depth; ++currentDepth) {

		// Search position, using aspiration windows for higher depths
		if (currentDepth > 6)
			bestScore = AspirationWindow(pos, info, currentDepth, bestScore);
		else
			bestScore = AlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, true);

		// Stop search if applicable
		if (info->stopped) break;

		// Print thinking
		PrintThinking(info, pos, bestScore, currentDepth);
	}

	// Print conclusion
	PrintConclusion(pos);
}