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
    S( 41, 57), S( 29, 43), S( 30, 32), S( 29, 25), S( 36,  7), S( 13,  8), S( 18, 20), S( 14, 28),
    S( 11, 65), S( 19, 46), S( 23, 27), S( 12, -1), S( 20, -1), S( 49, 18), S( 31, 20), S( 25, 29),
    S( -7, 35), S(-13, 25), S(-10, 10), S( 12,-22), S( 26,-15), S( 38, -9), S(  9, 10), S(  4, 10),
    S(-12, 14), S(-21, 13), S( -5,-10), S( -3,-15), S(  7,-12), S( 11,-11), S(  5, -2), S( -3, -6),
    S(-15,  3), S(-27,  0), S(-16, -3), S(-10, -7), S(  1,  0), S(  5,  1), S( 17,-11), S(  6,-13),
    S( -6, 10), S(-17,  6), S(-11,  4), S(  8, -8), S( -3, 14), S( 35,  9), S( 42, -8), S( 16,-24),
             0,          0,          0,          0,          0,          0,          0,          0},

    { // Knight
    S(-46,-61), S( -8,-10), S(-14,-10), S( -2, -9), S(  1, -3), S(-10, -9), S(-16,-17), S(-55,-45),
    S(-25,-21), S( -8,  4), S( 13, -4), S( 20, 16), S(  9, 20), S( 13,  8), S(  5, -9), S(-27,-19),
    S(-20,-14), S(  3,  8), S( 22, 19), S( 43, 22), S( 47, 28), S( 33, 20), S( 25, 13), S(  0, -7),
    S(  0, -9), S(  6,  8), S( 28, 27), S( 45, 39), S( 28, 45), S( 48, 36), S( 28, 21), S( 23, 10),
    S( -4, -5), S(  7,  6), S(  7, 23), S( 10, 33), S( 19, 34), S( 16, 27), S( 37, 13), S( 25,  3),
    S(-34,-21), S(-19,-14), S(-19, -9), S(  2,  9), S(  6,  6), S(-10,-15), S( -4, -9), S(-16,-20),
    S(-30,-19), S(-10,-13), S(-26,-23), S(-11,-15), S(-18,-11), S(-20,-20), S(-20, -2), S( -9,-14),
    S(-54,-59), S(-26,-25), S(-25,-20), S(-10,-10), S(-15, -8), S(-20,-24), S(-21,-16), S(-47,-42)},

    { // Bishop
    S(  1,  2), S(  3, -6), S( -2,  7), S(  0, -2), S(  0,  2), S( -6, -2), S( -5,  1), S(  0, -1),
    S(-11, -5), S(  1,  4), S( -3,  4), S(  6,  8), S( 12, 14), S(  0, -1), S( -1,  4), S(  3,  0),
    S( -2,  4), S(  7, 11), S( 16, 18), S( 20, 15), S( 21, 18), S( 27, 26), S( 15, 11), S( 18,  9),
    S( -9, -2), S( 14, 16), S( 15, 14), S( 48, 23), S( 31, 25), S( 29, 15), S( 16, 19), S(  2,  7),
    S( -8,  5), S(  4,  9), S(  5, 21), S( 24, 19), S( 28, 20), S( -6, 17), S(  1, 10), S( 11, -2),
    S( -5,  0), S( 10,  8), S(  3, 14), S(  7, 16), S(  5, 16), S(  8,  7), S( 12, -4), S( 15,  4),
    S(  8, -8), S(  5, -9), S( 11, -1), S(-10,  1), S(-10,  1), S( -3,-10), S( 19, -7), S(  5, -9),
    S(  8,  1), S( 10,  3), S( -8,-13), S(-16, -9), S(-19, -4), S(-11, -7), S( -3,  0), S(  2, -3)},

    { // Rook
    S( 20, 35), S( 14, 33), S( 20, 34), S( 13, 31), S( 26, 23), S(  8, 25), S( 13, 23), S(  6, 30),
    S( 21, 32), S( 15, 36), S( 32, 38), S( 36, 46), S( 31, 45), S( 25, 36), S( 31, 30), S( 33, 28),
    S(  3, 24), S( 18, 27), S( 19, 29), S( 30, 29), S( 35, 22), S( 30, 27), S( 20, 17), S( 22, 11),
    S( -2, 16), S(  9, 16), S( 17, 22), S( 25, 24), S( 24, 20), S( 23, 19), S( 13, 11), S( 12, 17),
    S(-19,  5), S( -4,  7), S( -9, 10), S( -1,  6), S( -7,  6), S(  4,  7), S(  5, 11), S(  9, -4),
    S(-28,-15), S(-22, -1), S(-21,-10), S(-15,-15), S(-17,-13), S(-14, -4), S( 13, -3), S( -9,-11),
    S(-24,-19), S(-22,-14), S(-16,-19), S(-16,-19), S(-15,-21), S( -6,-20), S( -5, -7), S(-17, -3),
    S(-20,-17), S(-12,-16), S( -8,-14), S(  1,-23), S( -3,-23), S( -6, -7), S(  6,-10), S(-19,-22)},

    { // Queen
    S( -8,  0), S(  3,  2), S( 16,  2), S(  6, 14), S(  5,  7), S( 12,  8), S( -4, -8), S(  9,  7),
    S(-10, -8), S(-57,  3), S(-11,  6), S( -4,  6), S(  4, 29), S( 25, 15), S(  2,  5), S( 25,  5),
    S(-14, -1), S( -5,-12), S(-10, -3), S(  5, -1), S( 27, 16), S( 33, 24), S( 44, 30), S( 57, 21),
    S(-16,-12), S(-11,  3), S( -5,-11), S( -4, 10), S( 18, 16), S( 27, 24), S( 44, 21), S( 51, 20),
    S(-15, -2), S(-10, -3), S(-11, -1), S(-12, 12), S( -4,  4), S( 12, 18), S( 20, 14), S( 33, 15),
    S(-14, -8), S( -9,-14), S(-10,  2), S(-17,-15), S(-13,-20), S(-11,  9), S( 12,-12), S(  5,  8),
    S(-11, -5), S(-16,-10), S( -6,-29), S(-10,-21), S(-12,-24), S(-19, -9), S(-18,-15), S( -6,-16),
    S(-17, -6), S(-10,-17), S(-14,-20), S( -6,-24), S(-17,-14), S(-29,-24), S( -9,-14), S(  5, -6)},

    { // King
    S(-73,-48), S(-54,-26), S(-72,  7), S(-66, 21), S(-76,  6), S(-70,-25), S(-67,-15), S(-64,-64),
    S(-62,-32), S(-33, 12), S(-56, 33), S(-63, 11), S(-37, 24), S(-54, 25), S(-38, 13), S(-54,-29),
    S(-25, -8), S(-23, 45), S(-22, 50), S( -7, 42), S(-12, 36), S( -6, 52), S(-17, 40), S(-39, -7),
    S(-16,  7), S(-11, 39), S(-24, 52), S(  3, 47), S(-20, 55), S(  1, 47), S(-18, 44), S(-25, -6),
    S( 10,-12), S( -5, 27), S(  2, 40), S(  1, 51), S( -6, 53), S( -5, 40), S(  6, 20), S(-19,-26),
    S(  4,-22), S( 30, -7), S(  5, 16), S(-18, 35), S(  6, 30), S(  9, 17), S( 17, -6), S(  3,-34),
    S( 43,-27), S( 26,-13), S( 21, -6), S(-28,  2), S(-22,  3), S( -8, -1), S( 50,-32), S( 62,-67),
    S(  6,-60), S( 84,-60), S( 43,-40), S(-62,-20), S( 15,-71), S(-53,-31), S( 59,-64), S( 57,-129)},
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
