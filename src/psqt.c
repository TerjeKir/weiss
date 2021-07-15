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


extern const int PieceTypeValue[TYPE_NB];


int PSQT[2][PIECE_NB][64];

// Black's point of view - easier to read as it's not upside down
const int PieceSqValue[2][6][64] = {{

    { S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0),
      S( 57,  3), S( 51, 32), S( 32, 64), S( 73, 35), S( 82, 40), S( 88, 27), S(-40, 85), S(-52, 53),
      S(  2, 74), S( 16, 77), S( 33, 53), S( 39, 28), S( 62, 31), S(133, 32), S( 91, 60), S( 33, 66),
      S(-11, 40), S(-11, 19), S(-10,  6), S(  4,-25), S( 23,-15), S( 39,-17), S( 10,  8), S(  2, 16),
      S(-18, 19), S(-27, 13), S(-13,-12), S(-15,-16), S( -3,-16), S(  4,-14), S( -4, -8), S( -4, -6),
      S(-25,  4), S(-35, -3), S(-30, -3), S(-25, -9), S(-15,  1), S( -7,  4), S(  2,-16), S( -5,-15),
      S(-13, 10), S(-14,  8), S(-21, 12), S(-11, 10), S(-17, 31), S( 19, 17), S( 32, -4), S(  3,-24),
      S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0) },

    { S(-207,-71), S(-112,-12), S(-145, 34), S(-53,  8), S( -1, 16), S(-128, 43), S(-79,-11), S(-158,-115),
      S(  0,-26), S(  0, -1), S( 51,-12), S( 57, 27), S( 65, 10), S( 79,-32), S(-12,  2), S( 16,-41),
      S(-21,-13), S( 26,  6), S( 36, 53), S( 55, 53), S( 98, 31), S( 80, 36), S( 45, -6), S(  3,-19),
      S(  9, -3), S( 25, 18), S( 48, 56), S( 43, 73), S( 29, 71), S( 69, 48), S( 30, 25), S( 39, -3),
      S(  0,  2), S( 18, 12), S( 23, 56), S( 27, 58), S( 25, 59), S( 36, 54), S( 54, 13), S( 31, 13),
      S(-22,-44), S( -5, -5), S( -1, 15), S( 15, 39), S( 16, 37), S(  8, 14), S( 11,  1), S( -1,-17),
      S(-34,-37), S(-40, -3), S(-19,-16), S( -5, 10), S(-10,  6), S(-10,-16), S(-22,-15), S( -6, -6),
      S(-88,-61), S(-15,-52), S(-40,-19), S( -7,  7), S(  0,  6), S( -5,-26), S(-13,-22), S(-54,-37) },

    { S(-40, 51), S(-55, 44), S(-133, 58), S(-125, 63), S(-128, 57), S(-150, 49), S(-25, 25), S(-39, 24),
      S(-21, 23), S( 23, 29), S(  8, 28), S(-23, 36), S(  7, 23), S( -7, 33), S(-18, 37), S(-49, 31),
      S(  4, 25), S( 31, 24), S( 57, 24), S( 39, 18), S( 49, 22), S( 57, 33), S( 27, 33), S( 12, 18),
      S( -5, 17), S( 40, 19), S( 30, 20), S( 58, 39), S( 44, 36), S( 38, 27), S( 45, 19), S( -1, 23),
      S( 15, -7), S( 21,  2), S( 23, 29), S( 39, 30), S( 34, 31), S( 25, 21), S( 25, 10), S( 40, -8),
      S( 12, -8), S( 32, 11), S( 22, 14), S( 16, 21), S( 21, 25), S( 27, 13), S( 35,  1), S( 37, -8),
      S( 25,-13), S( 22,-31), S( 26,-12), S(  2,  5), S(  3,  6), S( 10,-13), S( 29,-30), S( 29,-53),
      S( 34,-17), S( 40, -2), S( 15,  7), S( -4,  5), S(  9,  6), S( 12, -2), S( 22, -8), S( 33,-24) },

    { S( 36, 67), S( 36, 78), S( -3, 98), S(  4, 88), S( 14, 88), S( 15, 88), S( 34, 83), S( 50, 75),
      S( -1, 74), S(-17, 84), S( 11, 82), S( 18, 85), S(  8, 83), S( 19, 52), S(  4, 62), S( 27, 51),
      S( -3, 68), S( 50, 45), S( 26, 64), S( 53, 46), S( 70, 33), S( 65, 44), S( 89, 18), S( 30, 46),
      S( -8, 64), S( 14, 58), S( 25, 60), S( 47, 48), S( 41, 44), S( 46, 36), S( 44, 32), S( 25, 43),
      S(-23, 43), S(-21, 60), S(-20, 58), S(-12, 48), S(-16, 44), S(-20, 45), S(  5, 36), S(-12, 30),
      S(-30, 20), S(-22, 34), S(-30, 29), S(-23, 21), S(-22, 21), S(-22, 11), S(  7,  6), S(-18,  5),
      S(-49, 21), S(-26, 19), S(-17, 22), S(-13, 14), S( -9,  7), S(-18,  0), S( -7, -6), S(-44, 12),
      S(-23, 26), S(-22, 25), S(-20, 28), S(-10, 12), S(-11, 10), S( -9, 19), S( -3, 14), S(-19, 10) },

    { S(-32, 87), S(-10,101), S( -5,128), S( 10,135), S(  7,157), S( 45,146), S( 43,134), S( 31,125),
      S(-18, 57), S(-60, 99), S(-29,105), S(-84,199), S(-80,239), S(-15,179), S(-43,165), S( 16,150),
      S(-15, 44), S(  0, 33), S(-10, 79), S(-15,122), S(  7,164), S( 42,167), S( 63,126), S( 14,149),
      S(  3, 17), S(  2, 60), S(-11, 70), S(-15,130), S(-12,167), S(  0,182), S( 40,166), S( 18,133),
      S( 12, -3), S( 10, 45), S(  1, 50), S( -6, 95), S( -8,103), S( 11, 87), S( 27, 72), S( 39, 76),
      S(  6,-26), S( 19,  3), S(  9, 27), S(  4, 21), S(  8, 24), S(  8, 27), S( 35, -6), S( 28,-13),
      S( 10,-37), S( 15,-28), S( 20,-40), S( 15, -3), S( 16,-13), S( 17,-95), S( 29,-125), S( 23,-78),
      S(  2,-44), S( -6,-47), S( -2,-42), S(  7,-44), S(  6,-47), S(-16,-53), S( -1,-83), S( 12,-57) },

    { S(-99,-142), S(-50,-57), S(-64,-30), S(-66,  8), S(-75,-16), S(-50, -4), S(-32, 13), S(-60,-133),
      S(-81,-46), S(-21, 35), S(-20, 37), S(  3, 24), S(  6, 25), S( 14, 40), S( 20, 57), S(-33,-30),
      S(-40,-11), S( 58, 35), S( 45, 57), S( 17, 62), S( 53, 56), S( 87, 63), S( 72, 49), S(-30,-11),
      S(-37, -8), S( 34, 23), S( 28, 63), S(-10, 83), S(  3, 79), S( 53, 55), S( 43, 28), S(-73, -3),
      S(-46,-45), S( 47, -1), S( 54, 38), S(-30, 76), S( 15, 65), S( 50, 40), S( 48,  6), S(-111,-14),
      S(-38,-45), S( 44,-19), S( 18, 12), S( -4, 34), S( 17, 30), S(  4, 24), S( 16, -7), S(-68,-23),
      S(  5,-42), S(  5,-22), S(-14, -3), S(-70, 13), S(-53, 12), S(-41, 11), S( -3,-15), S(-12,-48),
      S(-35,-116), S( 15,-63), S(-20,-34), S(-110,-41), S(-46,-70), S(-105,-15), S( -5,-63), S(-28,-126) },
}, {
    { S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0),
      S( 57,  3), S( 51, 32), S( 32, 64), S( 73, 35), S( 82, 40), S( 88, 27), S(-40, 85), S(-52, 53),
      S(  2, 74), S( 16, 77), S( 33, 53), S( 39, 28), S( 62, 31), S(133, 32), S( 91, 60), S( 33, 66),
      S(-11, 40), S(-11, 19), S(-10,  6), S(  4,-25), S( 23,-15), S( 39,-17), S( 10,  8), S(  2, 16),
      S(-18, 19), S(-27, 13), S(-13,-12), S(-15,-16), S( -3,-16), S(  4,-14), S( -4, -8), S( -4, -6),
      S(-25,  4), S(-35, -3), S(-30, -3), S(-25, -9), S(-15,  1), S( -7,  4), S(  2,-16), S( -5,-15),
      S(-13, 10), S(-14,  8), S(-21, 12), S(-11, 10), S(-17, 31), S( 19, 17), S( 32, -4), S(  3,-24),
      S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0) },

    { S(-207,-71), S(-112,-12), S(-145, 34), S(-53,  8), S( -1, 16), S(-128, 43), S(-79,-11), S(-158,-115),
      S(  0,-26), S(  0, -1), S( 51,-12), S( 57, 27), S( 65, 10), S( 79,-32), S(-12,  2), S( 16,-41),
      S(-21,-13), S( 26,  6), S( 36, 53), S( 55, 53), S( 98, 31), S( 80, 36), S( 45, -6), S(  3,-19),
      S(  9, -3), S( 25, 18), S( 48, 56), S( 43, 73), S( 29, 71), S( 69, 48), S( 30, 25), S( 39, -3),
      S(  0,  2), S( 18, 12), S( 23, 56), S( 27, 58), S( 25, 59), S( 36, 54), S( 54, 13), S( 31, 13),
      S(-22,-44), S( -5, -5), S( -1, 15), S( 15, 39), S( 16, 37), S(  8, 14), S( 11,  1), S( -1,-17),
      S(-34,-37), S(-40, -3), S(-19,-16), S( -5, 10), S(-10,  6), S(-10,-16), S(-22,-15), S( -6, -6),
      S(-88,-61), S(-15,-52), S(-40,-19), S( -7,  7), S(  0,  6), S( -5,-26), S(-13,-22), S(-54,-37) },

    { S(-40, 51), S(-55, 44), S(-133, 58), S(-125, 63), S(-128, 57), S(-150, 49), S(-25, 25), S(-39, 24),
      S(-21, 23), S( 23, 29), S(  8, 28), S(-23, 36), S(  7, 23), S( -7, 33), S(-18, 37), S(-49, 31),
      S(  4, 25), S( 31, 24), S( 57, 24), S( 39, 18), S( 49, 22), S( 57, 33), S( 27, 33), S( 12, 18),
      S( -5, 17), S( 40, 19), S( 30, 20), S( 58, 39), S( 44, 36), S( 38, 27), S( 45, 19), S( -1, 23),
      S( 15, -7), S( 21,  2), S( 23, 29), S( 39, 30), S( 34, 31), S( 25, 21), S( 25, 10), S( 40, -8),
      S( 12, -8), S( 32, 11), S( 22, 14), S( 16, 21), S( 21, 25), S( 27, 13), S( 35,  1), S( 37, -8),
      S( 25,-13), S( 22,-31), S( 26,-12), S(  2,  5), S(  3,  6), S( 10,-13), S( 29,-30), S( 29,-53),
      S( 34,-17), S( 40, -2), S( 15,  7), S( -4,  5), S(  9,  6), S( 12, -2), S( 22, -8), S( 33,-24) },

    { S( 36, 67), S( 36, 78), S( -3, 98), S(  4, 88), S( 14, 88), S( 15, 88), S( 34, 83), S( 50, 75),
      S( -1, 74), S(-17, 84), S( 11, 82), S( 18, 85), S(  8, 83), S( 19, 52), S(  4, 62), S( 27, 51),
      S( -3, 68), S( 50, 45), S( 26, 64), S( 53, 46), S( 70, 33), S( 65, 44), S( 89, 18), S( 30, 46),
      S( -8, 64), S( 14, 58), S( 25, 60), S( 47, 48), S( 41, 44), S( 46, 36), S( 44, 32), S( 25, 43),
      S(-23, 43), S(-21, 60), S(-20, 58), S(-12, 48), S(-16, 44), S(-20, 45), S(  5, 36), S(-12, 30),
      S(-30, 20), S(-22, 34), S(-30, 29), S(-23, 21), S(-22, 21), S(-22, 11), S(  7,  6), S(-18,  5),
      S(-49, 21), S(-26, 19), S(-17, 22), S(-13, 14), S( -9,  7), S(-18,  0), S( -7, -6), S(-44, 12),
      S(-23, 26), S(-22, 25), S(-20, 28), S(-10, 12), S(-11, 10), S( -9, 19), S( -3, 14), S(-19, 10) },

    { S(-32, 87), S(-10,101), S( -5,128), S( 10,135), S(  7,157), S( 45,146), S( 43,134), S( 31,125),
      S(-18, 57), S(-60, 99), S(-29,105), S(-84,199), S(-80,239), S(-15,179), S(-43,165), S( 16,150),
      S(-15, 44), S(  0, 33), S(-10, 79), S(-15,122), S(  7,164), S( 42,167), S( 63,126), S( 14,149),
      S(  3, 17), S(  2, 60), S(-11, 70), S(-15,130), S(-12,167), S(  0,182), S( 40,166), S( 18,133),
      S( 12, -3), S( 10, 45), S(  1, 50), S( -6, 95), S( -8,103), S( 11, 87), S( 27, 72), S( 39, 76),
      S(  6,-26), S( 19,  3), S(  9, 27), S(  4, 21), S(  8, 24), S(  8, 27), S( 35, -6), S( 28,-13),
      S( 10,-37), S( 15,-28), S( 20,-40), S( 15, -3), S( 16,-13), S( 17,-95), S( 29,-125), S( 23,-78),
      S(  2,-44), S( -6,-47), S( -2,-42), S(  7,-44), S(  6,-47), S(-16,-53), S( -1,-83), S( 12,-57) },

    { S(-99,-142), S(-50,-57), S(-64,-30), S(-66,  8), S(-75,-16), S(-50, -4), S(-32, 13), S(-60,-133),
      S(-81,-46), S(-21, 35), S(-20, 37), S(  3, 24), S(  6, 25), S( 14, 40), S( 20, 57), S(-33,-30),
      S(-40,-11), S( 58, 35), S( 45, 57), S( 17, 62), S( 53, 56), S( 87, 63), S( 72, 49), S(-30,-11),
      S(-37, -8), S( 34, 23), S( 28, 63), S(-10, 83), S(  3, 79), S( 53, 55), S( 43, 28), S(-73, -3),
      S(-46,-45), S( 47, -1), S( 54, 38), S(-30, 76), S( 15, 65), S( 50, 40), S( 48,  6), S(-111,-14),
      S(-38,-45), S( 44,-19), S( 18, 12), S( -4, 34), S( 17, 30), S(  4, 24), S( 16, -7), S(-68,-23),
      S(  5,-42), S(  5,-22), S(-14, -3), S(-70, 13), S(-53, 12), S(-41, 11), S( -3,-15), S(-12,-48),
      S(-35,-116), S( 15,-63), S(-20,-34), S(-110,-41), S(-46,-70), S(-105,-15), S( -5,-63), S(-28,-126) },
}};

// Initialize the piece square tables with piece values included
CONSTR InitPSQT() {

    // Black scores are negative (white double negated -> positive)
    for (PieceType pt = PAWN; pt <= KING; ++pt)
        for (Square sq = A1; sq <= H8; ++sq) {
            // Base piece value + the piece square value
            PSQT[0][MakePiece(BLACK, pt)][sq] = -(PieceTypeValue[pt] + PieceSqValue[0][pt-1][sq]);
            // Same score inverted used for white on the square mirrored horizontally
            PSQT[0][MakePiece(WHITE, pt)][MirrorSquare(sq)] = -PSQT[0][pt][sq];

            // Base piece value + the piece square value
            PSQT[1][MakePiece(BLACK, pt)][sq] = -(PieceTypeValue[pt] + PieceSqValue[1][pt-1][sq]);
            // Same score inverted used for white on the square mirrored horizontally
            PSQT[1][MakePiece(WHITE, pt)][MirrorSquare(sq)] = -PSQT[1][pt][sq];
        }
}
