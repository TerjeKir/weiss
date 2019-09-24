// validate.c

#include "board.h"
#include "move.h"


int ValidSquare(const int sq) {
	return (sq >= 0 && sq < 64);
}

int SideValid(const int side) {
	return (side == WHITE || side == BLACK) ? 1 : 0;
}

int PieceValid(const int pce) {
	return (pce >= wP && pce <= bK) ? 1 : 0;
}

int PieceValidEmpty(const int pce) {
	return (PieceValid(pce) || pce == EMPTY) ? 1 : 0;
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

		if (!PieceValid(pos->pieces[from])) {
			PrintBoard(pos);
			return false;
		}
	}

	return true;
}