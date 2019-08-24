// io.c

#include <stdio.h>

#include "board.h"
#include "data.h"
#include "misc.h"
#include "move.h"
#include "movegen.h"
#include "pvtable.h"
#include "validate.h"


#define IsKnight(p) (PieceKnight[(p)])
#define IsBishop(p) (PieceBishop[(p)])
#define IsRook(p)   (PieceRook[(p)])
#define IsQueen(p)  (PieceQueen[(p)])


char *MoveToStr(const int move) {

	static char moveStr[6];

	int ff = fileOf(FROMSQ(move));
	int rf = rankOf(FROMSQ(move));
	int ft = fileOf(TOSQ(move));
	int rt = rankOf(TOSQ(move));

	int promotion = PROMOTION(move);

	if (promotion) {
		char pchar = 'q';
		if (IsKnight(promotion))
			pchar = 'n';
		else if (IsRook(promotion))
			pchar = 'r';
		else if (IsBishop(promotion))
			pchar = 'b';

		sprintf(moveStr, "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);
	} else
		sprintf(moveStr, "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));

	return moveStr;
}

int ParseMove(char *ptrChar, S_BOARD *pos) {

	assert(CheckBoard(pos));

	if (ptrChar[1] > '8' || ptrChar[1] < '1') return NOMOVE;
	if (ptrChar[3] > '8' || ptrChar[3] < '1') return NOMOVE;
	if (ptrChar[0] > 'h' || ptrChar[0] < 'a') return NOMOVE;
	if (ptrChar[2] > 'h' || ptrChar[2] < 'a') return NOMOVE;

	int from = (ptrChar[0] - 'a') + (8 * (ptrChar[1] - '1'));
	int to   = (ptrChar[2] - 'a') + (8 * (ptrChar[3] - '1'));

	assert(ValidSquare(from) && ValidSquare(to));

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int move, promotion;

	for (int moveNum = 0; moveNum < list->count; ++moveNum) {

		move = list->moves[moveNum].move;
		if (FROMSQ(move) == from && TOSQ(move) == to) {

			promotion = PROMOTION(move);
			if (promotion != EMPTY) {

				if (IsQueen(promotion) && ptrChar[4] == 'q')
					return move;
				else if (IsKnight(promotion) && ptrChar[4] == 'n')
					return move;
				else if (IsRook(promotion) && ptrChar[4] == 'r')
					return move;
				else if (IsBishop(promotion) && ptrChar[4] == 'b')
					return move;

				continue;
			}
			return move;
		}
	}
	return NOMOVE;
}

// Print thinking
void PrintThinking(S_SEARCHINFO *info, S_BOARD *pos, int bestScore, int currentDepth, int pvMoves) {

	unsigned timeElapsed = GetTimeMs() - info->starttime;

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
			printf("nps %I64d ", ((info->nodes * 1000) / timeElapsed));

		// Principal variation
		printf("pv");
		pvMoves = GetPvLine(currentDepth, pos);
		for (int pvNum = 0; pvNum < pvMoves; ++pvNum) {
			printf(" %s", MoveToStr(pos->PvArray[pvNum]));
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
		for (int pvNum = 0; pvNum < pvMoves; ++pvNum)
			printf(" %s", MoveToStr(pos->PvArray[pvNum]));

		printf("\n");
	}
}