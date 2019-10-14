// movegen.h

#pragma once

#include "types.h"


void GenAllMoves(const S_BOARD *pos, S_MOVELIST *list);
void GenNoisyMoves(const S_BOARD *pos, S_MOVELIST *list);
void GenQuietMoves(const S_BOARD *pos, S_MOVELIST *list);
bool MoveExists(S_BOARD *pos, const int move);