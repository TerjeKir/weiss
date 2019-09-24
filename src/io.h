// io.h

#pragma once

#include "types.h"


char *MoveToStr(const int move);
void PrintThinking(const S_SEARCHINFO *info, S_BOARD *pos, const int bestScore, const int currentDepth);
int ParseMove(const char *ptrChar, S_BOARD *pos);
#ifdef CLI
int ParseEPDMove(const char *ptrChar, S_BOARD *pos);
#endif