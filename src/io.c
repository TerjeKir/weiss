// io.c

#include <stdio.h>

#include "board.h"
#include "data.h"
#include "movegen.h"
#include "validate.h"


#define IsKnight(p) (PieceKnight[(p)])
#define IsBishop(p) (PieceBishop[(p)])
#define IsRook(p)   (PieceRook[(p)])
#define IsQueen(p)  (PieceQueen[(p)])


char *MoveToStr(const int move) {

	static char moveStr[6];

	int ff = SqToFile[FROMSQ(move)];
	int rf = SqToRank[FROMSQ(move)];
	int ft = SqToFile[  TOSQ(move)];
	int rt = SqToRank[  TOSQ(move)];

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