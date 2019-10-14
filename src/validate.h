//validate.h

#pragma once

#include "types.h"


#ifndef NDEBUG
bool ValidSquare(const int sq);
bool ValidSide(const int side);
bool ValidPiece(const int piece);
bool ValidPieceOrEmpty(const int piece);
bool MoveListOk(const S_MOVELIST *list,  const S_BOARD *pos);
#endif