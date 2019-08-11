// bitboards.h

#pragma once

#include "types.h"


bitboard fileABB = 0x0101010101010101ULL;
bitboard fileBBB = 0x0202020202020202ULL;
bitboard fileCBB = 0x0404040404040404ULL;
bitboard fileDBB = 0x0808080808080808ULL;
bitboard fileEBB = 0x1010101010101010ULL;
bitboard fileFBB = 0x2020202020202020ULL;
bitboard fileGBB = 0x4040404040404040ULL;
bitboard fileHBB = 0x8080808080808080ULL;

bitboard rank1BB = 0xFF;
bitboard rank2BB = 0xFF00;
bitboard rank3BB = 0xFF0000;
bitboard rank4BB = 0xFF000000;
bitboard rank5BB = 0xFF00000000;
bitboard rank6BB = 0xFF0000000000;
bitboard rank7BB = 0xFF000000000000;
bitboard rank8BB = 0xFF00000000000000;

bitboard fileBBs[] = { 0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
                       0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL };

bitboard rankBBs[] = {         0xFF,         0xFF00,         0xFF0000,         0xFF000000, 
                       0xFF00000000, 0xFF0000000000, 0xFF000000000000, 0xFF00000000000000 };


int Lsb(bitboard *bb);
int PopLsb(bitboard *bb);
int PopCount(bitboard x);
void PrintBB(bitboard bb);
