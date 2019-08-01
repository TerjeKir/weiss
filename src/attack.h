// attack.h

#pragma once

#include "defs.h"

void InitAttacks();
void InitKingAttacks();
void InitKnightAttacks();
void InitPawnAttacks();
void InitSliderAttacks();

extern int SqAttacked(const int sq, const int side, const S_BOARD *pos);