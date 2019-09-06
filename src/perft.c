// perft.c

#include <stdio.h>

#include "board.h"
#include "io.h"
#include "makemove.h"
#include "movegen.h"
#include "misc.h"


uint64_t leafNodes;


static void RecursivePerft(int depth, S_BOARD *pos) {

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

void Perft(int depth, S_BOARD *pos) {

	assert(CheckBoard(pos));

	printf("\nStarting Test To Depth:%d\n\n", depth);

	int start = GetTimeMs();

	leafNodes = 0;
	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int move;
	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {
		move = list->moves[MoveNum].move;

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