// search.c

#include <stdio.h>
#include <stdlib.h>

#include "fathom/tbprobe.h"
#include "attack.h"
#include "board.h"
#include "evaluate.h"
#include "io.h"
#include "makemove.h"
#include "misc.h"
#include "move.h"
#include "movegen.h"
#include "pvtable.h"
#include "syzygy.h"


// Check if time up, or interrupt from GUI
static void CheckUp(S_SEARCHINFO *info) {

	if (info->timeset && GetTimeMs() > info->stoptime)
		info->stopped = TRUE;

	ReadInput(info);
}

// Move best move to the front of the queue
static void PickNextMove(int moveNum, S_MOVELIST *list) {

	S_MOVE temp;
	int bestScore = 0;
	int bestNum = moveNum;

	for (int index = moveNum; index < list->count; ++index)
		if (list->moves[index].score > bestScore) {
			bestScore = list->moves[index].score;
			bestNum = index;
		}

	assert(moveNum >= 0 && moveNum < list->count);
	assert(bestNum >= 0 && bestNum < list->count);
	assert(bestNum >= moveNum);

	temp = list->moves[moveNum];
	list->moves[moveNum] = list->moves[bestNum];
	list->moves[bestNum] = temp;
}

// Checks whether position has already occurred
static int IsRepetition(const S_BOARD *pos) {

	for (int index = pos->hisPly - pos->fiftyMove; index < pos->hisPly - 1; ++index) {

		assert(index >= 0 && index < MAXGAMEMOVES);
		if (pos->posKey == pos->history[index].posKey)
			return TRUE;
	}

	return FALSE;
}

// Get ready to start a search
static void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info) {

	int index, index2;

	for (index = 0; index < 13; ++index)
		for (index2 = 0; index2 < 64; ++index2)
			pos->searchHistory[index][index2] = 0;

	for (index = 0; index < 2; ++index)
		for (index2 = 0; index2 < MAXDEPTH; ++index2)
			pos->searchKillers[index][index2] = 0;

#ifdef SEARCH_STATS
	pos->HashTable->hit = 0;
	pos->HashTable->cut = 0;
	pos->HashTable->overWrite = 0;

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
static int Quiescence(int alpha, int beta, S_BOARD *pos, S_SEARCHINFO *info) {

	assert(CheckBoard(pos));
	assert(beta > alpha);
	assert(beta <=  INFINITE);
	assert(beta >= -INFINITE);
	assert(alpha <=  INFINITE);
	assert(alpha >= -INFINITE);

	// Check if we should stop
	if ((info->nodes & 2047) == 0)
		CheckUp(info);

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
	if (score > alpha)
		alpha = score;

	// Generate all moves
	S_MOVELIST list[1];
	GenerateAllCaptures(pos, list);

	int legal = 0;
	int bestScore = score;
	score = -INFINITE;

	// Move loop
	for (int moveNum = 0; moveNum < list->count; ++moveNum) {

		PickNextMove(moveNum, list);

		// Recursively search the positions after making the moves, skipping illegal ones
		if (!MakeMove(pos, list->moves[moveNum].move)) continue;
		score = -Quiescence(-beta, -alpha, pos, info);
		TakeMove(pos);

		legal++;

		if(info->stopped == TRUE)
			return 0;
		
		if (score > bestScore)
			bestScore = score;

		// If score beats alpha we update alpha
		if (score > alpha) {

			// If score beats beta we have a cutoff
			if (score >= beta) {

#ifdef SEARCH_STATS
				if (legal == 1) info->fhf++;
				info->fh++;
#endif
				return bestScore;
			}

			alpha = score;
		}
	}

	return bestScore;
}

// Alpha Beta
static int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, int doNull) {

	assert(CheckBoard(pos));
	assert(beta > alpha);
	assert(depth >= 0);
	assert(alpha <=  INFINITE);
	assert(alpha >= -INFINITE);
	assert(beta  <=  INFINITE);
	assert(beta  >= -INFINITE);

	// Quiescence at the end of search
	if (depth <= 0) 
		return Quiescence(alpha, beta, pos, info);

	// Check for time
	if ((info->nodes & 2047) == 0) 
		CheckUp(info);

	info->nodes++;

	// Update selective depth
	if (pos->ply > info->seldepth) 
		info->seldepth = pos->ply;

	// Repetition reached
	if ((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply) 
		return 0;

	// Syzygy
#ifdef USE_TBS
	unsigned tbresult;
	if ((tbresult = probeWDL(pos, depth)) != TB_RESULT_FAILED) {

		info->tbhits++;

		int val = tbresult == TB_LOSS ? -INFINITE + pos->ply + 1
				: tbresult == TB_WIN  ?  INFINITE - pos->ply - 1
									  :  0;

        int flag = tbresult == TB_LOSS ? HFALPHA
                 : tbresult == TB_WIN  ? HFBETA 
									   : HFEXACT;

        if (    flag == HFEXACT
            || (flag == HFBETA  && val >= beta)
            || (flag == HFALPHA && val <= alpha)) {

            StoreHashEntry(pos, NOMOVE, val, flag, MAXDEPTH-1);
            return val;
        }
	}
#endif

	// Max Depth reached
	if (pos->ply >= MAXDEPTH) 
		return EvalPosition(pos);

	// Extend search if in check
	int inCheck = SqAttacked(pos->KingSq[pos->side], !pos->side, pos);
	if (inCheck) depth++;

	int score = -INFINITE;
	int pvMove = NOMOVE;

	// Probe transposition table
	if (ProbeHashEntry(pos, &pvMove, &score, alpha, beta, depth)) {
#ifdef SEARCH_STATS
		pos->HashTable->cut++;
#endif
		return score;
	}

	// Null Move Pruning
	if (doNull && !inCheck && pos->ply && (pos->bigPieces[pos->side] > 0) && depth >= 4) {

		MakeNullMove(pos);
		score = -AlphaBeta(-beta, -beta + 1, depth - 4, pos, info, FALSE);
		TakeNullMove(pos);

		if (score >= beta) {
#ifdef SEARCH_STATS
			info->nullCut++;
#endif
			return score;
		}
	}

	// Generate all moves
	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int moveNum;
	int legalMoves = 0;
	int oldAlpha = alpha;
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
		PickNextMove(moveNum, list);

		// Recursively search the positions after making the moves, skipping illegal ones
		if (!MakeMove(pos, list->moves[moveNum].move)) continue;
		score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE);
		TakeMove(pos);

		legalMoves++;

		if(info->stopped == TRUE)
			return 0;

		assert(-INFINITE <= score && score <=  INFINITE);

		// Found a new best move in this position
		if (score > bestScore) {

			bestScore = score;
			bestMove = list->moves[moveNum].move;

			// If score beats alpha we update alpha
			if (score > alpha) {
				// If score beats beta we have a cutoff
				if (score >= beta) {

					// Update killers if quiet move
					if (!(list->moves[moveNum].move & FLAG_CAPTURE)) {
						pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
						pos->searchKillers[0][pos->ply] = list->moves[moveNum].move;
					}
				
#ifdef SEARCH_STATS
					if (legal == 1) info->fhf++;
					info->fh++;
#endif

					StoreHashEntry(pos, bestMove, bestScore, HFBETA, depth);

					return bestScore;
				}

				alpha = score;

				// Update searchHistory if quiet move
				if (!(list->moves[moveNum].move & FLAG_CAPTURE))
					pos->searchHistory[pos->pieces[FROMSQ(bestMove)]][TOSQ(bestMove)] += depth;
			}
		}
	}

	// Checkmate or stalemate
	if (!legalMoves)
		return inCheck ? -INFINITE + pos->ply : 0;

	assert(alpha >= oldAlpha);
	assert(alpha <=  INFINITE);
	assert(alpha >= -INFINITE);
	assert(bestScore <=  INFINITE);
	assert(bestScore >= -INFINITE);

	int flag = alpha != oldAlpha ? HFEXACT : HFALPHA;
	StoreHashEntry(pos, bestMove, bestScore, flag, depth);

	return bestScore;
}

// Root of search
void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {

	int bestMove = NOMOVE;
	int bestScore, currentDepth;

	ClearForSearch(pos, info);

	// Iterative deepening
	for (currentDepth = 1; currentDepth <= info->depth; ++currentDepth) {

		// Search position
		bestScore = AlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, TRUE);

		// Stop search if applicable
		if (info->stopped) break;

		// Print thinking
		PrintThinking(info, pos, bestScore, currentDepth);

#ifdef SEARCH_STATS
		printf("Stats: Hits: %d Overwrite: %d NewWrite: %d Cut: %d\nOrdering %.2f NullCut: %d\n", pos->HashTable->hit,
			pos->HashTable->overWrite, pos->HashTable->newWrite, pos->HashTable->cut, (info->fhf/info->fh)*100, info->nullCut);
#endif
	}

	// Get and print best move when done thinking
	bestMove = pos->PvArray[0];

	if (info->GAME_MODE == UCIMODE)
		printf("bestmove %s\n", MoveToStr(bestMove));

	else {
		printf("\n\n***!! %s !!***\n\n", MoveToStr(bestMove));
		MakeMove(pos, bestMove);
		PrintBoard(pos);
	}
}