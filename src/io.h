// io.h

#pragma once

#include "types.h"


char *MoveToStr(const int move);
int ParseMove(const char *ptrChar, Position *pos);
#ifdef CLI
int ParseEPDMove(const char *ptrChar, Position *pos);
#endif