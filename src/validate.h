//validate.h

#pragma once

#include "types.h"


#ifndef NDEBUG
bool ValidSquare(int sq);
bool ValidSide(int side);
bool ValidPiece(int piece);
bool MoveListOk(const MoveList *list, const Position *pos);
#endif