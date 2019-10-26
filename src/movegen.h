// movegen.h

#pragma once

#include "types.h"


void GenAllMoves(const Position *pos, MoveList *list);
void GenNoisyMoves(const Position *pos, MoveList *list);
void GenQuietMoves(const Position *pos, MoveList *list);