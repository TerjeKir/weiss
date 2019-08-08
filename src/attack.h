// attack.h

#pragma once

#include "defs.h"

uint64_t king_attacks[64];
uint64_t knight_attacks[64];
uint64_t pawn_attacks[2][64];

void InitAttacks();

int SqAttacked(const int sq, const int side, const S_BOARD *pos);