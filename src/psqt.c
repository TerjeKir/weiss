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
    S( 43, 62), S( 31, 48), S( 31, 34), S( 30, 22), S( 36,  6), S( 13,  8), S( 16, 20), S( 13, 30),
    S(  6, 64), S( 19, 50), S( 24, 29), S( 14, -2), S( 24, -2), S( 59, 20), S( 33, 23), S( 23, 31),
    S( -5, 30), S(-11, 20), S( -5,  7), S( 15,-17), S( 26,-10), S( 37, -5), S( 10,  9), S(  5,  9),
    S(-13,  9), S(-19,  7), S( -5,-10), S( -2,-15), S(  7,-12), S( 10, -9), S(  4, -5), S( -2, -7),
    S(-16,  1), S(-26, -6), S(-16, -5), S(-11, -9), S( -1, -2), S(  3, -1), S( 13,-11), S(  5,-12),
    S( -7,  8), S(-16,  1), S(-10,  3), S(  2, -4), S( -3, 12), S( 32, 11), S( 35, -3), S( 14,-18),
             0,          0,          0,          0,          0,          0,          0,          0},

    { // Knight
    S(-48,-62), S( -8,-10), S(-14, -9), S( -2, -9), S(  1, -3), S(-10, -8), S(-16,-17), S(-55,-45),
    S(-24,-21), S( -9,  2), S( 15, -4), S( 23, 19), S( 12, 21), S( 14,  6), S(  5, -9), S(-26,-18),
    S(-20,-14), S(  4,  5), S( 25, 23), S( 46, 25), S( 51, 29), S( 38, 27), S( 28, 14), S(  2, -6),
    S(  2, -7), S(  8,  8), S( 28, 28), S( 45, 43), S( 28, 47), S( 48, 37), S( 26, 24), S( 28, 13),
    S( -4, -4), S(  6,  6), S(  7, 25), S(  9, 33), S( 19, 33), S( 14, 30), S( 40, 14), S( 23,  6),
    S(-34,-24), S(-19,-17), S(-20,-10), S( -2,  9), S(  2,  7), S(-13,-14), S( -7,-11), S(-18,-21),
    S(-31,-19), S(-16,-13), S(-29,-26), S(-13,-15), S(-19,-14), S(-22,-24), S(-23, -4), S( -9,-13),
    S(-54,-59), S(-27,-29), S(-28,-22), S(-14,-10), S(-15, -9), S(-22,-25), S(-23,-18), S(-47,-42)},

    { // Bishop
    S(  0,  2), S(  3, -5), S( -3,  7), S( -1,  0), S(  0,  3), S( -7, -2), S( -5,  2), S(  0, -1),
    S(-14, -5), S(  1,  6), S( -3,  5), S(  5,  8), S( 11, 13), S(  0,  2), S( -2,  5), S(  2,  0),
    S( -2,  4), S(  8, 14), S( 19, 19), S( 21, 15), S( 24, 19), S( 30, 30), S( 18, 15), S( 19, 10),
    S(-12, -2), S( 15, 16), S( 15, 14), S( 51, 25), S( 32, 27), S( 29, 16), S( 14, 20), S(  1, 10),
    S(-10,  4), S(  3,  7), S(  6, 20), S( 23, 20), S( 28, 19), S( -5, 16), S(  1,  9), S(  9, -2),
    S( -5,  0), S( 10,  8), S(  3, 13), S(  8, 15), S(  5, 13), S(  8,  7), S( 11, -3), S( 14,  5),
    S(  9, -8), S(  5,-11), S(  8, -3), S(-10, -3), S(-10, -1), S( -4,-13), S( 16, -8), S(  5,-12),
    S( 11,  0), S(  9,  3), S( -9,-12), S(-19, -9), S(-22, -6), S(-12, -7), S( -3,  0), S(  5, -3)},

    { // Rook
    S( 22, 39), S( 16, 40), S( 21, 40), S( 15, 37), S( 28, 30), S( 10, 33), S( 15, 31), S(  9, 38),
    S( 18, 32), S( 12, 36), S( 32, 39), S( 38, 47), S( 33, 47), S( 28, 36), S( 31, 31), S( 34, 30),
    S(  2, 24), S( 22, 26), S( 21, 31), S( 34, 29), S( 41, 25), S( 35, 31), S( 26, 21), S( 25, 17),
    S( -5, 16), S(  9, 18), S( 16, 22), S( 29, 23), S( 26, 21), S( 27, 21), S( 17, 14), S( 15, 19),
    S(-22,  3), S( -8,  8), S(-13,  9), S( -6,  5), S(-11,  5), S(  1,  7), S(  6, 11), S(  7, -3),
    S(-33,-19), S(-24, -5), S(-25,-13), S(-18,-18), S(-21,-16), S(-16, -8), S( 13, -6), S(-12,-14),
    S(-33,-24), S(-25,-20), S(-19,-22), S(-18,-23), S(-17,-24), S( -7,-24), S( -7,-12), S(-22, -8),
    S(-20,-21), S(-13,-17), S( -8,-16), S(  0,-23), S( -4,-25), S( -6,-13), S(  4,-14), S(-15,-27)},

    { // Queen
    S( -7,  1), S(  4,  4), S( 17,  5), S(  8, 17), S(  8, 12), S( 14, 11), S( -2, -5), S( 12, 10),
    S(-11, -7), S(-59,  6), S(-11,  7), S( -4,  9), S(  6, 33), S( 28, 19), S(  2,  6), S( 25,  8),
    S(-16, -2), S( -6,-12), S(-10, -2), S(  6,  2), S( 30, 20), S( 40, 31), S( 52, 35), S( 63, 26),
    S(-16,-13), S(-11,  4), S( -7,-11), S( -4, 13), S( 18, 21), S( 29, 29), S( 47, 26), S( 50, 24),
    S(-15, -3), S( -8, -3), S(-11, -1), S(-14, 15), S( -6,  7), S( 10, 21), S( 17, 15), S( 32, 17),
    S(-16,-11), S( -9,-14), S(-11,  1), S(-18,-16), S(-15,-21), S(-11,  7), S(  9,-14), S(  5,  7),
    S(-14, -8), S(-16,-14), S( -7,-36), S(-11,-25), S(-12,-30), S(-16,-18), S(-16,-20), S( -7,-17),
    S(-20,-10), S(-13,-21), S(-16,-26), S( -7,-32), S(-17,-22), S(-32,-29), S(-11,-16), S( -1, -8)},

    { // King
    S(-73,-48), S(-54,-26), S(-72,  7), S(-66, 21), S(-76,  6), S(-70,-25), S(-67,-15), S(-64,-64),
    S(-62,-32), S(-33, 13), S(-56, 33), S(-63, 11), S(-37, 24), S(-54, 25), S(-38, 16), S(-54,-29),
    S(-25, -8), S(-23, 45), S(-22, 52), S( -7, 42), S(-12, 36), S( -6, 54), S(-17, 43), S(-39, -7),
    S(-16,  6), S(-11, 39), S(-24, 54), S(  3, 51), S(-20, 57), S(  1, 50), S(-18, 42), S(-25, -8),
    S( 10,-15), S( -5, 25), S(  2, 42), S(  1, 55), S( -5, 55), S( -4, 40), S(  8, 19), S(-19,-29),
    S(  4,-24), S( 30, -6), S(  5, 18), S(-18, 34), S(  6, 30), S(  9, 17), S( 21, -7), S(  3,-38),
    S( 45,-30), S( 29,-15), S( 20, -6), S(-32, -1), S(-22, -1), S(-13, -4), S( 50,-25), S( 63,-61),
    S(  8,-63), S( 77,-59), S( 35,-40), S(-54,-27), S( 12,-72), S(-53,-39), S( 59,-55), S( 56,-124)},
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
