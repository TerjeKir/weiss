// weiss.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "init.h"
#include "polybook.h"
#include "pvtable.h"
#include "uci.h"
#include "xboard.h"

int main(int argc, char *argv[]) {

	InitAll();

	S_BOARD pos[1];
	S_SEARCHINFO info[1];
	info->quit = FALSE;
	pos->HashTable->pTable = NULL;
	InitHashTable(pos->HashTable, 64);
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	int ArgNum = 0;

	for (ArgNum = 0; ArgNum < argc; ++ArgNum) {
		if (strncmp(argv[ArgNum], "NoBook", 6) == 0) {
			EngineOptions->UseBook = FALSE;
			printf("Book Off\n");
		}
	}

	printf("Welcome to weiss! Type 'weiss' for console mode.\n> ");

	char line[256];
	while (TRUE) {
		memset(&line[0], 0, sizeof(line));

		fflush(stdout);
		if (!fgets(line, 256, stdin))
			continue;
		if (!strncmp(line, "uci", 3)) {
			Uci_Loop(pos, info);
			if (info->quit)
				break;
			continue;
		} else if (!strncmp(line, "xboard", 6)) {
			XBoard_Loop(pos, info);
			if (info->quit)
				break;
			continue;
		} else if (!strncmp(line, "weiss", 5)) {
			Console_Loop(pos, info);
			if (info->quit)
				break;
			continue;
		} else if (!strncmp(line, "quit", 4)) {
			break;
		} else {
			printf("Invalid command. Type 'weiss' for console mode.\n> ");
		}
	}

	free(pos->HashTable->pTable);
	CleanPolyBook();
	return 0;
}
