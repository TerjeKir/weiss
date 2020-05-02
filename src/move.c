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

#include <stdio.h>
#include <stdlib.h>

#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


// Checks whether a move is pseudo-legal (assuming it is pseudo-legal in some position)
bool MoveIsPseudoLegal(const Position *pos, const Move move) {

    if (!move) return false;

    const Color color = sideToMove;
    const Square from = fromSq(move);
    const Square to = toSq(move);

    // Must move our own piece to a square not occupied by our own pieces
    if (  !(colorBB(color) & SquareBB[from])
        || (colorBB(color) & SquareBB[to])
        || capturing(move) != pieceOn(to))
        return false;

    assert(ValidPiece(pieceOn(from)));

    // All moves but castling, en passant and pawn starts
    if (!moveIsSpecial(move)) {

        // Normal pawn moves
        if (PieceTypeOf(pieceOn(from)) == PAWN)
            return (moveIsCapture(move)) ? SquareBB[to] & PawnAttackBB(color, from)
                                         : (to + 8 - 16 * color) == from;

        // Normal moves by any other piece
        return SquareBB[to] & AttackBB(PieceTypeOf(pieceOn(from)), from, pieceBB(ALL));
    }

    if (moveIsCastle(move))
        switch (to) {
            case C1: return CastlePseudoLegal(pos, WHITE, OOO);
            case G1: return CastlePseudoLegal(pos, WHITE, OO);
            case C8: return CastlePseudoLegal(pos, BLACK, OOO);
            case G8: return CastlePseudoLegal(pos, BLACK, OO);
            default: assert(0); return false;
        }

    if (moveIsEnPas(move))
        return PieceTypeOf(pieceOn(from)) == PAWN
            && to == pos->epSquare
            && to & PawnAttackBB(color, from);

    if (moveIsPStart(move))
        return PieceTypeOf(pieceOn(from)) == PAWN
            && pieceOn(to ^ 8) == EMPTY
            && (to + 16 - 32 * color) == from;

    return false; // Unreachable
}

// Translates a move to a string
char *MoveToStr(const Move move) {

    static char moveStr[6];

    int ff = FileOf(fromSq(move));
    int rf = RankOf(fromSq(move));
    int ft = FileOf(toSq(move));
    int rt = RankOf(toSq(move));

    PieceType promo = PieceTypeOf(promotion(move));

    char pchar = promo == QUEEN  ? 'q'
               : promo == KNIGHT ? 'n'
               : promo == ROOK   ? 'r'
               : promo == BISHOP ? 'b'
                                 : '\0';

    sprintf(moveStr, "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);

    return moveStr;
}

// Translates a string to a move (assumes correct input)
// a2a4 b7b8q g7f8n
Move ParseMove(const char *str, const Position *pos) {

    // Translate coordinates into square numbers
    Square from = AlgebraicToSq(str[0], str[1]);
    Square to   = AlgebraicToSq(str[2], str[3]);

    Piece promo = str[4] == 'q' ? MakePiece(sideToMove, QUEEN)
                : str[4] == 'n' ? MakePiece(sideToMove, KNIGHT)
                : str[4] == 'r' ? MakePiece(sideToMove, ROOK)
                : str[4] == 'b' ? MakePiece(sideToMove, BISHOP)
                                : 0;

    PieceType pt = PieceTypeOf(pieceOn(from));

    int flag = pt == KING && Distance(from, to) > 1           ? FLAG_CASTLE
             : pt == PAWN && Distance(from, to) > 1           ? FLAG_PAWNSTART
             : pt == PAWN && str[0] != str[2] && !pieceOn(to) ? FLAG_ENPAS
                                                              : 0;

    return MOVE(from, to, pieceOn(to), promo, flag);
}