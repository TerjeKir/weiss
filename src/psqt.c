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


// Initialize the piece square tables with piece values included
CONSTR InitPSQT() {

    const int pieceValue[TYPE_NB] = {
        0, S(P_MG, P_EG), S(N_MG, N_EG), S(B_MG, B_EG), S(R_MG, R_EG), S(Q_MG, Q_EG)
    };

    // Blacks point of view - makes it easier to visualize as the board isn't upside down
    const int pieceSqValue[7][64] = {
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

        { 0 }, // Queen

        { // King
        S(-70,-50), S(-70,-25), S(-70,-10), S(-70, 0), S(-70, 0), S(-70,-10), S(-70,-25), S(-70,-50),
        S(-50,-25), S(-50,-10), S(-50, 10), S(-50,10), S(-50,10), S(-50, 10), S(-50,-10), S(-50,-25),
        S(-30,-10), S(-30, 10), S(-30, 20), S(-30,20), S(-30,20), S(-30, 20), S(-30, 10), S(-30,-10),
        S(-15,  0), S(-15, 20), S(-15, 20), S(-20,30), S(-20,30), S(-15, 20), S(-15, 20), S(-15,  0),
        S( 10,  0), S(  5, 20), S(  0, 20), S(-15,30), S(-15,30), S(  0, 20), S(  5, 20), S( 10,  0),
        S( 20,-10), S( 20, 10), S(  5, 20), S(-10,20), S(-10,20), S(  5, 20), S( 20, 10), S( 20,-10),
        S( 40,-25), S( 40,-10), S( 10, 10), S( -5,10), S( -5,10), S( 10, 10), S( 40,-10), S( 40,-25),
        S( 40,-50), S( 50,-25), S( 25,-10), S(  0, 0), S(  0, 0), S( 10,-10), S( 50,-25), S( 40,-50)}
    };

    // Black scores are negative (white double negated -> positive)
    for (PieceType pt = PAWN; pt <= KING; ++pt)
        for (Square sq = A1; sq <= H8; ++sq) {
            // Base piece value + the piece square value
            PSQT[MakePiece(BLACK, pt)][sq] = -(pieceValue[pt] + pieceSqValue[pt][sq]);
            // Same score inverted used for white on the square mirrored horizontally
            PSQT[MakePiece(WHITE, pt)][MirrorSquare(sq)] = -PSQT[pt][sq];
        }
}