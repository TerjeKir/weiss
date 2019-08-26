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

	pos->ply = 0;
#ifdef PV_STATS
	pos->HashTable->hit = 0;
	pos->HashTable->cut = 0;
	pos->HashTable->overWrite = 0;
#endif

	info->fh = 0;
	info->fhf = 0;
	info->nodes = 0;
	info->tbhits = 0;
	info->stopped = 0;
	info->seldepth = 0;
}

// Quiescence
static int Quiescence(int alpha, int beta, S_BOARD *pos, S_SEARCHINFO *info) {

	assert(CheckBoard(pos));
	assert(beta > alpha);

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
	if (pos->ply > MAXDEPTH - 1)
		return EvalPosition(pos);

	// Evaluate the position
	int Score = EvalPosition(pos);

	// TODO
	if (Score >= beta)
		return beta;
	// TODO
	if (Score > alpha)
		alpha = Score;

	// Generate all moves
	S_MOVELIST list[1];
	GenerateAllCaptures(pos, list);

	int Legal = 0;
	Score = -INFINITE;

	// Move loop
	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

		// Skip illegal moves
		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		Legal++;

		Score = -Quiescence(-beta, -alpha, pos, info);
		TakeMove(pos);

		// Stop search if gui says to stop
		if (info->stopped)
			return 0;

		// Update alpha, return beta if beta cutoff? TODO
		if (Score > alpha) {
			if (Score >= beta) {
				if (Legal == 1)
					info->fhf++;

				info->fh++;
				return beta;
			}
			alpha = Score;
		}
	}

	return alpha;
}

// Alpha Beta
static int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, int DoNull) {

	assert(CheckBoard(pos));
	assert(beta > alpha);
	assert(depth >= 0);
	assert(alpha <=  INFINITE);
	assert(alpha >= -INFINITE);
	assert(beta  <=  INFINITE);
	assert(beta  >= -INFINITE);

	// Quiescence at the end of search
	if (depth == 0) return Quiescence(alpha, beta, pos, info);

	// Check for time
	if ((info->nodes & 2047) == 0) CheckUp(info);

	info->nodes++;

	// Update selective depth
	if (pos->ply > info->seldepth) info->seldepth = pos->ply;

	// Repetition reached
	if ((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply) return 0;

	// Syzygy
#ifdef USE_TBS
	unsigned tbresult;
	if ((tbresult = probeWDL(pos, depth)) != TB_RESULT_FAILED) {

		info->tbhits++;

		int val = tbresult == TB_LOSS ? -INFINITE + pos->ply + 1
				: tbresult == TB_WIN  ?  INFINITE - pos->ply - 1
				: 0;

		return val;
	}
#endif
	// Max Depth reached
	if (pos->ply > MAXDEPTH - 1) return EvalPosition(pos);

	// Extend search if in check
	int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);
	if (InCheck) depth++;

	int Score = -INFINITE;
	int PvMove = NOMOVE;

	// Probe transposition table
	if (ProbeHashEntry(pos, &PvMove, &Score, alpha, beta, depth)) {
#ifdef PV_STATS
		pos->HashTable->cut++;
#endif
		return Score;
	}

	// Null Move Pruning
	if (DoNull && !InCheck && pos->ply && (pos->bigPieces[pos->side] > 0) && depth >= 4) {

		MakeNullMove(pos);
		Score = -AlphaBeta(-beta, -beta + 1, depth - 4, pos, info, FALSE);
		TakeNullMove(pos);

		if (info->stopped) return 0;

		if (Score >= beta && abs(Score) < ISMATE) {
			info->nullCut++;
			return beta;
		}
	}

	// Generate all moves
	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int MoveNum;
	int Legal = 0;
	int OldAlpha = alpha;
	int BestMove = NOMOVE;
	int BestScore = -INFINITE;

	Score = -INFINITE;

	// Set score of PV move
	if (PvMove != NOMOVE)
		for (MoveNum = 0; MoveNum < list->count; ++MoveNum)
			if (list->moves[MoveNum].move == PvMove) {
				list->moves[MoveNum].score = 2000000;
				break;
			}

	// Move loop
	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		// Move the best move to front of queue
		PickNextMove(MoveNum, list);

		// Skip illegal moves
		if (!MakeMove(pos, list->moves[MoveNum].move)) continue;

		Legal++;

		Score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE);
		TakeMove(pos);

		// Abort if we have to
		if (info->stopped) return 0;

		// Found a new best
		if (Score > BestScore) {

			BestScore = Score;
			BestMove = list->moves[MoveNum].move;

			// TODO
			if (Score > alpha) {
				if (Score >= beta) {

					if (Legal == 1) info->fhf++;

					info->fh++;

					if (!(list->moves[MoveNum].move & FLAG_CAPTURE)) {
						pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
						pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
					}

					assert(beta <=  INFINITE);
					assert(beta >= -INFINITE);
					
					StoreHashEntry(pos, BestMove, beta, HFBETA, depth);

					return beta;
				}
				alpha = Score;

				if (!(list->moves[MoveNum].move & FLAG_CAPTURE))
					pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth;
			}
		}
	}

	// Checkmate or stalemate
	if (Legal == 0) {
		if (InCheck)
			return -INFINITE + pos->ply;
		else
			return 0;
	}

	assert(alpha >= OldAlpha);
	assert(alpha <=  INFINITE);
	assert(alpha >= -INFINITE);
	assert(BestScore <=  INFINITE);
	assert(BestScore >= -INFINITE);

	if (alpha != OldAlpha)
		StoreHashEntry(pos, BestMove, BestScore, HFEXACT, depth);
	else
		StoreHashEntry(pos, BestMove, alpha, HFALPHA, depth);

	return alpha;
}

// Root of search
void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {

	int bestMove = NOMOVE;
	int bestScore, currentDepth, pvMoves;

	ClearForSearch(pos, info);

	// Iterative deepening
	for (currentDepth = 1; currentDepth <= info->depth; ++currentDepth) {
		if (currentDepth > info->seldepth) 
			info->seldepth = currentDepth;

		bestScore = AlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, TRUE);

		if (info->stopped) break;

		pvMoves = GetPvLine(currentDepth, pos);
		bestMove = pos->PvArray[0];

		PrintThinking(info, pos, bestScore, currentDepth, pvMoves);

#ifdef PV_STATS
		printf("PV Stats: Hits: %d Overwrite: %d NewWrite: %d Cut: %d\nOrdering %.2f NullCut: %d\n", pos->HashTable->hit,
			pos->HashTable->overWrite, pos->HashTable->newWrite, pos->HashTable->cut, (info->fhf/info->fh)*100, info->nullCut);
#endif
	}

	// Print the best move after search finishes
	if (info->GAME_MODE == UCIMODE)
		printf("bestmove %s\n", MoveToStr(bestMove));
	else {
		printf("\n\n***!! %s !!***\n\n", MoveToStr(bestMove));
		MakeMove(pos, bestMove);
		PrintBoard(pos);
	}
}