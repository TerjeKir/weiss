// io.c

#include <stdio.h>

#include "board.h"
#include "data.h"
#include "misc.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"
#include "validate.h"


#define IsKnight(p) (pieceKnight[(p)])
#define IsBishop(p) (pieceBishop[(p)])
#define IsRook(p)   (pieceRook[(p)])
#define IsQueen(p)  (pieceQueen[(p)])


// Translates a move to a string for printing
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

// Translates a string to a move
int ParseMove(const char *ptrChar, Position *pos) {

	assert(CheckBoard(pos));

	if (ptrChar[1] > '8' || ptrChar[1] < '1') return NOMOVE;
	if (ptrChar[3] > '8' || ptrChar[3] < '1') return NOMOVE;
	if (ptrChar[0] > 'h' || ptrChar[0] < 'a') return NOMOVE;
	if (ptrChar[2] > 'h' || ptrChar[2] < 'a') return NOMOVE;

	int from = (ptrChar[0] - 'a') + (8 * (ptrChar[1] - '1'));
	int to   = (ptrChar[2] - 'a') + (8 * (ptrChar[3] - '1'));

	assert(ValidSquare(from) && ValidSquare(to));

	MoveList list[1];
	GenAllMoves(pos, list);

	int move, promotion;

	for (unsigned int moveNum = 0; moveNum < list->count; ++moveNum) {

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

#ifdef CLI
// Translates a string from an .epd file to a move
// Nf3-g1+, f3-g1+, Nf3-g1, f3-g1N+ etc
int ParseEPDMove(const char *ptrChar, Position *pos) {

	assert(CheckBoard(pos));

	if ('B' <= ptrChar[0] && ptrChar[0] <= 'R')
		ptrChar++;

	if (ptrChar[1] > '8' || ptrChar[1] < '1') return NOMOVE;
	if (ptrChar[4] > '8' || ptrChar[4] < '1') return NOMOVE;
	if (ptrChar[0] > 'h' || ptrChar[0] < 'a') return NOMOVE;
	if (ptrChar[3] > 'h' || ptrChar[3] < 'a') return NOMOVE;

	int from = (ptrChar[0] - 'a') + (8 * (ptrChar[1] - '1'));
	int to   = (ptrChar[3] - 'a') + (8 * (ptrChar[4] - '1'));

	assert(ValidSquare(from) && ValidSquare(to));

	MoveList list[1];
	GenAllMoves(pos, list);

	int move, promotion;

	for (unsigned int moveNum = 0; moveNum < list->count; ++moveNum) {

		move = list->moves[moveNum].move;
		if (FROMSQ(move) == from && TOSQ(move) == to) {

			promotion = PROMOTION(move);
			if (promotion != EMPTY) {

				if (IsQueen(promotion) && ptrChar[5] == 'Q')
					return move;
				else if (IsKnight(promotion) && ptrChar[5] == 'N')
					return move;
				else if (IsRook(promotion) && ptrChar[5] == 'R')
					return move;
				else if (IsBishop(promotion) && ptrChar[5] == 'B')
					return move;

				continue;
			}
			return move;
		}
	}
	return NOMOVE;
}
#endif