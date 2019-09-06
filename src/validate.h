//validate.c

#pragma once

#include "types.h"


int ValidSquare(const int sq);
int SideValid(const int side);
int PieceValid(const int pce);
int PieceValidEmpty(const int pce);
int MoveListOk(const S_MOVELIST *list,  const S_BOARD *pos);
void MirrorEvalTest(S_BOARD *pos);
void MateInXTest(S_BOARD *pos);