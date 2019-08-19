// board.h

#pragma once

#include "types.h"


void InitDistance();
int Distance(int sq1, int sq2);
int CheckBoard(const S_BOARD *pos);
int ParseFen(char *fen, S_BOARD *pos);
void PrintBoard(const S_BOARD *pos);
void MirrorBoard(S_BOARD *pos);