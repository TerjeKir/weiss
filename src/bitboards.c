// bitboards.c

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "bitboards.h"


const bitboard fileBBs[] = { 0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
                             0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL };

const bitboard rankBBs[] = {         0xFF,         0xFF00,         0xFF0000,         0xFF000000, 
                             0xFF00000000, 0xFF0000000000, 0xFF000000000000, 0xFF00000000000000 };


int distance[64][64];


// Returns distance between sq1 and sq2
inline int Distance(int sq1, int sq2) {
    return distance[sq1][sq2];
}

void InitDistance() {

    int vertical, horizontal;

    for (int sq1 = 0; sq1 < 64; sq1++)
        for (int sq2 = 0; sq2 < 64; sq2++) {
            vertical = abs((sq1 / 8) - (sq2 / 8));
            horizontal = abs((sq1 % 8) - (sq2 % 8));
            distance[sq1][sq2] = ((vertical > horizontal) ? vertical : horizontal);
        }
}

// Population count/Hamming weight
inline int PopCount(bitboard bb) {

    return __builtin_popcountll(bb);
}

// Returns the index of the least significant bit
inline int Lsb(bitboard bb) {

    return __builtin_ctzll(bb);
}

// Returns the index of the least significant bit and unsets it
inline int PopLsb(bitboard *bb) {

	int lsb = Lsb(*bb);
	*bb &= (*bb - 1);

	return lsb;
}

// Prints a bitboard
void PrintBB(bitboard bb) {
    
    bitboard bitmask = 1;

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file <= 7; file++) {
            if (bb & (bitmask << ((rank * 8) + file)))
                printf("1");
            else
                printf("0");
        }
        printf("\n");
    }
}
