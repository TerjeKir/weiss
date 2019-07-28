// io.c

#pragma once

#include "defs.h"

extern char *PrMove(const int move);
extern char *PrSq(const int sq);
extern void PrintMoveList(const S_MOVELIST *list);
extern int ParseMove(char *ptrChar, S_BOARD *pos);