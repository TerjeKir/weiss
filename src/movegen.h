// movegen.c

#pragma once

#include "defs.h"

void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list);
void GenerateAllCaps(const S_BOARD *pos, S_MOVELIST *list);
int MoveExists(S_BOARD *pos, const int move);
void InitMvvLva();