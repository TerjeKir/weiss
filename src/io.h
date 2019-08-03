// io.c

#pragma once

#include "defs.h"

char *PrMove(const int move);
char *PrSq(const int sq);
int ParseMove(char *ptrChar, S_BOARD *pos);