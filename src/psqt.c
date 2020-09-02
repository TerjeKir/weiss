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
    S( 42, 59), S( 30, 45), S( 30, 33), S( 29, 24), S( 36,  7), S( 13,  8), S( 17, 20), S( 14, 29),
    S(  9, 66), S( 19, 48), S( 23, 28), S( 12, -1), S( 21, -2), S( 52, 18), S( 32, 21), S( 24, 30),
    S( -7, 35), S(-13, 25), S( -8, 10), S( 14,-21), S( 26,-15), S( 37,-10), S(  9, 10), S(  4, 10),
    S(-13, 14), S(-21, 13), S( -5,-10), S( -2,-15), S(  8,-13), S( 11,-11), S(  5, -3), S( -3, -6),
    S(-16,  3), S(-27,  0), S(-16, -3), S(-10, -7), S(  0,  0), S(  4,  0), S( 15,-12), S(  5,-14),
    S( -7, 10), S(-17,  5), S(-10,  5), S(  5, -7), S( -3, 14), S( 33,  8), S( 39, -9), S( 16,-23),
             0,          0,          0,          0,          0,          0,          0,          0},

    { // Knight
    S(-47,-61), S( -8,-10), S(-14,-10), S( -2, -9), S(  1, -3), S(-10, -9), S(-16,-17), S(-55,-45),
    S(-25,-21), S( -8,  3), S( 14, -4), S( 21, 17), S( 10, 20), S( 13,  7), S(  5, -9), S(-27,-19),
    S(-20,-14), S(  3,  7), S( 23, 20), S( 44, 22), S( 48, 28), S( 35, 22), S( 26, 13), S(  1, -7),
    S(  1, -8), S(  7,  8), S( 28, 27), S( 45, 40), S( 28, 46), S( 48, 36), S( 27, 22), S( 25, 11),
    S( -4, -5), S(  7,  6), S(  7, 24), S( 10, 34), S( 19, 34), S( 15, 28), S( 38, 13), S( 25,  4),
    S(-34,-22), S(-19,-15), S(-19, -9), S(  0,  9), S(  4,  6), S(-11,-15), S( -5,-10), S(-16,-20),
    S(-30,-19), S(-12,-13), S(-27,-24), S(-12,-15), S(-18,-12), S(-21,-21), S(-21, -2), S( -9,-14),
    S(-54,-59), S(-26,-26), S(-26,-21), S(-11,-10), S(-15, -8), S(-20,-24), S(-22,-17), S(-47,-42)},

    { // Bishop
    S(  1,  2), S(  3, -6), S( -2,  7), S(  0, -1), S(  0,  2), S( -6, -2), S( -5,  1), S(  0, -1),
    S(-12, -5), S(  1,  5), S( -3,  4), S(  6,  8), S( 12, 14), S(  0,  0), S( -1,  4), S(  3,  0),
    S( -2,  4), S(  7, 12), S( 17, 18), S( 20, 15), S( 22, 18), S( 28, 27), S( 16, 12), S( 18,  9),
    S(-10, -2), S( 14, 16), S( 15, 14), S( 49, 23), S( 31, 25), S( 29, 15), S( 15, 19), S(  2,  8),
    S( -9,  5), S(  4,  9), S(  5, 21), S( 24, 19), S( 28, 19), S( -6, 17), S(  1, 10), S( 10, -2),
    S( -5,  0), S( 10,  8), S(  3, 14), S(  7, 16), S(  5, 16), S(  8,  7), S( 12, -4), S( 15,  4),
    S(  8, -8), S(  5,-10), S(  9, -2), S(-10,  0), S(-10,  1), S( -3,-11), S( 18, -8), S(  5,-10),
    S(  9,  1), S( 10,  3), S( -8,-13), S(-17, -9), S(-20, -4), S(-11, -7), S( -3,  0), S(  3, -3)},

    { // Rook
    S( 21, 36), S( 15, 35), S( 21, 36), S( 14, 33), S( 27, 25), S(  9, 27), S( 14, 25), S(  7, 32),
    S( 20, 32), S( 14, 37), S( 32, 38), S( 37, 46), S( 32, 46), S( 26, 36), S( 31, 30), S( 33, 28),
    S(  3, 25), S( 19, 27), S( 20, 30), S( 31, 29), S( 37, 23), S( 32, 28), S( 22, 18), S( 23, 12),
    S( -3, 17), S(  9, 17), S( 17, 22), S( 26, 24), S( 25, 20), S( 24, 20), S( 14, 12), S( 13, 18),
    S(-20,  5), S( -5,  7), S(-10, 10), S( -3,  6), S( -8,  6), S(  3,  7), S(  5, 11), S(  8, -4),
    S(-30,-16), S(-23, -2), S(-22,-11), S(-16,-16), S(-18,-14), S(-15, -5), S( 13, -4), S(-10,-12),
    S(-27,-21), S(-23,-16), S(-17,-20), S(-17,-20), S(-16,-22), S( -7,-21), S( -6, -8), S(-19, -4),
    S(-20,-18), S(-12,-16), S( -8,-15), S(  0,-24), S( -3,-24), S( -6, -8), S(  4,-11), S(-18,-23)},

    { // Queen
    S( -8,  0), S(  3,  2), S( 16,  3), S(  7, 15), S(  6,  8), S( 13,  9), S( -3, -7), S( 10,  8),
    S(-10, -8), S(-58,  4), S(-11,  6), S( -4,  7), S(  5, 30), S( 26, 16), S(  2,  5), S( 25,  6),
    S(-15, -1), S( -5,-12), S(-10, -3), S(  5,  0), S( 28, 17), S( 35, 26), S( 47, 31), S( 59, 22),
    S(-16,-12), S(-11,  3), S( -6,-11), S( -4, 11), S( 18, 17), S( 28, 25), S( 45, 22), S( 51, 21),
    S(-15, -2), S( -9, -3), S(-11, -1), S(-13, 13), S( -4,  5), S( 11, 19), S( 19, 14), S( 33, 15),
    S(-15, -9), S( -9,-14), S(-10,  2), S(-17,-15), S(-14,-20), S(-11,  9), S( 11,-13), S(  5,  8),
    S(-12, -6), S(-16,-11), S( -6,-31), S(-10,-22), S(-12,-26), S(-18,-11), S(-17,-16), S( -6,-16),
    S(-18, -7), S(-11,-18), S(-15,-22), S( -7,-26), S(-17,-16), S(-30,-25), S(-10,-15), S(  3, -7)},

    { // King
    S(-73,-48), S(-54,-26), S(-72,  7), S(-66, 21), S(-76,  6), S(-70,-25), S(-67,-15), S(-64,-64),
    S(-62,-32), S(-33, 12), S(-56, 33), S(-63, 11), S(-37, 24), S(-54, 25), S(-38, 14), S(-54,-29),
    S(-25, -8), S(-23, 45), S(-22, 51), S( -7, 42), S(-12, 36), S( -6, 53), S(-17, 41), S(-39, -7),
    S(-16,  7), S(-11, 39), S(-24, 53), S(  3, 48), S(-20, 56), S(  1, 48), S(-18, 44), S(-25, -6),
    S( 10,-13), S( -5, 27), S(  2, 41), S(  1, 52), S( -5, 54), S( -4, 40), S(  7, 20), S(-19,-26),
    S(  4,-22), S( 30, -7), S(  5, 17), S(-18, 36), S(  6, 30), S(  9, 17), S( 18, -6), S(  3,-35),
    S( 43,-28), S( 27,-13), S( 21, -6), S(-29,  2), S(-22,  3), S( -9, -1), S( 49,-32), S( 62,-68),
    S(  7,-61), S( 81,-62), S( 39,-41), S(-60,-21), S( 14,-72), S(-53,-31), S( 59,-65), S( 57,-131)},
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
