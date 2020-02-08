/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2020  Terje Kirstihagen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>

#include "bitboards.h"


const Bitboard FileBB[8] = { fileABB, fileBBB, fileCBB, fileDBB,
                             fileEBB, fileFBB, fileGBB, fileHBB };

const Bitboard RankBB[8] = { rank1BB, rank2BB, rank3BB, rank4BB,
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