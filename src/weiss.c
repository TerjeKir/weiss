// weiss.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transposition.h"
#include "uci.h"

#ifdef CLI
#include "cli.h"
#endif

int main() {

	S_BOARD pos[1];
	S_SEARCHINFO info[1];
	info->quit = false;
	pos->hashTable->TT = NULL;
	InitHashTable(pos->hashTable, DEFAULTHASH);

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	printf("\nWelcome to weiss! Type 'weiss' for console mode.\n");

	char line[256];
	while (fgets(line, 256, stdin)) {

		if (!strncmp(line, "uci", 3)) {
			Uci_Loop(pos, info);
			break;
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

		printf("Invalid command. Type 'weiss' for console mode.\n> ");

		memset(&line[0], 0, sizeof(line));
	}

	free(pos->hashTable->TT);
	return 0;
}