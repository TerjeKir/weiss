// perft.c

#include "defs.h"
#include "stdio.h"
#include "board.h"
#include "io.h"
#include "makemove.h"
#include "movegen.h"
#include "misc.h"

long leafNodes;

void Perft(int depth, S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

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

	ASSERT(CheckBoard(pos));

	PrintBoard(pos);
	printf("\nStarting Test To Depth:%d\n", depth);
	leafNodes = 0;
	int start = GetTimeMs();
	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int move;
	int MoveNum = 0;
	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
		move = list->moves[MoveNum].move;
		if (!MakeMove(pos, move)) {
			continue;
		}
		long cumnodes = leafNodes;
		Perft(depth - 1, pos);
		TakeMove(pos);
		long oldnodes = leafNodes - cumnodes;
		printf("move %d : %s : %ld\n", MoveNum + 1, PrMove(move), oldnodes);
	}

	printf("\nTest Complete : %ld nodes visited in %dms\n", leafNodes, GetTimeMs() - start);

	return;
}
