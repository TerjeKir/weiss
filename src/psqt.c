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
    S( 47, 68), S( 34, 55), S( 34, 36), S( 31, 16), S( 36,  2), S( 13,  7), S( 10, 20), S(  9, 33),
    S(  1, 74), S( 19, 57), S( 28, 31), S( 20, -4), S( 31, -6), S( 76, 16), S( 37, 27), S( 19, 35),
    S( -6, 36), S(-10, 24), S( -5,  9), S( 16,-22), S( 27,-16), S( 35,-10), S(  8,  7), S(  2,  9),
    S(-13, 13), S(-19, 10), S( -5,-10), S( -2,-16), S(  7,-15), S(  9,-12), S(  1, -6), S( -6, -7),
    S(-17,  3), S(-26, -3), S(-17, -4), S(-11, -8), S( -2, -2), S(  1, -2), S( 11,-16), S(  2,-15),
    S( -8, 11), S(-16,  3), S(-12,  5), S(  0, -1), S( -5, 14), S( 29,  6), S( 33,-12), S( 12,-24),
             0,          0,          0,          0,          0,          0,          0,          0},

    { // Knight
    S(-53,-65), S( -9,-10), S(-15, -7), S( -2, -8), S(  2, -2), S(-11, -6), S(-16,-16), S(-56,-46),
    S(-21,-19), S(-11, -1), S( 19, -6), S( 29, 22), S( 18, 23), S( 18,  2), S(  3, -9), S(-22,-16),
    S(-20,-15), S(  6,  2), S( 30, 26), S( 49, 25), S( 60, 28), S( 50, 36), S( 35, 13), S(  7, -4),
    S(  3, -5), S(  7,  9), S( 27, 30), S( 41, 46), S( 25, 51), S( 46, 37), S( 24, 27), S( 33, 14),
    S( -7, -4), S(  4,  5), S(  4, 30), S(  7, 37), S( 17, 35), S( 11, 35), S( 42, 13), S( 19, 10),
    S(-35,-29), S(-20,-19), S(-20, -9), S( -4, 12), S(  0,  9), S(-13,-14), S( -7,-14), S(-20,-24),
    S(-34,-20), S(-26,-14), S(-30,-30), S(-14,-16), S(-19,-15), S(-23,-30), S(-28, -6), S(-11,-12),
    S(-54,-59), S(-28,-36), S(-33,-26), S(-20,-10), S(-17,-10), S(-24,-27), S(-26,-22), S(-47,-43)},

    { // Bishop
    S( -3,  2), S(  2, -3), S( -5,  8), S( -3,  4), S( -1,  6), S(-10, -1), S( -5,  4), S( -1,  0),
    S(-20, -3), S(  0,  9), S( -4,  8), S(  2,  9), S(  8, 12), S(  2,  7), S( -4,  8), S(  0,  1),
    S( -3,  5), S( 10, 17), S( 24, 18), S( 22, 14), S( 30, 20), S( 36, 33), S( 26, 21), S( 21, 12),
    S(-16,  1), S( 15, 16), S( 14, 13), S( 53, 20), S( 31, 27), S( 30, 16), S( 13, 23), S( -1, 13),
    S(-12,  1), S(  2,  5), S(  6, 21), S( 22, 19), S( 28, 16), S( -5, 16), S(  1,  8), S(  6, -3),
    S( -6, -1), S( 10,  7), S(  2, 12), S(  7, 14), S(  3, 12), S(  7,  5), S( 10, -4), S( 12,  6),
    S( 11, -8), S(  5,-16), S(  7, -7), S(-11, -3), S(-10, -2), S( -4,-17), S( 16,-14), S(  5,-17),
    S( 14, -3), S(  8,  2), S(-11,-10), S(-21, -9), S(-24, -6), S(-13, -6), S( -4, -1), S(  9, -4)},

    { // Rook
    S( 27, 43), S( 21, 48), S( 23, 46), S( 19, 44), S( 33, 39), S( 16, 46), S( 21, 43), S( 17, 50),
    S( 13, 37), S(  7, 42), S( 31, 41), S( 41, 47), S( 37, 49), S( 34, 36), S( 32, 31), S( 36, 31),
    S( -1, 29), S( 28, 24), S( 24, 32), S( 40, 26), S( 50, 24), S( 46, 35), S( 38, 23), S( 31, 21),
    S( -9, 21), S(  7, 21), S( 13, 24), S( 33, 20), S( 27, 20), S( 32, 21), S( 25, 17), S( 19, 20),
    S(-28,  4), S(-15, 12), S(-21, 14), S(-13,  6), S(-17,  6), S( -5,  9), S(  8, 12), S(  2, -3),
    S(-39,-20), S(-26, -6), S(-32,-13), S(-23,-19), S(-26,-18), S(-20,-12), S( 11,-11), S(-18,-18),
    S(-49,-28), S(-29,-26), S(-23,-23), S(-19,-26), S(-19,-27), S( -9,-31), S( -9,-20), S(-33,-16),
    S(-23,-22), S(-15,-18), S(-11,-18), S( -3,-28), S( -6,-30), S(-10,-15), S(  1,-19), S(-16,-34)},

    { // Queen
    S( -5,  3), S(  7,  8), S( 20,  9), S( 14, 24), S( 17, 23), S( 20, 19), S(  4,  2), S( 19, 16),
    S(-13, -6), S(-63, 13), S(-11, 10), S( -3, 15), S( 10, 42), S( 36, 28), S(  1,  9), S( 26, 13),
    S(-20, -4), S( -8,-13), S(-11,  0), S(  8,  7), S( 36, 27), S( 57, 45), S( 69, 44), S( 70, 34),
    S(-17,-15), S(-12,  5), S(-11,-10), S( -6, 19), S( 16, 29), S( 30, 38), S( 48, 35), S( 45, 30),
    S(-18, -7), S( -9, -3), S(-14, -1), S(-18, 22), S(-11, 13), S(  5, 25), S( 11, 17), S( 28, 21),
    S(-21,-16), S(-11,-17), S(-14, -1), S(-19,-19), S(-17,-24), S(-14,  3), S(  6,-18), S(  1,  4),
    S(-21,-14), S(-17,-22), S( -6,-52), S(-11,-34), S(-11,-42), S(-14,-39), S(-16,-33), S(-11,-20),
    S(-22,-17), S(-16,-30), S(-17,-39), S( -8,-46), S(-18,-38), S(-37,-39), S(-17,-21), S(-12,-13)},

    { // King
    S(-73,-49), S(-54,-27), S(-72,  6), S(-66, 20), S(-76,  5), S(-70,-25), S(-67,-15), S(-64,-65),
    S(-62,-33), S(-32, 17), S(-55, 34), S(-62, 12), S(-37, 24), S(-53, 26), S(-37, 22), S(-54,-30),
    S(-25, -7), S(-22, 49), S(-20, 56), S( -6, 43), S(-11, 38), S( -4, 59), S(-15, 52), S(-39, -6),
    S(-16,  4), S(-10, 42), S(-21, 60), S(  5, 58), S(-18, 62), S(  4, 55), S(-16, 44), S(-25, -9),
    S(  9,-20), S( -4, 24), S(  5, 46), S(  3, 60), S( -1, 60), S(  0, 43), S( 13, 19), S(-20,-31),
    S(  2,-28), S( 29, -6), S(  7, 20), S(-16, 40), S(  8, 32), S( 10, 18), S( 26,-11), S(  0,-41),
    S( 46,-38), S( 32,-19), S( 16, -5), S(-40,  6), S(-23,  4), S(-22,  2), S( 42,-33), S( 56,-74),
    S( 10,-70), S( 67,-65), S( 25,-43), S(-42,-35), S(  6,-77), S(-52,-33), S( 53,-69), S( 53,-139)},
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
