// makemove.h

#pragma once

#include "types.h"


bool MakeMove(Position *pos, int move);
void TakeMove(Position *pos);
void MakeNullMove(Position *pos);
void TakeNullMove(Position *pos);