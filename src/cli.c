// cli.c

#ifdef CLI

#include <stdio.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "io.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "misc.h"
#include "transposition.h"
#include "search.h"
#include "tests.h"


#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"


static int ThreeFoldRep(const S_BOARD *pos) {

	assert(CheckBoard(pos));

	int repeats = 0;

	for (int i = 0; i < pos->hisPly; ++i)
		if (pos->history[i].posKey == pos->posKey)
			repeats++;

	return repeats;
}

static int DrawMaterial(const S_BOARD *pos) {
	assert(CheckBoard(pos));

	// Pawns can promote to pieces that can mate
	if (pos->pieceBBs[PAWN])
		return false;
	// Rooks and queens can mate
	if (pos->pieceBBs[ROOK] || pos->pieceBBs[QUEEN])
		return false;
	// 3 knights can mate
	if (PopCount(pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT]) > 2 || PopCount(pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT]) > 2)
		return false;
	// 2 bishops can mate
	if (PopCount(pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP]) > 1 || PopCount(pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP]) > 1)
		return false;
	// Bishop + Knight can mate
	if ((pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT]) && (pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP]))
		return false;
	if ((pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT]) && (pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP]))
		return false;

	return true;
}

static int checkresult(S_BOARD *pos) {
	assert(CheckBoard(pos));

	if (pos->fiftyMove > 100) {
		printf("1/2-1/2 {fifty move rule (claimed by weiss)}\n");
		return true;
	}

	if (ThreeFoldRep(pos) >= 2) {
		printf("1/2-1/2 {3-fold repetition (claimed by weiss)}\n");
		return true;
	}

	if (DrawMaterial(pos)) {
		printf("1/2-1/2 {insufficient material (claimed by weiss)}\n");
		return true;
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int found = 0;
	for (unsigned int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		found++;
		TakeMove(pos);
		break;
	}

	if (found != 0)
		return false;

	int InCheck = SqAttacked(pos->kingSq[pos->side], !pos->side, pos);

	if (InCheck) {
		if (pos->side == WHITE) {
			printf("0-1 {black mates (claimed by weiss)}\n");
			return true;
		} else {
			printf("0-1 {white mates (claimed by weiss)}\n");
			return true;
		}
	} else {
		printf("\n1/2-1/2 {stalemate (claimed by weiss)}\n");
		return true;
	}
	return false;
}

void Console_Loop(S_BOARD *pos, S_SEARCHINFO *info) {

	printf("\nweiss started in console mode\n");
	printf("Type help for commands\n\n");

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	int depth = MAXDEPTH, movetime = 3000;
	int engineSide = BLACK;
	int move = NOMOVE;
	char inBuf[80], command[80];

	ParseFen(START_FEN, pos);

	// Perft vars
	S_BOARD board[1];
	int perftDepth = 3;

#ifdef USE_TBS
	tb_init("F:\\Syzygy");
#endif

	while (true) {

		if (pos->side == engineSide && checkresult(pos) == false) {
			info->starttime = GetTimeMs();
			info->depth = depth;

			if (movetime != 0) {
				info->timeset = true;
				info->stoptime = info->starttime + movetime;
			}

			SearchPosition(pos, info);
			PrintBoard(pos);
		}

		printf("\nweiss > ");

		memset(&inBuf[0], 0, sizeof(inBuf));

		if (!fgets(inBuf, 80, stdin))
			continue;

		sscanf(inBuf, "%s", command);

		// Basic commands

		if (!strcmp(command, "help")) {
			printf("Commands:\n");
			printf("quit - quit game\n");
			printf("force - computer will not think\n");
			printf("print - show board\n");
			printf("new - start new game\n");
			printf("go - set computer thinking\n");
			printf("depth x - set depth to x\n");
			printf("time x - set thinking time to x seconds (depth still applies if set)\n");
			printf("view - show current depth and movetime settings\n");
			printf("setboard x - set position to fen x\n");
			printf("** note ** - to reset time and depth, set to 0\n");
			printf("enter moves using b7b8q notation\n");
			continue;
		}

		if (!strcmp(command, "quit")) {
			info->quit = true;
			break;
		}

		// Search and make a move

		if (!strcmp(command, "go")) {
			engineSide = pos->side;
			continue;
		}

		// Engine settings

		if (!strcmp(command, "force")) {
			engineSide = BOTH;
			continue;
		}

		if (!strcmp(command, "depth")) {
			sscanf(inBuf, "depth %d", &depth);
			if (depth == 0)
				depth = MAXDEPTH;
			continue;
		}

		if (!strcmp(command, "time")) {
			sscanf(inBuf, "time %d", &movetime);
			movetime *= 1000;
			continue;
		}

		// Set gamestate

		if (!strcmp(command, "position")) {
			engineSide = BOTH;
			ParseFen(inBuf + 9, pos);
			continue;
		}

		if (!strcmp(command, "new")) {
			ClearHashTable(pos->hashTable);
			engineSide = BLACK;
			ParseFen(START_FEN, pos);
			continue;
		}

		// Show info about current state

		if (!strcmp(command, "eval")) {
			PrintBoard(pos);
			printf("Eval:%d", EvalPosition(pos));
			MirrorBoard(pos);
			PrintBoard(pos);
			printf("Eval:%d", EvalPosition(pos));
			continue;
		}

		if (!strcmp(command, "print")) {
			PrintBoard(pos);
			continue;
		}

		if (!strcmp(command, "view")) {
			if (depth == MAXDEPTH)
				printf("depth not set ");
			else
				printf("depth %d", depth);

			if (movetime != 0)
				printf(" movetime %ds\n", movetime / 1000);
			else
				printf(" movetime not set\n");

			continue;
		}

		// Tests

		if (!strcmp(command, "perft")) {

			sscanf(inBuf, "perft %d", &perftDepth);
			char *perftFen = inBuf + 8;
			
			if (!*perftFen)
				ParseFen(START_FEN, board);
			else
				ParseFen(perftFen, board);

			Perft(perftDepth, board);
			continue;
		}

		if (!strcmp(command, "mirrortest")) {
			engineSide = BOTH;
			MirrorEvalTest(pos);
			continue;
		}

		if (!strcmp(command, "matetest")) {
			engineSide = BOTH;
			MateInXTest(pos);
			continue;
		}

		// Parse user move

		move = ParseMove(inBuf, pos);
		if (move == NOMOVE) {
			printf("Command unknown:%s\n", inBuf);
			continue;
		}
		MakeMove(pos, move);
		pos->ply = 0;
	}
}
#endif