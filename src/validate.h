//validate.c

#pragma once

#include "types.h"


int MoveListOk(const S_MOVELIST *list,  const S_BOARD *pos);
int ValidSquare(const int sq);
int SideValid(const int side);
int FileRankValid(const int fr);
int PieceValidEmpty(const int pce);
int PieceValid(const int pce);
void MirrorEvalTest(S_BOARD *pos);
void MateInXTest(S_BOARD *pos);