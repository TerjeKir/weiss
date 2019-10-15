// io.h

#pragma once

#include "types.h"


char *MoveToStr(const int move);
void PrintThinking(const SearchInfo *info, Position *pos, const int bestScore, const int currentDepth);
void PrintConclusion(const Position *pos);
int ParseMove(const char *ptrChar, Position *pos);
#ifdef CLI
int ParseEPDMove(const char *ptrChar, Position *pos);
#endif