//validate.c

#pragma once

#include "defs.h"


int MoveListOk(const S_MOVELIST *list,  const S_BOARD *pos);
int ValidSquare(const int sq);
int SqOnBoard(const int sq);
int SideValid(const int side);
int FileRankValid(const int fr);
int PceValidEmptyOffbrd(const int pce);
int PieceValidEmpty(const int pce);
int PieceValid(const int pce);
void MirrorEvalTest(S_BOARD *pos);