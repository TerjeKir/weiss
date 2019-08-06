// cli.c

#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "evaluate.h"
#include "io.h"
#include "makemove.h"
#include "movegen.h"
#include "misc.h"
#include "pvtable.h"
#include "search.h"
#include "perft.h"

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

	if (pos->pieceBBs[0])
		return FALSE;
	if (pos->pieceBBs[3] || pos->pieceBBs[4])
		return FALSE;
	if (PopCount(pos->colors[WHITE] & pos->pieceBBs[1]) > 1 || PopCount(pos->colors[BLACK] & pos->pieceBBs[1]) > 1)
		return FALSE;
	if (PopCount(pos->colors[WHITE] & pos->pieceBBs[2]) > 1 || PopCount(pos->colors[BLACK] & pos->pieceBBs[2]) > 1)
		return FALSE;
	if ((pos->colors[WHITE] & pos->pieceBBs[1]) && (pos->colors[WHITE] & pos->pieceBBs[2]))
		return FALSE;
	if ((pos->colors[BLACK] & pos->pieceBBs[1]) && (pos->colors[BLACK] & pos->pieceBBs[2]))
		return FALSE;

	return TRUE;
}

static int checkresult(S_BOARD *pos) {
	assert(CheckBoard(pos));

	if (pos->fiftyMove > 100) {
		printf("1/2-1/2 {fifty move rule (claimed by weiss)}\n");
		return TRUE;
	}

	if (ThreeFoldRep(pos) >= 2) {
		printf("1/2-1/2 {3-fold repetition (claimed by weiss)}\n");
		return TRUE;
	}

	if (DrawMaterial(pos)) {
		printf("1/2-1/2 {insufficient material (claimed by weiss)}\n");
		return TRUE;
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int MoveNum = 0;
	int found = 0;
	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		found++;
		TakeMove(pos);
		break;
	}

	if (found != 0)
		return FALSE;

	int InCheck = SqAttacked(pos->colors[pos->side] & pos->pieceBBs[5], !pos->side, pos);

	if (InCheck) {
		if (pos->side == WHITE) {
			printf("0-1 {black mates (claimed by weiss)}\n");
			return TRUE;
		} else {
			printf("0-1 {white mates (claimed by weiss)}\n");
			return TRUE;
		}
	} else {
		printf("\n1/2-1/2 {stalemate (claimed by weiss)}\n");
		return TRUE;
	}
	return FALSE;
}

void Console_Loop(S_BOARD *pos, S_SEARCHINFO *info) {

	printf("\nweiss started in console mode\n");
	printf("Type help for commands\n\n");

	info->GAME_MODE = CONSOLEMODE;
	info->POST_THINKING = TRUE;
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

	while (TRUE) {

		fflush(stdout);

		if (pos->side == engineSide && checkresult(pos) == FALSE) {
			info->starttime = GetTimeMs();
			info->depth = depth;

			if (movetime != 0) {
				info->timeset = TRUE;
				info->stoptime = info->starttime + movetime;
			}

			SearchPosition(pos, info);
		}

		printf("\nweiss > ");

		fflush(stdout);

		memset(&inBuf[0], 0, sizeof(inBuf));
		fflush(stdout);
		if (!fgets(inBuf, 80, stdin))
			continue;

		sscanf(inBuf, "%s", command);

		if (!strcmp(command, "help")) {
			printf("Commands:\n");
			printf("quit - quit game\n");
			printf("force - computer will not think\n");
			printf("print - show board\n");
			printf("post - show thinking\n");
			printf("nopost - do not show thinking\n");
			printf("new - start new game\n");
			printf("go - set computer thinking\n");
			printf("depth x - set depth to x\n");
			printf("time x - set thinking time to x seconds (depth still applies if set)\n");
			printf("view - show current depth and movetime settings\n");
			printf("setboard x - set position to fen x\n");
			printf("** note ** - to reset time and depth, set to 0\n");
			printf("enter moves using b7b8q notation\n\n\n");
			continue;
		}

		if (!strcmp(command, "mirror")) {
			engineSide = BOTH;
			MirrorEvalTest(pos);
			continue;
		}

		if (!strcmp(command, "eval")) {
			PrintBoard(pos);
			printf("Eval:%d", EvalPosition(pos));
			MirrorBoard(pos);
			PrintBoard(pos);
			printf("Eval:%d", EvalPosition(pos));
			continue;
		}

		if (!strcmp(command, "setboard")) {
			engineSide = BOTH;
			ParseFen(inBuf + 9, pos);
			continue;
		}

		if (!strcmp(command, "quit")) {
			info->quit = TRUE;
			break;
		}

		if (!strcmp(command, "post")) {
			info->POST_THINKING = TRUE;
			continue;
		}

		if (!strcmp(command, "print")) {
			PrintBoard(pos);
			continue;
		}

		if (!strcmp(command, "nopost")) {
			info->POST_THINKING = FALSE;
			continue;
		}

		if (!strcmp(command, "force")) {
			engineSide = BOTH;
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

		if (!strcmp(command, "new")) {
			ClearHashTable(pos->HashTable);
			engineSide = BLACK;
			ParseFen(START_FEN, pos);
			continue;
		}

		if (!strcmp(command, "go")) {
			engineSide = pos->side;
			continue;
		}

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

		move = ParseMove(inBuf, pos);
		if (move == NOMOVE) {
			printf("Command unknown:%s\n", inBuf);
			continue;
		}
		MakeMove(pos, move);
		pos->ply = 0;
	}
}
