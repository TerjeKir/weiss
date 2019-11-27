// bitboards.c

#include <stdio.h>
#include <string.h>

#include "bitboards.h"


const Bitboard fileBBs[] = { 0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
                             0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL };

const Bitboard rankBBs[] = {         0xFF,         0xFF00,         0xFF0000,         0xFF000000,
                             0xFF00000000, 0xFF0000000000, 0xFF000000000000, 0xFF00000000000000 };


CONSTR InitBitMasks() {

    for (int i = A1; i <= H8; ++i) {
        SetMask[i]  |= (1ULL << i);
        ClearMask[i] = ~SetMask[i];
    }
}

// Unused, here for occasional print debugging
// Prints a bitboard
// void PrintBB(const Bitboard bb) {

//     Bitboard bitmask = 1;

//     for (int rank = 7; rank >= 0; --rank) {
//         for (int file = 0; file <= 7; ++file) {
//             if (bb & (bitmask << ((rank * 8) + file)))
//                 printf("1");
//             else
//                 printf("0");
//         }
//         printf("\n");
//     }
//     fflush(stdout);
// }