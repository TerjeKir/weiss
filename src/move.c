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

    // Make sure the piece at 'from' can move to 'to' (ignoring pins/moving into check)
    switch (PieceTypeOf(pieceOn(from))) {
        case PAWN:
            return (moveIsEnPas(move))   ? to == pos->epSquare
                 : (moveIsPStart(move))  ? pieceOn(to ^ 8) == EMPTY
                 : (moveIsCapture(move)) ? SquareBB[to] & PawnAttackBB(color, from)
                                         : (to + 8 - 16 * color) == from;
        case KING:
            if (moveIsCastle(move))
                switch (to) {
                    case C1: return CastlePseudoLegal(pos, bitB1C1D1, WHITE_OOO, E1, D1, WHITE);
                    case G1: return CastlePseudoLegal(pos, bitF1G1,   WHITE_OO,  E1, F1, WHITE);
                    case C8: return CastlePseudoLegal(pos, bitB8C8D8, BLACK_OOO, E8, D8, BLACK);
                    case G8: return CastlePseudoLegal(pos, bitF8G8,   BLACK_OO,  E8, F8, BLACK);
                    default: assert(0); return false;
                }
            // fall through
        default:
            return SquareBB[to] & AttackBB(PieceTypeOf(pieceOn(from)), from, pieceBB(ALL));
    }
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

    assert(CheckBoard(pos));

    // Translate coordinates into square numbers
    Square from = (str[0] - 'a') + (8 * (str[1] - '1'));
    Square to   = (str[2] - 'a') + (8 * (str[3] - '1'));

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

#ifdef DEV
// Translates a string from an .epd file to a move
// Nf3-g1+, f3-g1+, Nf3-g1, f3-g1N+
Move ParseEPDMove(const char *ptrChar, const Position *pos) {

    assert(CheckBoard(pos));

    // Skip piece indicator as it's not needed
    if ('B' <= ptrChar[0] && ptrChar[0] <= 'R')
        ptrChar++;

    // Make sure coordinates make sense
    if (   (ptrChar[1] > '8' || ptrChar[1] < '1')
        || (ptrChar[4] > '8' || ptrChar[4] < '1')
        || (ptrChar[0] > 'h' || ptrChar[0] < 'a')
        || (ptrChar[3] > 'h' || ptrChar[3] < 'a')) {

        PrintBoard(pos);
        printf("\n\nBad move: %s\n\n", ptrChar);
        free(TT.mem);
        exit(EXIT_FAILURE);
    }

    // Translate coordinates into square numbers
    Square from = (ptrChar[0] - 'a') + (8 * (ptrChar[1] - '1'));
    Square to   = (ptrChar[3] - 'a') + (8 * (ptrChar[4] - '1'));

    // Type of promotion if any
    PieceType promoType = ptrChar[5] == 'Q' ? QUEEN
                        : ptrChar[5] == 'N' ? KNIGHT
                        : ptrChar[5] == 'R' ? ROOK
                        : ptrChar[5] == 'B' ? BISHOP
                                            : NO_TYPE;

    // Make sure the move is legal in the current position
    // by checking that it is generated by movegen
    MoveList list[1];
    GenAllMoves(pos, list);

    for (unsigned moveNum = 0; moveNum < list->count; ++moveNum) {

        Move move = list->moves[moveNum].move;

        if ( fromSq(move) == from
            && toSq(move) == to
            && promoType == PieceTypeOf(promotion(move)))

            return move;
    }
    return NOMOVE;
}
#endif