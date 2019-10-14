// makemove.h

#pragma once

#include "types.h"


bool MakeMove(S_BOARD *pos, const int move);
void TakeMove(S_BOARD *pos);
void MakeNullMove(S_BOARD *pos);
void TakeNullMove(S_BOARD *pos);