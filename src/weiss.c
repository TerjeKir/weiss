// weiss.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "init.h"
#include "pvtable.h"
#include "uci.h"


int main() {

	InitAll();

	S_BOARD pos[1];
	S_SEARCHINFO info[1];
	info->quit = FALSE;
	pos->HashTable->pTable = NULL;
	InitHashTable(pos->HashTable, DEFAULTHASH);

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	printf("\nWelcome to weiss! Type 'weiss' for console mode.\n> ");

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
		} 
		
		if (!strncmp(line, "weiss", 5)) {
			Console_Loop(pos, info);
			if (info->quit)
				break;
			continue;
		} 
		
		if (!strncmp(line, "quit", 4))
			break;

		else
			printf("Invalid command. Type 'weiss' for console mode.\n> ");
	}

	free(pos->HashTable->pTable);
	return 0;
}
