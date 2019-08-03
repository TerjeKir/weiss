// board.h

#pragma once

#include "defs.h"


int CheckBoard(const S_BOARD *pos);
int ParseFen(char *fen, S_BOARD *pos);
void PrintBoard(const S_BOARD *pos);
void MirrorBoard(S_BOARD *pos);