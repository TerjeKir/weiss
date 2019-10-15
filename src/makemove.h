// makemove.h

#pragma once

#include "types.h"


bool MakeMove(Position *pos, const int move);
void TakeMove(Position *pos);
void MakeNullMove(Position *pos);
void TakeNullMove(Position *pos);