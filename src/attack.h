// attack.h

#pragma once

#include "defs.h"

bitboard king_attacks[64];
bitboard knight_attacks[64];
bitboard pawn_attacks[2][64];

void InitAttacks();

int SqAttacked(const int sq, const int side, const S_BOARD *pos);