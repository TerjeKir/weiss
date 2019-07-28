// bitboards.h

#pragma once

#include "types.h"

#define POP(b) PopBit(b)
#define CNT(b) PopCount(b)

extern void PrintBitBoard(uint64_t bb);
extern int PopBit(uint64_t *bb);
extern int PopCount(uint64_t x);
