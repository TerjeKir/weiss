// hashkeys.h

#pragma once

#include "types.h"


// Zobrist keys
extern uint64_t PieceKeys[PIECE_NB][64];
extern uint64_t CastleKeys[16];
extern uint64_t SideKey;


uint64_t GeneratePosKey(const Position *pos);