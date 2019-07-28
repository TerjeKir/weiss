// bitboards.h

#include "types.h"

#define POP(b) PopBit(b)
#define CNT(b) PopCount(b)

extern void PrintBitBoard(U64 bb);
extern int PopBit(U64 *bb);
extern int PopCount(U64 x);
