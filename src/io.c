// io.c

#include <stdio.h>

#include "defs.h"
#include "data.h"
#include "movegen.h"
#include "board.h"
#include "validate.h"


#define IsKnight(p) (PieceKnight[(p)])
#define IsBishop(p) (PieceBishop[(p)])
#define IsRook(p)   (PieceRook[(p)])
#define IsQueen(p)  (PieceQueen[(p)])


char *MoveToStr(const int move) {

	static char MvStr[6];

	int ff = SqToFile[FROMSQ(move)];
	int rf = SqToRank[FROMSQ(move)];
	int ft = SqToFile[  TOSQ(move)];
	int rt = SqToRank[  TOSQ(move)];

	int promoted = PROMOTED(move);

	if (promoted) {
		char pchar = 'q';
		if (IsKnight(promoted))
			pchar = 'n';
		else if (IsRook(promoted))
			pchar = 'r';
		else if (IsBishop(promoted))
			pchar = 'b';

		sprintf(MvStr, "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);
	} else
		sprintf(MvStr, "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));

	return MvStr;
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

	int Move, PromPce;

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		Move = list->moves[MoveNum].move;
		if (FROMSQ(Move) == from && TOSQ(Move) == to) {

			PromPce = PROMOTED(Move);
			if (PromPce != EMPTY) {

				if (IsQueen(PromPce) && ptrChar[4] == 'q')
					return Move;
				else if (IsKnight(PromPce) && ptrChar[4] == 'n')
					return Move;
				else if (IsRook(PromPce) && ptrChar[4] == 'r')
					return Move;
				else if (IsBishop(PromPce) && ptrChar[4] == 'b')
					return Move;

				continue;
			}
			return Move;
		}
	}
	return NOMOVE;
}