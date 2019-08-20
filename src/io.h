// io.c

#pragma once

#include "types.h"


char *MoveToStr(const int move);
int ParseMove(char *ptrChar, S_BOARD *pos);
void PrintThinking(const S_SEARCHINFO *info, const S_BOARD *pos, int bestScore, int currentDepth, int pvMoves);