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
    S( 0, 0), S( 0, 0), S( 0, 0), S(  0,  0), S(  0,  0), S( 0, 0), S( 0, 0), S( 0, 0),
    S(20,20), S(20,20), S(20,20), S( 30, 30), S( 30, 30), S(20,20), S(20,20), S(20,20),
    S(10,10), S(10,10), S(10,10), S( 20, 20), S( 20, 20), S(10,10), S(10,10), S(10,10),
    S( 5, 5), S( 5, 5), S( 5, 5), S( 10, 10), S( 10, 10), S( 5, 5), S( 5, 5), S( 5, 5),
    S( 0, 0), S( 0, 0), S(10,10), S( 20, 20), S( 20, 20), S(10,10), S( 0, 0), S( 0, 0),
    S( 5, 5), S( 5, 5), S( 0, 0), S(  5,  5), S(  5,  5), S( 0, 0), S( 5, 5), S( 5, 5),
    S(10,10), S(10,10), S( 0, 0), S(-10,-10), S(-10,-10), S( 5, 5), S(10,10), S(10,10),
    S( 0, 0), S( 0, 0), S( 0, 0), S(  0,  0), S(  0,  0), S( 0, 0), S( 0, 0), S( 0, 0)},

    { // Knight
    S(-50,-50), S(-10,-10), S(-10,-10), S( -5,-5), S( -5,-5), S(-10,-10), S(-10,-10), S(-50,-50),
    S(-25,-25), S(  0,  0), S(  5,  5), S( 10,10), S( 10,10), S(  5,  5), S(  0,  0), S(-25,-25),
    S(-10,-10), S( 10, 10), S( 10, 10), S( 20,20), S( 20,20), S( 10, 10), S( 10, 10), S(-10,-10),
    S(-10,-10), S( 10, 10), S( 15, 15), S( 20,20), S( 20,20), S( 15, 15), S( 10, 10), S(-10,-10),
    S(-10,-10), S(  5,  5), S( 10, 10), S( 20,20), S( 20,20), S( 10, 10), S(  5,  5), S(-10,-10),
    S(-10,-10), S(  5,  5), S( 10, 10), S( 10,10), S( 10,10), S( 10, 10), S(  5,  5), S(-10,-10),
    S(-25,-25), S(  0,  0), S(  0,  0), S(  5, 5), S(  5, 5), S(  0,  0), S(  0,  0), S(-25,-25),
    S(-50,-50), S(-10,-10), S(-10,-10), S( -5,-5), S( -5,-5), S(-10,-10), S(-10,-10), S(-50,-50)},

    { // Bishop
    S(0,0), S( 0, 0), S(  0,  0), S( 0, 0), S( 0, 0), S(  0,  0), S( 0, 0), S(0,0),
    S(0,0), S( 0, 0), S(  0,  0), S(10,10), S(10,10), S(  0,  0), S( 0, 0), S(0,0),
    S(0,0), S( 0, 0), S( 10, 10), S(15,15), S(15,15), S( 10, 10), S( 0, 0), S(0,0),
    S(0,0), S(10,10), S( 15, 15), S(20,20), S(20,20), S( 15, 15), S(10,10), S(0,0),
    S(0,0), S(10,10), S( 15, 15), S(20,20), S(20,20), S( 15, 15), S(10,10), S(0,0),
    S(0,0), S( 0, 0), S( 10, 10), S(15,15), S(15,15), S( 10, 10), S( 0, 0), S(0,0),
    S(0,0), S(10,10), S(  0,  0), S(10,10), S(10,10), S(  0,  0), S(10,10), S(0,0),
    S(0,0), S( 0, 0), S(-10,-10), S( 0, 0), S( 0, 0), S(-10,-10), S( 0, 0), S(0,0)},

    { // Rook
    S( 0, 0), S( 0, 0), S( 5, 5), S(10,10), S(10,10), S( 5, 5), S( 0, 0), S( 0, 0),
    S(25,25), S(25,25), S(25,25), S(25,25), S(25,25), S(25,25), S(25,25), S(25,25),
    S( 0, 0), S( 0, 0), S( 5, 5), S(10,10), S(10,10), S( 5, 5), S( 0, 0), S( 0, 0),
    S( 0, 0), S( 0, 0), S( 5, 5), S(10,10), S(10,10), S( 5, 5), S( 0, 0), S( 0, 0),
    S( 0, 0), S( 0, 0), S( 5, 5), S(10,10), S(10,10), S( 5, 5), S( 0, 0), S( 0, 0),
    S( 0, 0), S( 0, 0), S( 5, 5), S(10,10), S(10,10), S( 5, 5), S( 0, 0), S( 0, 0),
    S( 0, 0), S( 0, 0), S( 5, 5), S(10,10), S(10,10), S( 5, 5), S( 0, 0), S( 0, 0),
    S( 0, 0), S( 0, 0), S( 5, 5), S(10,10), S(10,10), S( 5, 5), S( 0, 0), S( 0, 0)},

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
    S(-70,-48), S(-72,-28), S(-73, -8), S(-71, -2), S(-72, -4), S(-70,-11), S(-71,-25), S(-68,-50),
    S(-51,-23), S(-49,-10), S(-52,  7), S(-50,  8), S(-53, 11), S(-51, 12), S(-48,-11), S(-51,-28),
    S(-33,-11), S(-27, 12), S(-32, 21), S(-24, 20), S(-32, 20), S(-31, 23), S(-30,  5), S(-31,-12),
    S(-14,  2), S(-14, 22), S(-17, 17), S(-19, 31), S(-22, 29), S(-14, 21), S(-16, 19), S(-13,  2),
    S( 11, -1), S(  1, 19), S(  2, 17), S(-14, 33), S(-14, 31), S( -2, 19), S(  7, 16), S( 10, -3),
    S( 17,-11), S( 21,  6), S(  5, 18), S( -9, 23), S(-10, 22), S(  0, 18), S( 13, 11), S( 27,-12),
    S( 39,-23), S( 41, -9), S( 10,  9), S( -7,  9), S( -4, 10), S( 11, 11), S( 43, -7), S( 39,-27),
    S( 37,-53), S( 54,-27), S( 23, -9), S( -5,  0), S(  6,  3), S(  8,-11), S( 52,-23), S( 36,-50)}
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