// uci.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "board.h"
#include "io.h"
#include "makemove.h"
#include "misc.h"
#include "move.h"
#include "pvtable.h"
#include "search.h"
#include "validate.h"


#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define INPUTBUFFER 400 * 6


static void ParseGo(const char *line, S_SEARCHINFO *info, S_BOARD *pos) {

	info->starttime = GetTimeMs();

	int moveOverhead = 50;
	int depth = -1, movestogo = 30, movetime = -1;
	int time = -1, inc = 0;
	char *ptr = NULL;
	info->timeset = false;

	if ((ptr = strstr(line, "infinite"))) {
		;
	}

	if ((ptr = strstr(line, "binc")) && pos->side == BLACK)
		inc = atoi(ptr + 5);

	if ((ptr = strstr(line, "winc")) && pos->side == WHITE)
		inc = atoi(ptr + 5);

	if ((ptr = strstr(line, "wtime")) && pos->side == WHITE)
		time = atoi(ptr + 6);

	if ((ptr = strstr(line, "btime")) && pos->side == BLACK)
		time = atoi(ptr + 6);

	if ((ptr = strstr(line, "movestogo")))
		movestogo = atoi(ptr + 10);

	if ((ptr = strstr(line, "movetime")))
		movetime = atoi(ptr + 9);

	if ((ptr = strstr(line, "depth")))
		depth = atoi(ptr + 6);

	if (movetime != -1) {
		time = movetime;
		movestogo = 1;
	}

	info->depth = depth == -1 ? MAXDEPTH : depth;

	if (time != -1) {
		info->timeset = true;
		int timeThisMove = (time / movestogo) + inc;	// Try to use 1/30 of remaining time + increment
		int maxTime = time; 							// Most time we can use
		info->stoptime = info->starttime;
		info->stoptime += timeThisMove > maxTime ? maxTime : timeThisMove;
		info->stoptime -= moveOverhead;
	}

	SearchPosition(pos, info);
}

static void ParsePosition(const char *lineIn, S_BOARD *pos) {

	lineIn += 9;
	const char *ptrChar = lineIn;

	if (strncmp(lineIn, "startpos", 8) == 0) {
		ParseFen(START_FEN, pos);
	} else {
		ptrChar = strstr(lineIn, "fen");
		if (ptrChar == NULL) {
			ParseFen(START_FEN, pos);
		} else {
			ptrChar += 4;
			ParseFen(ptrChar, pos);
		}
	}

	ptrChar = strstr(lineIn, "moves");
	int move;

	if (ptrChar != NULL) {
		ptrChar += 6;
		while (*ptrChar) {
			move = ParseMove(ptrChar, pos);
			if (move == NOMOVE)
				break;
			MakeMove(pos, move);
			pos->ply = 0;
			while (*ptrChar && *ptrChar != ' ')
				ptrChar++;
			ptrChar++;
		}
	}
}

void Uci_Loop(S_BOARD *pos, S_SEARCHINFO *info) {

	info->GAME_MODE = UCIMODE;

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	char line[INPUTBUFFER];
	printf("id name %s\n", NAME);
	printf("id author LoliSquad\n");
	printf("option name Hash type spin default %d min 4 max %d\n", DEFAULTHASH, MAXHASH);
	printf("option name Ponder type check default false\n"); // Turn on ponder stats in cutechess
#ifdef USE_TBS
	printf("option name SyzygyPath type string default <empty>\n");
#endif
	printf("uciok\n");

	int MB = DEFAULTHASH;
	int newMB;

	while (true) {

		memset(&line[0], 0, sizeof(line));
		fflush(stdout);

		if (!fgets(line, INPUTBUFFER, stdin))
			continue;

		if (!strncmp(line, "go", 2))
			ParseGo(line, info, pos);

		else if (line[0] == '\n')
			continue;

		else if (!strncmp(line, "isready", 7))
			printf("readyok\n");

		else if (!strncmp(line, "position", 8))
			ParsePosition(line, pos);

		else if (!strncmp(line, "ucinewgame", 10))
			ParsePosition("position startpos\n", pos);

		else if (!strncmp(line, "quit", 4)) {
			info->quit = true;
			break;

		} else if (!strncmp(line, "uci", 3)) {
			printf("id name %s\n", NAME);
			printf("id author LoliSquad\n");
			printf("uciok\n");

		} else if (!strncmp(line, "setoption name Hash value ", 26)) {

			sscanf(line, "%*s %*s %*s %*s %d", &newMB);
			if (newMB == MB) continue; // Ignore if same as before
			MB = newMB;
			if (MB < 4) MB = 4;
			if (MB > MAXHASH) MB = MAXHASH;
			printf("Set Hash to %d MB\n", MB);
			InitHashTable(pos->HashTable, MB);
#ifdef USE_TBS
		} else if (!strncmp(line, "setoption name SyzygyPath value ", 32)) {

			char *path = line + strlen("setoption name SyzygyPath value ");

			// Replace newline with null
			char *newline;
			if ((newline = strchr(path, '\n')))
				path[newline-path] = '\0';

			strcpy(info->syzygyPath, path);
			tb_init(info->syzygyPath);
#endif
		}
		if (info->quit) break;
	}
}