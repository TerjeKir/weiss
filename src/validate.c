// validate.c

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "evaluate.h"
#include "io.h"
#include "move.h"
#include "search.h"


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

	char filename[] = "../EPDs/all.epd";

	FILE *file;
	if ((file = fopen(filename, "r")) == NULL){
		printf("MirrorEvalTest: %s not found.\n", filename);
		return;
	}

	char lineIn[1024];
	int ev1, ev2;
	int positions = 0;

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
			printf("\n\nMirrorEvalTest Fail:\n%s\n", lineIn);
			getchar();
			return;
		}

		if (positions % 100 == 0)
			printf("position %d\n", positions);

		memset(&lineIn[0], 0, sizeof(lineIn));
	}

	fclose(file);
}

void MateInXTest(S_BOARD *pos) {

	char filename[] = "../EPDs/mate_-_.epd"; 				// _s are placeholders
	FILE *file;

	for (char depth = '3'; depth < '9'; ++depth) {
		filename[12] = depth;
		for (char side = 'b'; side >= 'b'; side += 21) { 	// Hacky way to alternate 'b' <-> 'w', overflowing goes below 'b'
			filename[14] = side;

			if ((file = fopen(filename, "r")) == NULL){
				printf("MateInXTest: %s not found.\n", filename);
				return;
			}

			S_SEARCHINFO info[1];
			char lineIn[1024];
			char *bm, *ce;
			int bestMoves[20];
			int lineCnt = 0;
			int correct, bmCnt, bestFound, extensions;
			uint8_t mateInX;

			while (fgets(lineIn, 1024, file) != NULL) {

				// 1k1r4/2p2ppp/8/8/Qb6/2R1Pn2/PP2KPPP/3r4 b - - bm Nf3-g1+; ce +M1; pv Nf3-g1+;

				ParseFen(lineIn, pos);
				lineCnt++;

				printf("Line %d in file: %s\n", lineCnt, filename);

				// Clear bestMoves for reuse
				memset(bestMoves, 0, sizeof(int) * 20);

				bm = strstr(lineIn, "bm") + 3;
				ce = strstr(lineIn, "ce");

				// Read in the best move(s)
				bmCnt = 0;
				while (bm < ce) {
					bestMoves[bmCnt] = ParseEPDMove(bm, pos);
					bm = strstr(bm, " ") + 1;
					bmCnt++;
				}

				info->depth = (depth - '0') * 2;
				extensions = 0;
search:
				SearchPosition(pos, info);

				bestFound = pos->PvArray[0];

				correct = 0;
				for (int i = 0; i <= bmCnt; ++i)
					if (bestFound == bestMoves[i]){
						printf("\nBest Found: %s\n\n", MoveToStr(bestFound));
						correct = 1;
					}
					
				if (!correct && extensions < 7) {
					info->depth += 1;
					extensions += 1;
					goto search;
				}

				if (!correct) {
					printf("\n\nMateInXTest Fail: Not found.\n%s", lineIn);
					printf("Line %d in file: %s\n", lineCnt, filename);
					for (int i = 0; i < bmCnt; ++i)
						printf("Accepted answer: %s\n", MoveToStr(bestMoves[i]));
					getchar();
					fclose(file);
					return;
				}
				memset(&lineIn[0], 0, sizeof(lineIn));
			}
		}
	}
	printf("MateInXTest Complete!\n\n");
	fclose(file);
}