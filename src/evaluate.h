// evaluate.h

#pragma once

#include "types.h"


// Check for (likely) material draw
#define CHECK_MAT_DRAW


#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define S(mg, eg) MakeScore((mg), (eg))
#define MgScore(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define EgScore(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))


typedef struct EvalInfo {

    Bitboard mobilityArea[2];

} EvalInfo;


int EvalPosition(const Position *pos);