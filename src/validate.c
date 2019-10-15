// validate.c

#include "board.h"
#include "move.h"

#ifndef NDEBUG

bool ValidSquare(const int sq) {
	return A1 <= sq && sq <= H8;
}

bool ValidSide(const int side) {
	return side == WHITE || side == BLACK;
}

bool ValidPiece(const int piece) {
	return (wP <= piece && piece <= wK) || (bP <= piece && piece <= bK);
}

bool MoveListOk(const MoveList *list, const Position *pos) {

	if (list->count >= MAXPOSITIONMOVES)
		return false;

	int from, to;

	for (unsigned int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		to 	 =   TOSQ(list->moves[MoveNum].move);
		from = FROMSQ(list->moves[MoveNum].move);

		if (!ValidSquare(to) || !ValidSquare(from))
			return false;

		if (!ValidPiece(pos->board[from])) {
			PrintBoard(pos);
			return false;
		}
	}

	return true;
}
#endif