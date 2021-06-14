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

#include "board.h"
#include "evaluate.h"
#include "psqt.h"


extern const int PieceTypeValue[7];


int PSQT[PIECE_NB][64];

// Black's point of view - easier to read as it's not upside down
const int PieceSqValue[7][64] = {
    { 0 },

    { S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0),
      S( 61, 59), S( 45, 59), S( 51, 35), S( 48,  3), S( 49,  2), S( 30, 20), S(-15, 47), S(-19, 53),
      S(  9, 76), S( 36, 62), S( 49, 33), S( 45, -3), S( 65, -6), S(113, 15), S( 67, 41), S( 22, 51),
      S(  0, 35), S( -4, 24), S( -1, 10), S( 19,-20), S( 27,-14), S( 31, -3), S(  0, 16), S( -1, 15),
      S(-10, 12), S(-14, 10), S( -2,-10), S( -1,-15), S(  7,-14), S(  6, -8), S( -6,  0), S( -9, -3),
      S(-14,  2), S(-20, -3), S(-17, -2), S(-12, -8), S( -7, -2), S( -7,  3), S(  2,-11), S( -4,-11),
      S( -3,  6), S(-10,  1), S(-12,  8), S( -4,  6), S( -9, 14), S( 14, 10), S( 19, -9), S(  6,-20),
      S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0) },

    { S(-86,-82), S(-23,-21), S(-37,  0), S(-12, -8), S( -3, -4), S(-28,  3), S(-26,-18), S(-74,-60),
      S(-13,-26), S(-17, -7), S( 38,-20), S( 49, 10), S( 42,  7), S( 39,-22), S(-23,-11), S(-19,-21),
      S(-20,-18), S( 21, -2), S( 26, 32), S( 44, 26), S( 76, 13), S( 58, 32), S( 38, -6), S( -9,-14),
      S(  4, -7), S( 15, 14), S( 30, 38), S( 30, 48), S( 20, 53), S( 36, 34), S( 15, 24), S( 12,  0),
      S(  0,  0), S( 17,  8), S( 12, 40), S( 18, 43), S( 20, 39), S( 19, 39), S( 35,  9), S( 22, 10),
      S(-27,-36), S(-12,-12), S(-13,  6), S(  2, 24), S(  3, 22), S( -7,  1), S( -6, -9), S(-14,-25),
      S(-28,-12), S(-44,  4), S(-26,-21), S(-10, -2), S(-14, -1), S(-24,-22), S(-33,  1), S( -8,  1),
      S(-48,-47), S(-13,-45), S(-17,-29), S(-13, -2), S( -6, -3), S(-11,-16), S(-14,-23), S(-49,-43) },

    { S(-24, 15), S(-12,  9), S(-29,  7), S(-29, 14), S(-25, 12), S(-43,  1), S(-11,  6), S(-15,  6),
      S(-48, 14), S(  3, 15), S( -1, 13), S(-17, 13), S( -9, 11), S( -6, 15), S(-20, 17), S(-49, 11),
      S(  4, 10), S( 27, 18), S( 52, 10), S( 31,  6), S( 45,  7), S( 46, 23), S( 25, 22), S(  8, 10),
      S(-11,  5), S( 31, 13), S( 20, 10), S( 52, 16), S( 40, 23), S( 19, 15), S( 28, 19), S(-12, 12),
      S(  0, -8), S( 10, -1), S( 14, 18), S( 29, 18), S( 30, 17), S( 10, 15), S( 11,  2), S( 11, -8),
      S(  5, -9), S( 16,  5), S( 13, 11), S( 13, 14), S( 13, 13), S( 16,  7), S( 22, -4), S( 22, -2),
      S( 19,-12), S( 17,-23), S(  6,-10), S( -4, -1), S( -2,  2), S(  1,-16), S( 21,-18), S( 21,-31),
      S( 26, -9), S( 12,  6), S(  9, -5), S( -1,-10), S( -3, -4), S(  9,  1), S(  6,  8), S( 24,-12) },

    { S( 38, 38), S( 29, 45), S(  9, 52), S( 11, 47), S( 20, 45), S( 15, 51), S( 29, 46), S( 32, 47),
      S(  5, 39), S( -6, 45), S( 20, 41), S( 28, 45), S( 20, 46), S( 33, 25), S(  8, 32), S( 16, 31),
      S(  6, 31), S( 57, 12), S( 37, 26), S( 67, 10), S( 79,  2), S( 62, 17), S( 73,  2), S( 29, 20),
      S(  0, 23), S( 18, 21), S( 27, 22), S( 54, 10), S( 44, 11), S( 37, 10), S( 37,  8), S( 23, 15),
      S(-22, 10), S(-16, 23), S(-18, 23), S(-12, 14), S(-14, 10), S(-18, 15), S(  3, 10), S(-12,  5),
      S(-32,-13), S(-16, -1), S(-28, -4), S(-20,-12), S(-23,-13), S(-24,-12), S(  0,-13), S(-24,-18),
      S(-63, -7), S(-25,-17), S(-19,-12), S(-12,-18), S(-10,-22), S(-18,-29), S(-15,-29), S(-62,-14),
      S(-24,-10), S(-18, -8), S(-17, -5), S( -8,-17), S(-12,-19), S( -9,-11), S( -6,-15), S(-16,-22) },

    { S(  1, 11), S( 15, 26), S( 27, 31), S( 31, 50), S( 39, 57), S( 45, 51), S( 26, 34), S( 32, 36),
      S(-18, 10), S(-70, 48), S(-18, 34), S(-28, 50), S(-23, 78), S( 18, 56), S(-37, 30), S(-17, 35),
      S(-13,  3), S( -2, -3), S( -7, 24), S( -1, 38), S( 24, 55), S( 40, 71), S( 47, 55), S( 17, 53),
      S( -3,-15), S( -1, 23), S(-10, 15), S(-11, 56), S( -4, 66), S( -2, 72), S( 29, 70), S( 14, 53),
      S(  2,-24), S(  5,  2), S( -3,  6), S(-13, 46), S( -9, 42), S( -2, 38), S( 12, 23), S( 17, 29),
      S( -4,-40), S( 11,-33), S(  3,-12), S( -2,-22), S(  1,-23), S( -2,-10), S( 18,-36), S(  4,-23),
      S( -5,-42), S(  5,-53), S( 14,-81), S(  8,-38), S( 11,-50), S(  7,-97), S(  2,-78), S(-15,-41),
      S( -8,-54), S( -9,-68), S( -4,-78), S(  2,-75), S(  0,-76), S(-10,-75), S( -9,-51), S(-13,-46) },

    { S(-76,-68), S(-52,-37), S(-71, -4), S(-64, 12), S(-76, -5), S(-68,-25), S(-65,-19), S(-67,-81),
      S(-62,-41), S(-24, 32), S(-45, 37), S(-54, 21), S(-33, 16), S(-45, 28), S(-31, 38), S(-56,-43),
      S(-23,-11), S( -6, 46), S(  1, 58), S(  9, 50), S(  5, 45), S( 14, 53), S( -3, 42), S(-42,-19),
      S(-19,-10), S(  5, 27), S(  5, 63), S( 28, 73), S(  8, 70), S( 30, 51), S( -8, 23), S(-38,-26),
      S( -4,-45), S( 20, 15), S( 37, 47), S( 29, 69), S( 35, 65), S( 38, 40), S( 49,  6), S(-38,-46),
      S(-18,-34), S( 29, -2), S( 33, 23), S( 16, 44), S( 40, 36), S( 29, 24), S( 40, -7), S(-30,-34),
      S( 14,-37), S(  8,-11), S( -5,  9), S(-55, 23), S(-35, 17), S(-28, 13), S(  9,-16), S( 10,-53),
      S(-25,-106), S( 19,-64), S(-12,-33), S(-39,-54), S(-25,-68), S(-66,-27), S( 14,-63), S(-11,-128) },
};

// Initialize the piece square tables with piece values included
CONSTR InitPSQT() {

    // Black scores are negative (white double negated -> positive)
    for (PieceType pt = PAWN; pt <= KING; ++pt)
        for (Square sq = A1; sq <= H8; ++sq) {
            // Base piece value + the piece square value
            PSQT[MakePiece(BLACK, pt)][sq] = -(PieceTypeValue[pt] + PieceSqValue[pt][sq]);
            // Same score inverted used for white on the square mirrored horizontally
            PSQT[MakePiece(WHITE, pt)][MirrorSquare(sq)] = -PSQT[pt][sq];
        }
}
