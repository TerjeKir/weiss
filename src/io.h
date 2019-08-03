// io.c

#pragma once

#include "defs.h"

char *PrMove(const int move);
char *PrSq(const int sq);
void PrintMoveList(const S_MOVELIST *list);
int ParseMove(char *ptrChar, S_BOARD *pos);