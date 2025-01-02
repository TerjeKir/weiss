/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2023 Terje Kirstihagen

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

    // Must move our own piece
    if (ColorOf(piece(move)) != color)
        return false;

    // Castling
    if (moveIsCastle(move))
        return CastleLegal(pos, to);

    // Must move the specified piece and capture the specified piece (or not capture)
    if (piece(move) != pieceOn(from) || capturing(move) != pieceOn(to))
        return false;

    // Filter some illegal moves due to check
    if (pos->checkers && pieceTypeOn(from) != KING) {
        // Only moving the king can get out of double-check
        if (Multiple(pos->checkers))
            return false;
        // Non-king moves must block or capture the attacker
        if (!(BB(to) & (BetweenBB[kingSq(color)][Lsb(pos->checkers)] | pos->checkers))) {
            if (!moveIsEnPas(move))
                return false;

            if (!(pos->checkers & BB(pos->epSquare ^ 8)))
                return false;
        }
    }

    // Pawn moves
    if (pieceTypeOn(from) == PAWN)
        return  moveIsEnPas(move)  ? to == pos->epSquare
              : moveIsPStart(move) ? pieceOn(to ^ 8) == EMPTY
                                   : true;

    // All other moves
    return BB(to) & AttackBB(pieceTypeOn(from), from, pieceBB(ALL));
}

// Checks whether a move is legal
// (assumes the move is pseudo-legal in the current position)
bool MoveIsLegal(const Position *pos, const Move move) {

    Color color = sideToMove;
    Square from = fromSq(move);
    Square to = toSq(move);

    Square kingSq = PieceTypeOf(pieceOn(from)) == KING ? to : (Square)kingSq(color);

    Bitboard occ = (pieceBB(ALL) ^ BB(from)) | BB(to);
    Bitboard exclude = BB(to);

    if (moveIsEnPas(move))
        occ ^= BB(to ^ 8),
        exclude ^= BB(to ^ 8);

    Bitboard bishops = colorBB(!color) & (pieceBB(BISHOP) | pieceBB(QUEEN));
    Bitboard rooks   = colorBB(!color) & (pieceBB(ROOK)   | pieceBB(QUEEN));

    Bitboard kingAttackers =
        (  (PawnAttackBB(color, kingSq)   & colorPieceBB(!color, PAWN))
         | (AttackBB(KING,   kingSq, occ) & colorPieceBB(!color, KING))
         | (AttackBB(KNIGHT, kingSq, occ) & colorPieceBB(!color, KNIGHT))
         | (AttackBB(BISHOP, kingSq, occ) & bishops)
         | (AttackBB(ROOK,   kingSq, occ) & rooks));

    // If any non-excluded piece is attacking the king, the move is illegal
    return !(kingAttackers & ~exclude);
}

// Translates a move to a string
char *MoveToStr(const Move move) {

    static char moveStr[6] = "";

    SqToStr(fromSq(move), moveStr);
    SqToStr(  toSq(move), moveStr + 2);
    moveStr[4] = "\0.nbrq"[PieceTypeOf(promotion(move))];

    // Encode castling as KxR for chess 960
    if (Chess960 && moveIsCastle(move)) {
        int color = ColorOf(piece(move)) == WHITE ? WHITE_CASTLE : BLACK_CASTLE;
        int side = FileOf(toSq(move)) == FILE_G ? OO : OOO;
        SqToStr(RookSquare[color & side], moveStr + 2);
    }

    return moveStr;
}

// Translates a string to a move
Move ParseMove(const char *str, const Position *pos) {

    Square from = StrToSq(str);
    Square to   = StrToSq(str+2);

    Piece promo = str[4] == 'q' ? MakePiece(sideToMove, QUEEN)
                : str[4] == 'n' ? MakePiece(sideToMove, KNIGHT)
                : str[4] == 'r' ? MakePiece(sideToMove, ROOK)
                : str[4] == 'b' ? MakePiece(sideToMove, BISHOP)
                                : 0;

    PieceType pt = pieceTypeOn(from);

    int flag = pt == KING && Distance(from, to) > 1           ? FLAG_CASTLE
             : pt == PAWN && Distance(from, to) > 1           ? FLAG_PAWNSTART
             : pt == PAWN && str[0] != str[2] && !pieceOn(to) ? FLAG_ENPAS
                                                              : 0;

    if (Chess960 && pt == KING && pieceOn(to) == MakePiece(sideToMove, ROOK)) {
        to = RelativeSquare(sideToMove, to > from ? G1 : C1);
        flag = FLAG_CASTLE;
    }

    return MOVE(from, to, pieceOn(from), pieceOn(to), promo, flag);
}

// Checks if the move is in the list of searchmoves if any were given
bool NotInSearchMoves(Move searchmoves[], Move move) {
    if (searchmoves[0] == NOMOVE) return false;
    for (Move *m = searchmoves; *m != NOMOVE; ++m)
        if (*m == move)
            return false;
    return true;
}
