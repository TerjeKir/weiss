//validate.h

#pragma once

#include "types.h"


#ifndef NDEBUG
int ValidSquare(const int sq);
int ValidSide(const int side);
int ValidPiece(const int pce);
int ValidPieceOrEmpty(const int pce);
int MoveListOk(const S_MOVELIST *list,  const S_BOARD *pos);
#endif