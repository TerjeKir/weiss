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
    S(-41,-54), S( -4,-11), S(-10,-13), S( -2,-11), S(  2, -4), S( -8,-14), S(-15,-14), S(-50,-51),
    S(-30,-25), S( -2,  7), S( 12, -2), S( 11,  9), S(  6,  8), S(  7, 11), S(  4, -6), S(-30,-24),
    S(-13,-11), S(  7, 17), S(  7,  8), S( 29, 18), S( 30, 25), S( 12,  3), S( 11, 12), S( -7,-11),
    S( -9,-12), S(  2,  8), S( 22, 24), S( 32, 21), S( 27, 24), S( 28, 18), S( 15,  7), S( -9, -8),
    S( -9, -8), S(  9,  7), S( 16, 11), S( 16, 22), S( 26, 31), S( 19, 13), S(  5,  2), S(  0,-10),
    S(-22, -9), S( -6, -1), S( -7,  8), S( 10,  9), S( 15,  7), S(  3,  8), S(  7,  1), S(-15,-14),
    S(-28,-21), S(  4, -2), S( -3, -3), S(  4, -3), S( -5,  3), S( -3,  2), S( -6,  2), S(-18,-20),
    S(-56,-56), S(-14,-10), S(-15,-13), S( -9, -7), S(-13, -6), S(-17,-14), S(-11, -9), S(-44,-46)},

    { // Bishop
    S(  0, -3), S( -1, -4), S(  1,  2), S(  0, -2), S( -3,  1), S( -3,  0), S(  0,  1), S( -3,  0),
    S( -5,  1), S( -1, -2), S( -1,  1), S( 10, 10), S( 11, 12), S(  1, -2), S(  5,  2), S(  0,  0),
    S(  4,  1), S(  0, -1), S(  7, 10), S( 11, 14), S( 12, 16), S( 13, 16), S(  2, -1), S(  3,  3),
    S(  2,  0), S( 11, 12), S( 16, 15), S( 21, 16), S( 21, 20), S( 15, 13), S( 11, 10), S(  1,  0),
    S( -2,  5), S( 11, 11), S( 14, 15), S( 19, 21), S( 21, 24), S( 14, 17), S(  5, 13), S(  4,  2),
    S(  3,  0), S( -1,  3), S(  8,  6), S( 20, 12), S( 13, 18), S(  8, 10), S(  3,  0), S(  6,  0),
    S(  2, -1), S( 10,  9), S(  2,  0), S(  2,  9), S(  6, 11), S(  0,  2), S( 12, 11), S( -2,  0),
    S(  0,  1), S(  0,  3), S( -7, -8), S(  3,  0), S( -1,  2), S( -9, -9), S( -6, -1), S( -1,  0)},

    { // Rook
    S(  2,  5), S( -1, -5), S(  7, 12), S(  9,  6), S( 16,  8), S( -1,  4), S(  3,  1), S( -5,  3),
    S( 24, 27), S( 25, 27), S( 22, 28), S( 26, 31), S( 23, 28), S( 25, 34), S( 29, 29), S( 26, 27),
    S(  1,  3), S(  2, 12), S(  9,  4), S(  9, 15), S( 17, 13), S(  9,  7), S(  4,  4), S(  7,  0),
    S(  0,  0), S(  1, -3), S(  6,  3), S(  4, 15), S( 10, 11), S(  2,  4), S(  2, -1), S( -2,  2),
    S( -2,  7), S(  1, -4), S(  5, 10), S( 10, 18), S(  8, 13), S(  4,  0), S( -5,  2), S(  1, -6),
    S( -2, -1), S( -1,  3), S(  1,  2), S(  6,  6), S( 10,  9), S( -2,  6), S(  9, -4), S( -1, -1),
    S(  6,  0), S( -5,  2), S(  9, -3), S(  9,  8), S( 10,  5), S( -2,  0), S(  0, -4), S( -2, 10),
    S( -4,  6), S(  1, -4), S(  3,  7), S(  4, 10), S( 10,  6), S(  1,  9), S(  3,  4), S( -4,  1)},

    { // Queen
    S(-2, 6), S(  2,  3), S( 4,-2), S( 0, 11), S( -2, -6), S(  2,  0), S( -6, -6), S(-4, 0),
    S( 0,-2), S(-21,  4), S(-4,-1), S(-6,  4), S(  0, 10), S( 15,  6), S( -2,  0), S(12,-2),
    S( 0, 2), S(  5, -6), S(-1,-1), S( 2,-11), S( 18,  4), S(  5,  0), S(  6,  4), S(18, 3),
    S(-5,-8), S( -3,  1), S( 4,-4), S(-4,  8), S( 14,  5), S( 13,  9), S(  7,  4), S(19, 6),
    S( 8, 5), S( -3, -3), S(-1, 4), S(-2,  5), S( -4, -6), S(  3,  4), S( 18,  3), S(15, 6),
    S(-7, 1), S(  6,-11), S(-8, 6), S(-3, -6), S( -8,-14), S( -1, 11), S( 15, -2), S( 1,11),
    S( 0, 4), S( -4,  0), S(-7, 0), S(-3, -3), S( -3, -8), S(-11,  9), S(-15, -8), S(-2,-8),
    S(-1, 2), S( -1, -1), S(-2,-5), S(-6, -1), S(-10, -2), S( -6,-11), S(  4, -5), S( 1,-6)},

    { // King
    S(-77,-51), S(-64,-30), S(-63,  4), S(-75, -2), S(-60, -8), S(-71,-13), S(-65,-25), S(-62,-58),
    S(-45,-27), S(-46,-12), S(-48,  9), S(-51,  5), S(-44,  7), S(-54,  9), S(-39,-17), S(-47,-30),
    S(-33,-17), S(-27, 17), S(-30, 23), S(-25, 26), S(-27, 24), S(-20, 23), S(-38,  1), S(-31,-15),
    S(-14,  6), S(-14, 27), S(-23, 25), S(-21, 33), S(-24, 23), S(-14, 21), S(-17, 24), S(-14,  8),
    S( 10, -7), S( -2, 18), S(  3, 23), S(-11, 43), S(-14, 35), S( -6, 21), S(  4, 21), S(  2, -8),
    S( 17,-13), S( 20,  6), S(  3,  9), S(-12, 17), S( -9, 14), S( -3, 18), S( 10,  7), S( 28,-18),
    S( 39,-18), S( 35, -6), S( 14, 11), S( -9, 14), S( -7, 12), S(  3, 12), S( 44, -9), S( 35,-26),
    S( 30,-53), S( 57,-34), S( 42,-10), S( -9, -3), S( 26,  1), S(-13,-18), S( 56,-33), S( 22,-68)}
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