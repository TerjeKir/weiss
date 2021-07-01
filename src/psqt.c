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
const int PieceSqValue[6][64] = {

    { S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0),
      S( 52, 76), S( 41, 71), S( 44, 49), S( 49,  1), S( 44, -4), S( 40, 16), S(-45, 71), S(-49, 79),
      S( 16, 82), S( 23, 81), S( 51, 33), S( 56,-16), S( 72,-21), S(121,  4), S( 82, 50), S( 35, 58),
      S( -5, 40), S( -8, 26), S( -1,  7), S( 11,-20), S( 25,-19), S( 34,-11), S(  2, 14), S(  4, 16),
      S(-14, 17), S(-22, 15), S( -8, -9), S( -8,-13), S(  1,-13), S(  4,-12), S( -8,  1), S( -5, -4),
      S(-22,  6), S(-28,  1), S(-24, -2), S(-19, -8), S( -9, -4), S(-11,  1), S( -2,-11), S( -7,-12),
      S( -9, 10), S(-10,  7), S(-11,  7), S( -6,  5), S( -4,  8), S( 15,  7), S( 24, -6), S(  3,-21),
      S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0) },

    { S(-143,-93), S(-51,-30), S(-79,  7), S(-33,-11), S(-13, -5), S(-68,  1), S(-51,-23), S(-112,-89),
      S(-15,-22), S( -9,  0), S( 34, -8), S( 50,  5), S( 38, -3), S( 58,-26), S(-24, -2), S(-18,-30),
      S(-17,-12), S( 26, -1), S( 26, 39), S( 46, 35), S( 80, 13), S( 52, 19), S( 31,-11), S(-18,-20),
      S(  7, -2), S( 21, 16), S( 40, 41), S( 35, 50), S( 27, 44), S( 52, 32), S( 22, 12), S( 23,-11),
      S(  2,  4), S( 18, 12), S( 20, 42), S( 24, 42), S( 23, 44), S( 31, 32), S( 41,  9), S( 30,  2),
      S(-25,-28), S( -8,  0), S( -6, 14), S(  5, 30), S(  9, 26), S(  2,  7), S(  5, -5), S( -3,-22),
      S(-33,-14), S(-37, -4), S(-24, -7), S( -9,  8), S(-13,  3), S(-22, -9), S(-29,-12), S(-10, -1),
      S(-57,-42), S(-20,-28), S(-21,-16), S(-11,  2), S( -4,  6), S( -9,-20), S(-18,-13), S(-42,-40) },

    { S(-36, 31), S(-38, 20), S(-66, 22), S(-73, 27), S(-67, 23), S(-90, 13), S(-24, 11), S(-39, 11),
      S(-25, 13), S( 15, 14), S(  4, 15), S(-18, 19), S(  0, 10), S(-15, 16), S(-19, 20), S(-60, 22),
      S(  3, 16), S( 28, 15), S( 45,  9), S( 38,  4), S( 35,  4), S( 54, 14), S( 16, 14), S( 11,  3),
      S( -3, 10), S( 39, 11), S( 31, 10), S( 48, 29), S( 47, 16), S( 30, 15), S( 46,  0), S( -4, 10),
      S( 11, -7), S( 12,  7), S( 20, 21), S( 35, 22), S( 31, 22), S( 24, 11), S( 16,  8), S( 36,-14),
      S(  9,-11), S( 27,  9), S( 17, 12), S( 14, 22), S( 17, 20), S( 22, 10), S( 32, -1), S( 30,-11),
      S( 21,-11), S( 17,-26), S( 13,-12), S( -1,  6), S(  0,  5), S(  4, -9), S( 20,-23), S( 21,-37),
      S( 23, -7), S( 23,  3), S( 12,  9), S(  2,  2), S( 12,  1), S(  7,  8), S( 15,  0), S( 27,-25) },

    { S( 28, 31), S( 21, 39), S( -1, 55), S(  1, 47), S(  6, 42), S(  8, 43), S( 24, 35), S( 30, 34),
      S( -5, 39), S(-15, 54), S(  5, 55), S( 13, 51), S(  1, 46), S( 17, 28), S( -8, 35), S(  8, 27),
      S( -3, 36), S( 39, 23), S( 22, 34), S( 44, 13), S( 61,  4), S( 47,  8), S( 71,  0), S( 23, 12),
      S( -3, 32), S( 16, 30), S( 19, 35), S( 34, 19), S( 27, 10), S( 26,  9), S( 29, 10), S( 18, 10),
      S(-18, 23), S(-16, 34), S(-11, 30), S( -5, 23), S( -8, 19), S(-23, 23), S(  3, 12), S( -9,  7),
      S(-26,  9), S(-13, 13), S(-21,  9), S(-17,  5), S(-14, -1), S(-13,-10), S( 12,-19), S(-12,-17),
      S(-39, 17), S(-18,  6), S( -8,  7), S( -7,  2), S( -1, -7), S(-14, -8), S(  0,-16), S(-43,  7),
      S(-16, 16), S(-10, 10), S( -9, 12), S( -3, -1), S( -2, -8), S(  0,  4), S(  3, -6), S(-10, -4) },

    { S(-17,  6), S( -5, 24), S(  7, 42), S( 17, 54), S( 17, 60), S( 34, 57), S( 12, 48), S( 10, 36),
      S(-10,  8), S(-56, 54), S(-31, 60), S(-60,101), S(-59,128), S(-19, 81), S(-46, 79), S(-13, 75),
      S( -5,  4), S( -2,  6), S( -8, 41), S(-13, 67), S(  2, 83), S( 15, 92), S( 26, 57), S( 13, 67),
      S(  3, -7), S( 12, 20), S( -6, 29), S(-13, 74), S( -9, 93), S( -2, 91), S( 26, 84), S( 15, 53),
      S( 14,-32), S(  8,  8), S(  5, 12), S( -5, 51), S( -3, 48), S(  9, 35), S( 25, 18), S( 29, 19),
      S(  6,-54), S( 20,-31), S(  8, -3), S(  3,-12), S(  6,-11), S(  8,-15), S( 30,-42), S( 18,-43),
      S(  7,-62), S( 13,-60), S( 19,-70), S( 14,-32), S( 16,-41), S( 14,-106), S( 20,-111), S(  4,-62),
      S(  3,-72), S( -2,-77), S(  2,-78), S(  7,-70), S(  6,-72), S(  0,-90), S(  5,-83), S(  2,-81) },

    { S(-82,-103), S(-49,-51), S(-68,-20), S(-63,  6), S(-73,-10), S(-61,-16), S(-57,-12), S(-66,-107),
      S(-64,-44), S(-16, 33), S(-33, 47), S(-32, 40), S(-18, 32), S(-22, 46), S(-17, 49), S(-48,-40),
      S(-26,-20), S( 16, 43), S( 22, 64), S( 27, 70), S( 34, 68), S( 47, 68), S( 24, 53), S(-35,-13),
      S(-30,-22), S( 21, 23), S( 26, 64), S( 33, 85), S( 22, 82), S( 46, 60), S( 13, 26), S(-60,-18),
      S(-23,-53), S( 37,  2), S( 54, 43), S( 23, 74), S( 35, 69), S( 41, 40), S( 51, -1), S(-75,-38),
      S(-47,-41), S( 25,-10), S( 18, 22), S(  4, 44), S( 18, 39), S(  2, 26), S( 19,-10), S(-60,-33),
      S(  7,-51), S(  6,-21), S( -3, -1), S(-51, 19), S(-40, 15), S(-25,  4), S(  7,-28), S( -2,-64),
      S(-35,-115), S( 13,-82), S(-15,-52), S(-77,-42), S(-39,-58), S(-77,-34), S(  1,-73), S(-28,-125) },
};

// Initialize the piece square tables with piece values included
CONSTR InitPSQT() {

    // Black scores are negative (white double negated -> positive)
    for (PieceType pt = PAWN; pt <= KING; ++pt)
        for (Square sq = A1; sq <= H8; ++sq) {
            // Base piece value + the piece square value
            PSQT[MakePiece(BLACK, pt)][sq] = -(PieceTypeValue[pt] + PieceSqValue[pt-1][sq]);
            // Same score inverted used for white on the square mirrored horizontally
            PSQT[MakePiece(WHITE, pt)][MirrorSquare(sq)] = -PSQT[pt][sq];
        }
}
