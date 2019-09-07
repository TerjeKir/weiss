// bitboards.h

#pragma once

#include "types.h"


#define SETBIT(bb, sq) ((bb) |= SetMask[(sq)])
#define CLRBIT(bb, sq) ((bb) &= ClearMask[(sq)])


enum {
    fileABB = 0x0101010101010101ULL,
    fileBBB = 0x0202020202020202ULL,
    fileCBB = 0x0404040404040404ULL,
    fileDBB = 0x0808080808080808ULL,
    fileEBB = 0x1010101010101010ULL,
    fileFBB = 0x2020202020202020ULL,
    fileGBB = 0x4040404040404040ULL,
    fileHBB = 0x8080808080808080ULL,

    rank1BB = 0xFF,
    rank2BB = 0xFF00,
    rank3BB = 0xFF0000,
    rank4BB = 0xFF000000,
    rank5BB = 0xFF00000000,
    rank6BB = 0xFF0000000000,
    rank7BB = 0xFF000000000000,
    rank8BB = 0xFF00000000000000,
};


bitboard   SetMask[64];
bitboard ClearMask[64];

const bitboard fileBBs[8];
const bitboard rankBBs[8];

void InitBitMasks();
// void PrintBB(const bitboard bb);

// Population count/Hamming weight
static inline int PopCount(bitboard bb) {

    return __builtin_popcountll(bb);
}

// Returns the index of the least significant bit
static inline int Lsb(const bitboard bb) {

    return __builtin_ctzll(bb);
}

// Returns the index of the least significant bit and unsets it
static inline int PopLsb(bitboard *bb) {

	int lsb = Lsb(*bb);
	*bb &= (*bb - 1);

	return lsb;
}