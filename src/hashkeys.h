// hashkeys.c

#pragma once

#include "types.h"


// Zobrist keys
extern uint64_t PieceKeys[13][64];
extern uint64_t SideKey;
extern uint64_t CastleKeys[16];


void InitHashKeys();

uint64_t GeneratePosKey(const S_BOARD *pos);