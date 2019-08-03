// board.h

#pragma once

#include "defs.h"

void ResetBoard(S_BOARD *pos);
int ParseFen(char *fen, S_BOARD *pos);
void PrintBoard(const S_BOARD *pos);
void UpdateListsMaterial(S_BOARD *pos);
int CheckBoard(const S_BOARD *pos);
void MirrorBoard(S_BOARD *pos);