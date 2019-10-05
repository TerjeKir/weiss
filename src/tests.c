// tests.c

#ifdef CLI

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "data.h"
#include "evaluate.h"
#include "io.h"
#include "makemove.h"
#include "misc.h"
#include "move.h"
#include "movegen.h"
#include "search.h"


/* Perft */
uint64_t leafNodes;


static void RecursivePerft(const int depth, S_BOARD *pos) {

	assert(CheckBoard(pos));

	if (depth == 0) {
		leafNodes++;
		return;
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		RecursivePerft(depth - 1, pos);
		TakeMove(pos);
	}

	return;
}

// Counts number of moves that can be made in a position to some depth
void Perft(const int depth, S_BOARD *pos) {

	assert(CheckBoard(pos));

	printf("\nStarting Test To Depth:%d\n\n", depth);

	int start = GetTimeMs();
	leafNodes = 0;

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		int move = list->moves[MoveNum].move;

		if (!MakeMove(pos, move)){
			printf("move %d : %s : Illegal\n", MoveNum + 1, MoveToStr(move));
			continue;
		}

		uint64_t oldCount = leafNodes;
		RecursivePerft(depth - 1, pos);
		uint64_t newNodes = leafNodes - oldCount;
		printf("move %d : %s : %I64d\n", MoveNum + 1, MoveToStr(move), newNodes);

		TakeMove(pos);
	}

	int timeElapsed = GetTimeMs() - start;
	printf("\nPerft Complete : %I64d nodes visited in %dms\n", leafNodes, timeElapsed);
	if (timeElapsed > 0) 
		printf("               : %I64d nps\n", ((leafNodes * 1000) / timeElapsed));

	return;
}

/* Other tests */
// Checks evaluation is symmetric
void MirrorEvalTest(S_BOARD *pos) {

	char filename[] = "../EPDs/all.epd";

	FILE *file;
	if ((file = fopen(filename, "r")) == NULL) {
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

// Checks engine can find mates
void MateInXTest(S_BOARD *pos) {

	char filename[] = "../EPDs/mate_-_.epd"; 				// _s are placeholders
	FILE *file;

	int failures = 0;

	S_SEARCHINFO info[1];
	char lineIn[1024];
	char *bm, *ce;

	int bestMoves[20];
	int foundScore;
	int correct, bmCnt, bestFound, extensions, mateDepth, foundDepth;

	for (char depth = '1'; depth < '9'; ++depth) {
		filename[12] = depth;
		for (char side = 'b'; side >= 'b'; side += 21) { 	// Hacky way to alternate 'b' <-> 'w', overflowing goes below 'b'
			filename[14] = side;

			if ((file = fopen(filename, "r")) == NULL) {
				printf("MateInXTest: %s not found.\n", filename);
				return;
			}

			int lineCnt = 0;

			while (fgets(lineIn, 1024, file) != NULL) {

				// 1k1r4/2p2ppp/8/8/Qb6/2R1Pn2/PP2KPPP/3r4 b - - bm Nf3-g1+; ce +M1; pv Nf3-g1+;

				// Reset
				memset(bestMoves, 0, sizeof(bestMoves));
				memset(info, 0, sizeof(info));
				foundScore = 0;
				bestFound = 0;

				// Read next line
				ParseFen(lineIn, pos);
				lineCnt++;
				printf("Line %d in file: %s\n", lineCnt, filename);

				bm = strstr(lineIn, "bm") + 3;
				ce = strstr(lineIn, "ce");

				// Read in the mate depth
				mateDepth = depth - '0';

				// Read in the best move(s)
				bmCnt = 0;
				while (bm < ce) {
					bestMoves[bmCnt] = ParseEPDMove(bm, pos);
					bm = strstr(bm, " ") + 1;
					bmCnt++;
				}

				// Search setup
				info->depth = (depth - '0') * 2 - 1;
				info->starttime = GetTimeMs();
				extensions = 0;
search:
				SearchPosition(pos, info);

				bestFound = pos->pvArray[0];

				// Check if correct move is found
				correct = 0;
				for (int i = 0; i <= bmCnt; ++i)
					if (bestFound == bestMoves[i]){
						printf("\nBest Found: %s\n\n", MoveToStr(bestFound));
						correct = 1;
					}

				// Extend search if not found
				if (!correct && extensions < 8) {
					info->depth += 1;
					extensions += 1;
					goto search;
				}

				// Correct move not found
				if (!correct) {
					printf("\n\nMateInXTest Fail: Not found.\n%s", lineIn);
					printf("Line %d in file: %s\n", lineCnt, filename);
					for (int i = 0; i < bmCnt; ++i)
						printf("Accepted answer: %s\n", MoveToStr(bestMoves[i]));
					getchar();
					continue;
				}

				// Get pv score
				int index = pos->posKey % pos->hashTable->numEntries;
				if (pos->hashTable->TT[index].posKey == pos->posKey)
					foundScore = pos->hashTable->TT[index].score;

				// Translate score to mate depth
				if (foundScore > ISMATE)
					foundDepth = ((INFINITE - foundScore) / 2) + 1;
				else if (foundScore < -ISMATE)
					foundDepth = (INFINITE + foundScore) / 2;
				else
					foundDepth = 0;

				// Extend search if shortest mate not found
				if (foundDepth != mateDepth && extensions < 8) {
					info->depth += 1;
					extensions += 1;
					goto search;
				}

				// Failed to find correct move and/or correct depth
				if (foundDepth != mateDepth) {
					printf("\n\nMateInXTest Fail: Wrong depth.\n%s", lineIn);
					printf("Line %d in file: %s\n", lineCnt, filename);
					printf("Mate depth should be %d, was %d.\n", mateDepth, foundDepth);
					failures += 1;
					getchar();
					continue;
				}

				// Clear lineIn for reuse
				memset(&lineIn[0], 0, sizeof(lineIn));
			}
		}
	}
	printf("MateInXTest Complete!\n Failures: %d\n\n", failures);
	fclose(file);
}
#endif