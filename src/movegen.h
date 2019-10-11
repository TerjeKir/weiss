// movegen.h

#pragma once

#include "types.h"


void GenAllMoves(const S_BOARD *pos, S_MOVELIST *list);
void GenNoisyMoves(const S_BOARD *pos, S_MOVELIST *list);
int MoveExists(S_BOARD *pos, const int move);