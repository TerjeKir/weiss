// movegen.c

#pragma once

#include "defs.h"


void InitMvvLva();
int MoveExists(S_BOARD *pos, const int move);
void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list);
void GenerateAllCaptures(const S_BOARD *pos, S_MOVELIST *list);