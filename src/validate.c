// validate.c

#include "board.h"
#include "move.h"

#ifndef NDEBUG

int ValidSquare(const int sq) {
	return A1 <= sq && sq <= H8;
}

int ValidSide(const int side) {
	return side == WHITE || side == BLACK;
}

int ValidPiece(const int piece) {
	return (wP <= piece && piece <= wK) || (bP <= piece && piece <= bK);
}

int ValidPieceOrEmpty(const int piece) {
	return piece == EMPTY || ValidPiece(piece);
}

int MoveListOk(const S_MOVELIST *list, const S_BOARD *pos) {

	if (list->count < 0 || list->count >= MAXPOSITIONMOVES)
		return false;

	int from, to;

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		to 	 =   TOSQ(list->moves[MoveNum].move);
		from = FROMSQ(list->moves[MoveNum].move);

		if (!ValidSquare(to) || !ValidSquare(from))
			return false;

		if (!ValidPiece(pos->pieces[from])) {
			PrintBoard(pos);
			return false;
		}
	}

	return true;
}
#endif