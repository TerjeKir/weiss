// validate.c

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "evaluate.h"


int ValidSquare(const int sq) {
	return (sq >= 0 && sq < 64);
}

int SideValid(const int side) {
	return (side == WHITE || side == BLACK) ? 1 : 0;
}

int FileRankValid(const int fr) {
	return (fr >= 0 && fr <= 7) ? 1 : 0;
}

int PieceValid(const int pce) {
	return (pce >= wP && pce <= bK) ? 1 : 0;
}

int PieceValidEmpty(const int pce) {
	return (PieceValid(pce) || pce == EMPTY) ? 1 : 0;
}

int MoveListOk(const S_MOVELIST *list, const S_BOARD *pos) {

	if (list->count < 0 || list->count >= MAXPOSITIONMOVES)
		return FALSE;

	int from, to;

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		to 	 =   TOSQ(list->moves[MoveNum].move);
		from = FROMSQ(list->moves[MoveNum].move);

		if (!ValidSquare(to) || !ValidSquare(from))
			return FALSE;

		if (!PieceValid(pos->pieces[from])) {
			PrintBoard(pos);
			return FALSE;
		}
	}

	return TRUE;
}

void MirrorEvalTest(S_BOARD *pos) {
	FILE *file;
	file = fopen("mirror.epd", "r");
	char lineIn[1024];
	int ev1 = 0;
	int ev2 = 0;
	int positions = 0;
	if (file == NULL) {
		printf("File Not Found\n");
		return;
	} else {
		while (fgets(lineIn, 1024, file) != NULL) {
			ParseFen(lineIn, pos);
			positions++;
			ev1 = EvalPosition(pos);
			MirrorBoard(pos);
			ev2 = EvalPosition(pos);

			if (ev1 != ev2) {
				printf("\n\n\n");
				ParseFen(lineIn, pos);
				PrintBoard(pos);
				MirrorBoard(pos);
				PrintBoard(pos);
				printf("\n\nMirror Fail:\n%s\n", lineIn);
				getchar();
				return;
			}

			if ((positions % 1000) == 0)
				printf("position %d\n", positions);

			memset(&lineIn[0], 0, sizeof(lineIn));
		}
	}
}
