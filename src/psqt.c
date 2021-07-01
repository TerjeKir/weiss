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
      S( 50, 80), S( 38, 70), S( 41, 52), S( 45, -3), S( 40, -8), S( 37, 14), S(-45, 73), S(-50, 82),
      S( 23, 87), S( 17, 87), S( 46, 30), S( 50,-22), S( 66,-27), S(121, -1), S( 88, 57), S( 42, 64),
      S(-11, 45), S(-10, 25), S( -4,  3), S(  4,-25), S( 25,-22), S( 31,-15), S(  8, 12), S( 11, 11),
      S(-20, 14), S(-28, 11), S(-12, -9), S( -6,-17), S(  1,-16), S(  5,-16), S(-12, -2), S( -6,-10),
      S(-27,  5), S(-31,  1), S(-22, -7), S(-19,-11), S(-10, -7), S( -9, -6), S(  2,-15), S( -9,-15),
      S(-14, 12), S( -6, 12), S( -5,  0), S( -6,  9), S(  4, 13), S( 19, -1), S( 31, -6), S( -3,-14),
      S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0) },

    { S(-140,-92), S(-50,-31), S(-77,  4), S(-34,-14), S(-14, -7), S(-67, -2), S(-50,-24), S(-110,-88),
      S(-18,-19), S( -5,  4), S( 32, -6), S( 45, -1), S( 32, -9), S( 59,-26), S(-20,  0), S(-20,-31),
      S(-13, -8), S( 31,  1), S( 22, 35), S( 41, 30), S( 76,  8), S( 46, 12), S( 27,-16), S(-20,-23),
      S(  7,  3), S( 16, 15), S( 36, 37), S( 42, 43), S( 26, 37), S( 58, 29), S( 15,  6), S( 29, -9),
      S(  5, 10), S( 13, 10), S( 17, 40), S( 21, 36), S( 22, 40), S( 36, 25), S( 36,  6), S( 27,  6),
      S(-22,-23), S(-14,  6), S( -3, 17), S(  0, 33), S( 13, 28), S(  4, 10), S( 12, -1), S(  4,-16),
      S(-37,-14), S(-31, -1), S(-20, -1), S(-10, 11), S( -9,  2), S(-18, -4), S(-24, -9), S(-11,  3),
      S(-58,-40), S(-26,-22), S(-27,-11), S( -5,  7), S(  0, 11), S( -3,-14), S(-21, -8), S(-37,-37) },

    { S(-34, 30), S(-38, 17), S(-64, 19), S(-72, 22), S(-66, 18), S(-88, 10), S(-24,  9), S(-40,  8),
      S(-21, 13), S( 16,  8), S(  0,  9), S(-20, 14), S( -2,  5), S(-19, 10), S(-21, 15), S(-64, 17),
      S( -1, 16), S( 24,  9), S( 38,  2), S( 36, -2), S( 29, -3), S( 53,  9), S( 11,  8), S( 16,  0),
      S( -8,  8), S( 34,  6), S( 28,  6), S( 44, 29), S( 43,  9), S( 27, 10), S( 42, -6), S( -9,  5),
      S( 12, -3), S(  5,  5), S( 14, 16), S( 38, 19), S( 25, 17), S( 27,  5), S( 10,  4), S( 43,-10),
      S(  7, -7), S( 34, 15), S( 20, 12), S(  8, 20), S( 17, 25), S( 26, 14), S( 38,  4), S( 33,-13),
      S( 20, -8), S( 15,-21), S( 20,-13), S( -5,  8), S(  4,  6), S(  8, -4), S( 25,-18), S( 23,-33),
      S( 16, -8), S( 30,  9), S( 16, 16), S(  3,  7), S( 19,  7), S(  5, 16), S( 21,  3), S( 31,-24) },

    { S( 22, 24), S( 15, 32), S( -5, 50), S( -5, 40), S(  1, 35), S(  3, 37), S( 20, 29), S( 25, 27),
      S(-12, 32), S(-20, 50), S(  0, 54), S(  7, 44), S( -6, 39), S( 12, 23), S(-12, 29), S(  4, 21),
      S( -9, 30), S( 33, 23), S( 16, 28), S( 37,  6), S( 55, -2), S( 40,  1), S( 68, -3), S( 17,  5),
      S( -9, 36), S( 11, 28), S( 12, 33), S( 27, 12), S( 19,  3), S( 18,  2), S( 23,  5), S( 12,  4),
      S(-20, 28), S(-21, 31), S(-15, 28), S( -8, 22), S(-12, 18), S(-30, 17), S( -2,  7), S(-13,  4),
      S(-21, 15), S(-18, 15), S(-16, 14), S(-14, 11), S( -9,  5), S( -9, -7), S( 14,-23), S( -6,-13),
      S(-32, 24), S(-12, 12), S( -1, 14), S( -1,  9), S(  5, -1), S(-13, -2), S(  7,-10), S(-37, 13),
      S( -9, 23), S(-10, 17), S( -9, 20), S(  1,  5), S(  5, -2), S(  2,  4), S(  9, -3), S( -7,  2) },

    { S(-23,  1), S(-10, 19), S(  4, 38), S( 13, 49), S( 12, 55), S( 31, 53), S( 10, 44), S(  6, 32),
      S(-12,  5), S(-49, 58), S(-33, 60), S(-60, 98), S(-63,123), S(-24, 76), S(-41, 78), S( -6, 79),
      S( -2,  5), S( -4,  5), S( -3, 45), S(-13, 66), S( -3, 79), S( 10, 87), S( 20, 52), S( 14, 63),
      S( -1, -8), S( 11, 18), S( -4, 32), S(-16, 72), S(-13, 89), S( -7, 86), S( 19, 78), S(  8, 47),
      S( 16,-31), S(  2,  4), S(  2, 12), S( -4, 49), S( -3, 47), S(  3, 30), S( 23, 16), S( 24, 15),
      S(  7,-53), S( 16,-32), S(  4, -3), S(  3, -8), S( 10, -7), S( 14,-12), S( 36,-38), S( 24,-39),
      S( 10,-60), S( 15,-59), S( 20,-64), S( 20,-27), S( 22,-35), S( 17,-102), S( 27,-106), S( 10,-59),
      S(  0,-72), S( -6,-76), S(  4,-74), S(  9,-68), S(  6,-68), S( -3,-87), S(  5,-81), S(  5,-78) },

    { S(-82,-101), S(-50,-51), S(-69,-20), S(-63,  6), S(-73,-10), S(-61,-16), S(-57,-12), S(-66,-106),
      S(-64,-43), S(-17, 30), S(-34, 46), S(-33, 41), S(-18, 34), S(-22, 49), S(-17, 50), S(-47,-38),
      S(-27,-22), S( 13, 38), S( 20, 61), S( 28, 74), S( 35, 73), S( 47, 72), S( 23, 54), S(-34,-10),
      S(-31,-26), S( 18, 19), S( 23, 59), S( 33, 87), S( 21, 86), S( 45, 61), S( 11, 29), S(-58,-14),
      S(-22,-53), S( 33, -3), S( 50, 39), S( 20, 70), S( 31, 64), S( 37, 36), S( 46, -6), S(-72,-32),
      S(-49,-45), S( 23, -9), S( 13, 19), S( -1, 39), S( 12, 35), S( -4, 21), S( 14, -8), S(-65,-30),
      S(  3,-54), S(  2,-20), S( -5,  0), S(-48, 19), S(-44, 20), S(-20,  2), S( 14,-29), S( -6,-62),
      S(-35,-112), S( 19,-85), S( -7,-49), S(-85,-38), S(-36,-50), S(-73,-33), S(  1,-80), S(-30,-117) },
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
