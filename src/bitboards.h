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


extern bitboard   SetMask[64];
extern bitboard ClearMask[64];

extern bitboard FileBBMask[8];
extern bitboard RankBBMask[8];

const bitboard fileBBs[8];
const bitboard rankBBs[8];

void InitDistance();
int Distance(int sq1, int sq2);
int PopCount(bitboard x);
int Lsb(bitboard bb);
int PopLsb(bitboard *bb);
void PrintBB(bitboard bb);
