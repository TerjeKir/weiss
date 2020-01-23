// bitboards.c

#include <stdio.h>
#include <string.h>

#include "bitboards.h"


const Bitboard fileBB[] = { fileABB, fileBBB, fileCBB, fileDBB,
                            fileEBB, fileFBB, fileGBB, fileHBB };

const Bitboard rankBB[] = { rank1BB, rank2BB, rank3BB, rank4BB,
                            rank5BB, rank6BB, rank7BB, rank8BB };

Bitboard SquareBB[64];


CONSTR InitBitMasks() {

    for (int sq = A1; sq <= H8; ++sq)
        SquareBB[sq] = (1ULL << sq);
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