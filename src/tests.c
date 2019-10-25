// tests.c

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "misc.h"
#include "move.h"
#include "movegen.h"
#include "search.h"
#include "transposition.h"


/* Benchmark heavily inspired by Ethereal*/
static const char *BenchmarkFENs[] = {
	#include "bench.csv"
	""
};

void benchmark(int depth, Position *pos, SearchInfo *info) {

	uint64_t nodes = 0ULL;

	info->depth = depth;
	info->timeset = false;

	int startTime = GetTimeMs();

	for (int i = 0; strcmp(BenchmarkFENs[i], ""); ++i) {
		printf("Bench %d: %s\n", i + 1, BenchmarkFENs[i]);
		ParseFen(BenchmarkFENs[i], pos);
		info->starttime = GetTimeMs();
		SearchPosition(pos, info);
		nodes += info->nodes;
		ClearHashTable(pos->hashTable);
	}

	int elapsed = GetTimeMs() - startTime;

	printf("Benchmark complete:\n");
	printf("Time : %dms\n", elapsed);
	printf("Nodes: %" PRId64 "\n", nodes);
	printf("NPS  : %" PRId64 "\n", 1000 * nodes / (1 + elapsed));
}

#ifdef DEV

/* Perft */
static uint64_t leafNodes;

static void RecursivePerft(const int depth, Position *pos) {

	assert(CheckBoard(pos));

	if (depth == 0) {
		leafNodes++;
		return;
	}

	MoveList list[1];
	GenAllMoves(pos, list);

	for (unsigned int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		RecursivePerft(depth - 1, pos);
		TakeMove(pos);
	}

	return;
}

// Counts number of moves that can be made in a position to some depth
void Perft(const int depth, Position *pos) {

	assert(CheckBoard(pos));

	printf("\nStarting Test To Depth:%d\n\n", depth);
	fflush(stdout);

	const int start = GetTimeMs();
	leafNodes = 0;

	MoveList list[1];
	GenAllMoves(pos, list);

	for (unsigned int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		int move = list->moves[MoveNum].move;

		if (!MakeMove(pos, move)){
			printf("move %d : %s : Illegal\n", MoveNum + 1, MoveToStr(move));
			fflush(stdout);
			continue;
		}

		uint64_t oldCount = leafNodes;
		RecursivePerft(depth - 1, pos);
		uint64_t newNodes = leafNodes - oldCount;
		printf("move %d : %s : %" PRId64 "\n", MoveNum + 1, MoveToStr(move), newNodes);
		fflush(stdout);

		TakeMove(pos);
	}

	const int timeElapsed = GetTimeMs() - start;
	printf("\nPerft Complete : %" PRId64 " nodes visited in %dms\n", leafNodes, timeElapsed);
	if (timeElapsed > 0)
		printf("               : %" PRId64 " nps\n", ((leafNodes * 1000) / timeElapsed));
	fflush(stdout);

	return;
}

// Checks evaluation is symmetric
void MirrorEvalTest(Position *pos) {

	const char filename[] = "../EPDs/all.epd";

	FILE *file;
	if ((file = fopen(filename, "r")) == NULL) {
		printf("MirrorEvalTest: %s not found.\n", filename);
		fflush(stdout);
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
			printf("\n\nMirrorEvalTest Fail: %d - %d\n%s\n", ev1, ev2, lineIn);
			fflush(stdout);
			return;
		}

		if (positions % 100 == 0) {
			printf("position %d\n", positions);
			fflush(stdout);
		}

		memset(&lineIn[0], 0, sizeof(lineIn));
	}
	printf("\n\nMirrorEvalTest Successful\n\n");
	fflush(stdout);
	fclose(file);
}

// Checks engine can find mates
void MateInXTest(Position *pos) {

	char filename[] = "../EPDs/mate_-_.epd"; 				// '_'s are placeholders
	FILE *file;

	int failures = 0;

	SearchInfo info[1];
	char lineIn[1024];
	char *bm, *ce;

	int bestMoves[20];
	int foundScore;
	int correct, bmCnt, bestFound, extensions, mateDepth, foundDepth;

	for (char depth = '1'; depth < '9'; ++depth) {
		filename[12] = depth;
		for (char side = 'b'; side >= 'b'; side += 21) { 	// Alternates 'b' <-> 'w', overflowing goes below 'b'
			filename[14] = side;

			if ((file = fopen(filename, "r")) == NULL) {
				printf("MateInXTest: %s not found.\n", filename);
				fflush(stdout);
				return;
			}

			int lineCnt = 0;

			while (fgets(lineIn, 1024, file) != NULL) {

				// Reset
				memset(bestMoves, 0, sizeof(bestMoves));
				memset(info, 0, sizeof(info));
				foundScore = 0;
				bestFound = 0;

				// Read next line
				ParseFen(lineIn, pos);
				lineCnt++;
				printf("Line %d in file: %s\n", lineCnt, filename);
				fflush(stdout);

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
						fflush(stdout);
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
					fflush(stdout);
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
					fflush(stdout);
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
	fflush(stdout);
	fclose(file);
}
#endif