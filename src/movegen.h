// movegen.h

#pragma once

#include "types.h"


void InitMvvLva();

void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list);
void GenerateAllCaptures(const S_BOARD *pos, S_MOVELIST *list);
int MoveExists(S_BOARD *pos, const int move);