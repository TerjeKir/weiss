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

// Blacks point of view - makes it easier to visualize as the board isn't upside down
static const int PieceSqValue[7][64] = {
    { 0 }, // Unused
    { // Pawn
             0,          0,          0,          0,          0,          0,          0,          0,
    S( 38, 49), S( 26, 37), S( 28, 29), S( 28, 26), S( 35,  7), S( 13,  8), S( 20, 20), S( 15, 26),
    S( 16, 59), S( 19, 39), S( 22, 24), S( 12,  1), S( 19,  2), S( 40, 16), S( 29, 18), S( 26, 27),
    S( -5, 34), S(-13, 24), S( -9, 11), S( 12,-19), S( 26,-13), S( 36, -5), S(  7, 12), S(  2, 13),
    S(-11, 14), S(-21, 13), S( -5,-11), S(  0,-13), S(  8,-10), S( 11, -9), S(  2,  1), S( -4, -4),
    S(-12,  2), S(-26, -1), S(-14, -3), S( -9, -6), S(  2,  3), S(  4,  4), S( 17, -8), S(  6,-11),
    S( -2,  8), S(-17,  4), S( -9,  4), S( 10, -9), S(  0, 12), S( 37, 11), S( 43, -6), S( 16,-22),
             0,          0,          0,          0,          0,          0,          0,          0},

    { // Knight
    S(-44,-60), S( -8,-10), S(-14,-11), S( -2, -9), S(  1, -3), S(-10, -9), S(-16,-17), S(-55,-45),
    S(-26,-22), S( -7,  5), S( 12, -3), S( 18, 14), S(  7, 19), S( 12, 10), S(  5, -9), S(-28,-20),
    S(-19,-14), S(  3, 10), S( 19, 16), S( 41, 21), S( 43, 28), S( 28, 15), S( 22, 13), S( -2, -8),
    S( -1,-10), S(  8,  9), S( 28, 27), S( 47, 37), S( 30, 42), S( 46, 35), S( 30, 19), S( 16,  7),
    S( -4, -5), S(  8,  7), S(  9, 20), S( 13, 31), S( 20, 34), S( 17, 24), S( 31, 11), S( 25,  1),
    S(-33,-19), S(-18,-12), S(-18, -7), S(  2,  9), S(  8,  5), S(-10,-14), S( -4, -7), S(-15,-19),
    S(-29,-19), S( -7,-12), S(-24,-20), S(-10,-14), S(-17, -9), S(-19,-17), S(-17, -1), S( -9,-14),
    S(-54,-59), S(-25,-22), S(-22,-18), S( -9,-10), S(-14, -7), S(-19,-23), S(-18,-14), S(-47,-42)},

    { // Bishop
    S(  2,  2), S(  3, -7), S( -1,  7), S(  0, -4), S(  0,  1), S( -5, -2), S( -5,  0), S(  0, -1),
    S( -9, -5), S(  1,  2), S( -2,  3), S(  7,  8), S( 13, 14), S( -1, -3), S( -1,  3), S(  3,  0),
    S( -1,  4), S(  6,  8), S( 14, 17), S( 19, 15), S( 18, 17), S( 24, 24), S( 12,  8), S( 16,  8),
    S( -6, -2), S( 14, 16), S( 16, 14), S( 44, 24), S( 31, 25), S( 28, 14), S( 18, 18), S(  2,  5),
    S( -6,  6), S(  5, 10), S(  5, 20), S( 25, 19), S( 29, 23), S( -6, 17), S(  1, 10), S( 11, -2),
    S( -4,  0), S( 10,  8), S(  4, 14), S(  8, 16), S(  6, 17), S(  9,  8), S( 13, -4), S( 15,  4),
    S(  7, -8), S(  6, -5), S( 11,  1), S(-10,  2), S(-11,  2), S( -3, -8), S( 21, -2), S(  4, -7),
    S(  6,  1), S(  9,  3), S( -6,-13), S(-14, -9), S(-16, -3), S(-12, -7), S( -3,  0), S(  1, -3)},

    { // Rook
    S( 17, 31), S( 11, 26), S( 18, 29), S( 10, 25), S( 24, 18), S(  6, 19), S( 11, 17), S(  3, 23),
    S( 23, 31), S( 17, 35), S( 32, 37), S( 34, 44), S( 29, 42), S( 24, 37), S( 31, 30), S( 32, 28),
    S(  4, 22), S( 15, 27), S( 17, 26), S( 26, 29), S( 31, 20), S( 25, 23), S( 15, 14), S( 19,  7),
    S(  0, 14), S(  8, 13), S( 17, 20), S( 22, 24), S( 21, 19), S( 18, 16), S(  9,  8), S(  9, 15),
    S(-16,  6), S( -1,  5), S( -4, 10), S(  3,  7), S( -3,  7), S(  6,  6), S(  3, 10), S( 10, -5),
    S(-23,-13), S(-19,  1), S(-16, -8), S(-12,-12), S(-12,-10), S(-12, -2), S( 13, -1), S( -7,-10),
    S(-17,-14), S(-18,-10), S(-11,-16), S(-14,-15), S(-11,-17), S( -5,-16), S( -4, -4), S(-14,  0),
    S(-19,-11), S(-10,-14), S( -6,-10), S(  3,-20), S( -1,-19), S( -5, -2), S(  8, -7), S(-20,-17)},

    { // Queen
    S( -8,  0), S(  2,  1), S( 15,  1), S(  4, 12), S(  2,  3), S( 10,  6), S( -6,-10), S(  7,  5),
    S( -8, -8), S(-53,  2), S(-11,  5), S( -4,  5), S(  2, 26), S( 22, 12), S(  2,  4), S( 22,  3),
    S(-10,  1), S( -3,-11), S( -8, -3), S(  4, -3), S( 24, 13), S( 25, 18), S( 35, 25), S( 49, 16),
    S(-14,-11), S( -9,  3), S( -2,-10), S( -3,  8), S( 17, 13), S( 23, 20), S( 38, 16), S( 49, 17),
    S(-13,  0), S(-10, -3), S( -9, -1), S( -9, 10), S( -3,  2), S( 12, 16), S( 22, 14), S( 31, 13),
    S(-12, -6), S( -7,-13), S( -9,  3), S(-16,-14), S(-12,-19), S( -9, 10), S( 14,-11), S(  4,  8),
    S( -8, -3), S(-15, -8), S( -5,-22), S(-10,-18), S(-11,-19), S(-19, -3), S(-18,-11), S( -6,-15),
    S(-14, -3), S( -7,-14), S(-12,-16), S( -3,-18), S(-17, -9), S(-24,-20), S( -7,-13), S(  6, -5)},

    { // King
    S(-73,-48), S(-54,-26), S(-72,  7), S(-66, 21), S(-76,  6), S(-70,-25), S(-67,-15), S(-64,-64),
    S(-62,-32), S(-33, 11), S(-56, 33), S(-63, 11), S(-37, 24), S(-54, 25), S(-38, 11), S(-54,-29),
    S(-25, -8), S(-23, 43), S(-23, 49), S( -7, 42), S(-12, 36), S( -7, 50), S(-18, 37), S(-39, -7),
    S(-16,  7), S(-11, 38), S(-25, 50), S(  2, 44), S(-21, 53), S(  0, 44), S(-19, 44), S(-25, -6),
    S( 10,-11), S( -5, 28), S(  1, 39), S(  0, 47), S( -8, 50), S( -7, 38), S(  4, 19), S(-19,-25),
    S(  4,-21), S( 30, -8), S(  4, 15), S(-19, 33), S(  5, 28), S(  8, 17), S( 14, -4), S(  4,-32),
    S( 43,-25), S( 26,-12), S( 22, -6), S(-25,  2), S(-22,  4), S( -4, -1), S( 54,-31), S( 62,-63),
    S(  5,-59), S( 83,-59), S( 46,-38), S(-60,-16), S( 17,-66), S(-49,-31), S( 61,-62), S( 55,-123)}
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
