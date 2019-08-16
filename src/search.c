// search.c

#include <stdio.h>

#include "defs.h"
#include "fathom/tbprobe.h"
#include "attack.h"
#include "board.h"
#include "evaluate.h"
#include "io.h"
#include "movegen.h"
#include "makemove.h"
#include "misc.h"
#include "pvtable.h"


int rootDepth;


// Check if time up, or interrupt from GUI
static void CheckUp(S_SEARCHINFO *info) {
	
	if (info->timeset && GetTimeMs() > info->stoptime)
		info->stopped = TRUE;

	ReadInput(info);
}

// Move best move to the front of the queue
static void PickNextMove(int moveNum, S_MOVELIST *list) {

	S_MOVE temp;
	int index = 0;
	int bestScore = 0;
	int bestNum = moveNum;

	for (index = moveNum; index < list->count; ++index) {
		if (list->moves[index].score > bestScore) {
			bestScore = list->moves[index].score;
			bestNum = index;
		}
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

	int index = 0;

	for (index = pos->hisPly - pos->fiftyMove; index < pos->hisPly - 1; ++index) {
		assert(index >= 0 && index < MAXGAMEMOVES);
		if (pos->posKey == pos->history[index].posKey)
			return TRUE;
	}
	return FALSE;
}

// Get ready to start a search
static void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info) {

	int index = 0;
	int index2 = 0;

	for (index = 0; index < 13; ++index)
		for (index2 = 0; index2 < BRD_SQ_NUM; ++index2)
			pos->searchHistory[index][index2] = 0;

	for (index = 0; index < 2; ++index)
		for (index2 = 0; index2 < MAXDEPTH; ++index2)
			pos->searchKillers[index][index2] = 0;

	pos->ply = 0;
	pos->HashTable->hit = 0;
	pos->HashTable->cut = 0;
	pos->HashTable->overWrite = 0;

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

	if ((info->nodes & 2047) == 0)
		CheckUp(info);

	info->nodes++;

	if (pos->ply > info->seldepth)
		info->seldepth = pos->ply;

	if (IsRepetition(pos) || pos->fiftyMove >= 100)
		return 0;

	if (pos->ply > MAXDEPTH - 1)
		return EvalPosition(pos);

	int Score = EvalPosition(pos);

	assert(Score > -INFINITE && Score < INFINITE);

	if (Score >= beta)
		return beta;

	if (Score > alpha)
		alpha = Score;

	S_MOVELIST list[1];
	GenerateAllCaptures(pos, list);

	int MoveNum = 0;
	int Legal = 0;
	Score = -INFINITE;

	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		Legal++;
		Score = -Quiescence(-beta, -alpha, pos, info);
		TakeMove(pos);

		if (info->stopped)
			return 0;

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

	//assert(alpha >= OldAlpha);

	return alpha;
}

// Alpha Beta
static int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, int DoNull) {

	assert(CheckBoard(pos));
	assert(beta > alpha);
	assert(depth >= 0);
	
	// unsigned tbresult;
	// int val;

	// Quiescence at the end of search
	if (depth == 0)
		return Quiescence(alpha, beta, pos, info);

	// Check for time
	if ((info->nodes & 2047) == 0)
		CheckUp(info);

	info->nodes++;

	if (pos->ply > info->seldepth)
		info->seldepth = pos->ply;

	// Repetition reached
	if ((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply)
		return 0;

	// Syzygy
	// if ((tbresult = probeWDL(pos, depth)) != TB_RESULT_FAILED) {

	// 	info->tbhits++; // Increment tbhits counter

	// 	// Convert the WDL value to a score. We consider blessed losses
	// 	// and cursed wins to be a draw, and thus set value to zero.
	// 	val = tbresult == TB_LOSS ? -INFINITE + MAXDEPTH + pos->ply + 1
	// 		: tbresult == TB_WIN  ?  INFINITE - MAXDEPTH - pos->ply - 1 : 0;

	// 	return val;
	// }

	// Max Depth reached
	if (pos->ply > MAXDEPTH - 1)
		return EvalPosition(pos);

	// Extend search if in check
	int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);
	if (InCheck)
		depth++;

	int Score = -INFINITE;
	int PvMove = NOMOVE;

	if (ProbeHashEntry(pos, &PvMove, &Score, alpha, beta, depth)) {
		pos->HashTable->cut++;
		return Score;
	}

	// Null Move Pruning
	if (DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 0) && depth >= 4) {
		MakeNullMove(pos);
		Score = -AlphaBeta(-beta, -beta + 1, depth - 4, pos, info, FALSE);
		TakeNullMove(pos);
		if (info->stopped) {
			return 0;
		}

		if (Score >= beta && abs(Score) < ISMATE) {
			info->nullCut++;
			return beta;
		}
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int MoveNum = 0;
	int Legal = 0;
	int OldAlpha = alpha;
	int BestMove = NOMOVE;

	int BestScore = -INFINITE;

	Score = -INFINITE;

	if (PvMove != NOMOVE)
		for (MoveNum = 0; MoveNum < list->count; ++MoveNum)
			if (list->moves[MoveNum].move == PvMove) {
				list->moves[MoveNum].score = 2000000;
				//printf("Pv move found \n");
				break;
			}


	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		Legal++;
		Score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE);
		TakeMove(pos);

		if (info->stopped)
			return 0;

		if (Score > BestScore) {
			BestScore = Score;
			BestMove = list->moves[MoveNum].move;
			if (Score > alpha) {
				if (Score >= beta) {

					if (Legal == 1)
						info->fhf++;

					info->fh++;

					if (!(list->moves[MoveNum].move & MOVE_FLAG_CAP)) {
						pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
						pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
					}

					StoreHashEntry(pos, BestMove, beta, HFBETA, depth);

					return beta;
				}
				alpha = Score;

				if (!(list->moves[MoveNum].move & MOVE_FLAG_CAP))
					pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth;
			}
		}
	}

	if (Legal == 0) {
		if (InCheck)
			return -INFINITE + pos->ply;
		else
			return 0;
	}

	assert(alpha >= OldAlpha);

	if (alpha != OldAlpha)
		StoreHashEntry(pos, BestMove, BestScore, HFEXACT, depth);
	else
		StoreHashEntry(pos, BestMove, alpha, HFALPHA, depth);

	return alpha;
}

// Root of search
void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {

	int bestMove, bestScore, currentDepth, pvMoves, pvNum, timeElapsed;

	ClearForSearch(pos, info);

	// iterative deepening
	for (currentDepth = 1; currentDepth <= info->depth; ++currentDepth) {
		if (currentDepth > info->seldepth) 
			info->seldepth = currentDepth;

		rootDepth = currentDepth;
		bestScore = AlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, TRUE);

		if (info->stopped) break;

		pvMoves = GetPvLine(currentDepth, pos);
		bestMove = pos->PvArray[0];

		timeElapsed = GetTimeMs() - info->starttime;

		// Print thinking
		// UCI mode
		if (info->GAME_MODE == UCIMODE) {

			printf("info score ");

			// Score or mate
			if (bestScore > ISMATE)
				printf("mate %d ", ((INFINITE - bestScore) / 2) + 1);
			else if (bestScore < -ISMATE)
				printf("mate -%d ", (INFINITE + bestScore) / 2);
			else
				printf("cp %d ", bestScore);

			// Basic info
			printf("depth %d seldepth %d nodes %I64d tbhits %I64d time %d ",
					currentDepth, info->seldepth, info->nodes, info->tbhits, timeElapsed);

			// Nodes per second
			if (timeElapsed > 0)
				printf("nps %I64d ", ((info->nodes / timeElapsed) * 1000));

			// Principal variation
			printf("pv");
			pvMoves = GetPvLine(currentDepth, pos);
			for (pvNum = 0; pvNum < pvMoves; ++pvNum) {
				printf(" %s", PrMove(pos->PvArray[pvNum]));
			}
			printf("\n");
		// CLI mode
		} else if (info->POST_THINKING) {
			// Basic info
			printf("score:%d depth:%d nodes:%I64d time:%d(ms) ",
					bestScore, currentDepth, info->nodes, timeElapsed);

			// Principal variation
			printf("pv");
			pvMoves = GetPvLine(currentDepth, pos);
			for (pvNum = 0; pvNum < pvMoves; ++pvNum)
				printf(" %s", PrMove(pos->PvArray[pvNum]));

			printf("\n");
		}

		//printf("Hits:%d Overwrite:%d NewWrite:%d Cut:%d\nOrdering %.2f NullCut:%d\n", pos->HashTable->hit,
		//	pos->HashTable->overWrite, pos->HashTable->newWrite, pos->HashTable->cut, (info->fhf/info->fh)*100, info->nullCut);
	}

	// Print the move chosen after searching
	if (info->GAME_MODE == UCIMODE)
		printf("bestmove %s\n", PrMove(bestMove));
	else {
		printf("\n\n***!! weiss makes move %s !!***\n\n", PrMove(bestMove));
		MakeMove(pos, bestMove);
		PrintBoard(pos);
	}
}