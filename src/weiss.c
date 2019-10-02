// weiss.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "hashkeys.h"
#include "movegen.h"
#include "pvtable.h"
#include "uci.h"

#ifdef CLI
#include "cli.h"
#endif

void InitAll() {
	InitDistance();
	InitBitMasks();
	InitHashKeys();
	InitEvalMasks();
	InitMvvLva();
	InitAttacks();
}

int main() {

	InitAll();

	S_BOARD pos[1];
	S_SEARCHINFO info[1];
	info->quit = false;
	pos->hashTable->pTable = NULL;
	InitHashTable(pos->hashTable, DEFAULTHASH);

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	printf("\nWelcome to weiss! Type 'weiss' for console mode.\n");

	char line[256];
	while (true) {
		memset(&line[0], 0, sizeof(line));

		fflush(stdout);
		if (!fgets(line, 256, stdin))
			continue;

		if (!strncmp(line, "uci", 3)) {
			Uci_Loop(pos, info);
			if (info->quit)
				break;
			continue;
		}
#ifdef CLI
		if (!strncmp(line, "weiss", 5)) {
			Console_Loop(pos, info);
			if (info->quit)
				break;
			continue;
		}
#endif
		if (!strncmp(line, "quit", 4))
			break;

		else
			printf("Invalid command. Type 'weiss' for console mode.\n> ");
	}

	free(pos->hashTable->pTable);
	return 0;
}