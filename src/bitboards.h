// bitboards.h

#pragma once

#include "types.h"

#define POP(b) PopBit(b)
#define CNT(b) PopCount(b)

void PrintBB(uint64_t bb);
int PopBit(uint64_t *bb);
int PopCount(uint64_t x);
