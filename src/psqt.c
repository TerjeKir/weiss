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


int PSQT[PIECE_NB][64];

// Blacks point of view - makes it easier to visualize as the board isn't upside down
tuneable_static_const int PieceSqValue[7][64] = {
    { 0 }, // Unused
    { // Pawn
             0,          0,          0,          0,          0,          0,          0,          0,
    S( 29, 28), S( 20, 23), S( 25, 23), S( 27, 31), S( 35,  9), S( 12,  8), S( 24, 21), S( 17, 24),
    S( 21, 27), S( 14, 13), S( 18, 19), S( 21, 20), S( 19, 16), S( 15,  4), S( 25, 14), S( 24, 22),
    S(  3, 18), S( -2, 12), S( -2,  8), S(  4,  1), S( 22,  0), S( 14,  2), S(  3,  3), S(  6, 11),
    S( -4,  4), S(-19,  8), S(  1, -1), S( 16,  9), S( 14,  9), S( 11, -3), S(-13,  3), S( -1,  1),
    S( -1,  0), S( -8, -2), S( -3, -1), S(  2,  4), S(  9,  6), S( -4,  2), S( 10,  6), S(  6,  4),
    S( 11,  5), S(  0,  0), S(  1, -5), S(  1,-15), S(  3, -2), S( 25,  8), S( 21,  7), S(  8,  0),
             0,          0,          0,          0,          0,          0,          0,          0},

    { // Knight
    S(-40,-58), S( -8,-10), S(-13,-13), S( -2,-10), S(  1, -4), S(-10,-10), S(-16,-17), S(-54,-44),
    S(-28,-23), S( -3,  9), S( 10, -1), S( 14,  9), S(  3, 17), S(  9, 14), S(  5, -9), S(-30,-21),
    S(-17,-13), S(  4, 14), S(  8,  4), S( 32, 14), S( 33, 26), S( 15,  1), S( 14, 12), S( -7, -9),
    S( -7,-15), S(  3,  7), S( 24, 23), S( 37, 24), S( 31, 29), S( 27, 26), S( 21,  9), S(-10, -3),
    S(-10, -6), S( 12,  9), S( 18,  9), S( 18, 18), S( 25, 35), S( 23, 13), S( 11,  3), S(  5, -8),
    S(-26,-12), S(-13, -6), S( -8,  4), S(  3,  8), S( 15,  6), S( -3,  7), S(  6,  1), S(-16,-16),
    S(-26,-18), S(  0, -9), S( -1, -7), S(  0, -7), S( -5,  2), S( -2, -4), S( -6,  4), S(-16,-16),
    S(-53,-58), S(-13,-10), S(-15,-14), S( -6, -8), S(-13, -4), S(-14,-19), S(-11, -9), S(-46,-42)},

    { // Bishop
    S(  4,  2), S(  3, -9), S(  0,  7), S(  1, -8), S(  1, -1), S( -3, -3), S( -5, -1), S(  0, -1),
    S( -4, -6), S(  1, -5), S(  0,  1), S(  9,  7), S( 15, 15), S( -3, -8), S( -1,  3), S(  5,  1),
    S(  4,  5), S(  4,  0), S(  6, 13), S( 16, 12), S( 12, 13), S( 16, 16), S(  3, -1), S(  6,  3),
    S(  6, -1), S(  8, 11), S( 18, 14), S( 25, 19), S( 24, 20), S( 24, 11), S( 16, 13), S( -2, -1),
    S( -1, 11), S(  9, 12), S( 10, 17), S( 21, 18), S( 20, 28), S( 12, 20), S(  1, 12), S(  6, -3),
    S(  1,  2), S(  6,  6), S(  6,  9), S( 17, 14), S( 12, 20), S(  9, 10), S( 15, -2), S(  5,  0),
    S(  3, -8), S(  6,  7), S(  6,  6), S( -6,  7), S(  3, 13), S(  2,  0), S( 20, 11), S(  0, -2),
    S( -2,  2), S(  2,  2), S(  2,-11), S(  1, -1), S( -2,  3), S(-13, -6), S( -3,  1), S( -1, -3)},

    { // Rook
    S(  7,  9), S( -1, -2), S( 13, 13), S(  3,  7), S( 18,  3), S(  0,  3), S(  5,  1), S( -6,  3),
    S( 27, 27), S( 21, 28), S( 29, 30), S( 27, 34), S( 21, 31), S( 21, 39), S( 31, 33), S( 29, 28),
    S(  1, 11), S(  1, 18), S(  3,  6), S( 10, 18), S( 17, 11), S( 10,  7), S(  3,  5), S(  9, -8),
    S( -1,  2), S( -2, -5), S( 10,  8), S(  6, 14), S(  7,  8), S(  2,  3), S( -2, -3), S(  0,  7),
    S( -8,  8), S(  2, -1), S(  7,  9), S( 16, 14), S( 10, 14), S(  7,  2), S( -2,  6), S(  8, -9),
    S( -9, -3), S(-10,  9), S(  0,  3), S(  4,  3), S( 11,  6), S( -4,  5), S( 10,  1), S( -3, -6),
    S(  2,  1), S( -6,  3), S(  7, -1), S(  6,  6), S( 12,  4), S( -2, -6), S(  0,  3), S( -6,  7),
    S( -5, 14), S(  2, -7), S(  2,  6), S(  5,  1), S(  5,  1), S( -4, 14), S( 12,  1), S( -2,  2)},

    { // Queen
    S( -9, -1), S(  0, -1), S( 13, -2), S(  0,  8), S( -4, -5), S(  6,  1), S(-10,-14), S(  1,  1),
    S(  1, -6), S(-33,  1), S( -9,  5), S( -5,  2), S( -4, 18), S( 13,  4), S( -1,  1), S( 12, -3),
    S(  2,  6), S(  4, -8), S( -2, -2), S(  3, -6), S( 17,  6), S(  4,  3), S( 10, 12), S( 19,  1),
    S(-10, -8), S( -3,  4), S(  8, -7), S(  1,  5), S( 13,  4), S(  9,  8), S( 13,  1), S( 26,  4),
    S( -1,  5), S(-13, -4), S( -4,  0), S( -2,  6), S( -7, -5), S( -1,  7), S( 22, 12), S( 17,  5),
    S( -4, -2), S( -2,-13), S(-12,  4), S( -4,-10), S( -7,-17), S( -3, 12), S( 22, -7), S(  0,  8),
    S(  2,  3), S( -4,  0), S(  0, -3), S( -2, -7), S( -2, -3), S( -8, 15), S(-16, -3), S( -6,-14),
    S( -3,  4), S(  0, -7), S(-10, -5), S( -8, -2), S(-11,  4), S( -7,-10), S( -3,-10), S(  9, -3)},

    { // King
    S(-73,-47), S(-54,-26), S(-72,  7), S(-66, 21), S(-76,  6), S(-70,-25), S(-67,-15), S(-64,-63),
    S(-62,-32), S(-34,  8), S(-57, 32), S(-63, 10), S(-37, 24), S(-54, 24), S(-39,  6), S(-54,-28),
    S(-25, -9), S(-24, 38), S(-24, 46), S( -7, 42), S(-12, 36), S( -8, 46), S(-20, 27), S(-39, -7),
    S(-16,  7), S(-12, 36), S(-27, 43), S(  1, 39), S(-22, 51), S( -3, 34), S(-20, 43), S(-25, -6),
    S( 11, -7), S( -5, 31), S( -1, 37), S( -2, 35), S(-13, 36), S(-12, 28), S( -2,  8), S(-19,-26),
    S(  5,-18), S( 30,-10), S(  2, 13), S(-23, 24), S(  0, 18), S(  1,  3), S(  3, -1), S(  5,-29),
    S( 43,-22), S( 25, -7), S( 23, -5), S(-16, 10), S(-23, 11), S(  7,  0), S( 63,-24), S( 42,-48),
    S(  5,-57), S( 80,-57), S( 66,-27), S(-53, -3), S( 37,-43), S(-33,-24), S( 69,-63), S( 43,-102)}
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
