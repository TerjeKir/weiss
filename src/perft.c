// perft.c

#include <stdio.h>

#include "defs.h"
#include "board.h"
#include "io.h"
#include "makemove.h"
#include "movegen.h"
#include "misc.h"

uint64_t leafNodes;

static void Perft(int depth, S_BOARD *pos) {

	assert(CheckBoard(pos));

	if (depth == 0) {
		leafNodes++;
		return;
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int MoveNum = 0;
	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue;
		}
		Perft(depth - 1, pos);
		TakeMove(pos);
	}

	return;
}

void PerftTest(int depth, S_BOARD *pos) {

	assert(CheckBoard(pos));
	
	printf("\nStarting Test To Depth:%d\n", depth);

	int start = GetTimeMs();

	leafNodes = 0;
	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int move;
	int MoveNum = 0;
	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
		move = list->moves[MoveNum].move;
		if (!MakeMove(pos, move)) {
			continue;
		}
		uint64_t cumnodes = leafNodes;
		Perft(depth - 1, pos);
		TakeMove(pos);
		uint64_t oldnodes = leafNodes - cumnodes;
		printf("move %d : %s : %I64d\n", MoveNum + 1, PrMove(move), oldnodes);
	}

	int timeElapsed = GetTimeMs() - start;
	printf("\nPerft Complete : %I64d nodes visited in %dms\n", leafNodes, timeElapsed);
	printf("               : %.0f nps\n", leafNodes / ((double) timeElapsed / 1000));

	return;
}
