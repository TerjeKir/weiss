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

#include "bitboard.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"


enum { QUIET, NOISY };

// Constructs and adds a move to the move list
INLINE void AddMove(const Position *pos, MoveList *list, const Square from, const Square to, const Piece promo, const int flag) {
    list->moves[list->count++].move = MOVE(from, to, pieceOn(from), pieceOn(to), promo, flag);
}

// Adds promotions
INLINE void AddPromotions(const Position *pos, MoveList *list, const Color color, const int type, Bitboard moves, const Direction dir) {
    while (moves) {
        Square to = PopLsb(&moves);
        Square from = to - dir;

        if (type == NOISY)
            AddMove(pos, list, from, to, MakePiece(color, QUEEN), FLAG_NONE);

        if (type == QUIET) {
            AddMove(pos, list, from, to, MakePiece(color, KNIGHT), FLAG_NONE);
            AddMove(pos, list, from, to, MakePiece(color, ROOK  ), FLAG_NONE);
            AddMove(pos, list, from, to, MakePiece(color, BISHOP), FLAG_NONE);
        }
    }
}

// Adds pawn moves aside from promos and en passant
INLINE void AddPawnMoves(const Position *pos, MoveList *list, Bitboard moves, const Direction dir, const int flag) {
    while (moves) {
        Square to = PopLsb(&moves);
        AddMove(pos, list, to - dir, to, EMPTY, flag);
    }
}

// Castling is now a bit less of a mess
INLINE void GenCastling(const Position *pos, MoveList *list, const Color color, const int type) {

    if (type != QUIET) return;

    const Square from = kingSq(color);

    // King side castle
    Square toShort = RelativeSquare(color, G1);
    if (CastleLegal(pos, toShort))
        AddMove(pos, list, from, toShort, EMPTY, FLAG_CASTLE);

    // Queen side castle
    Square toLong = RelativeSquare(color, C1);
    if (CastleLegal(pos, toLong))
        AddMove(pos, list, from, toLong, EMPTY, FLAG_CASTLE);
}

// Pawns are a mess
INLINE void GenPawn(const Position *pos, MoveList *list, const Color color, const int type) {

    const Direction up    = color == WHITE ? NORTH : SOUTH;
    const Direction left  = color == WHITE ? WEST  : EAST;
    const Direction right = color == WHITE ? EAST  : WEST;

    const Bitboard empty   = ~pieceBB(ALL);
    const Bitboard enemies = pos->checkers ?: colorBB(!color);
    const Bitboard pawns   = colorPieceBB(color, PAWN);
    const Bitboard promo   = rank8BB | rank1BB;
    const Bitboard normal  = ~promo;

    const Bitboard push = empty & ShiftBB(pawns, up);
    const Bitboard lCap = enemies & ShiftBB(pawns, up+left);
    const Bitboard rCap = enemies & ShiftBB(pawns, up+right);

    // Normal moves forward
    if (type == QUIET) {

        Bitboard moves = push & normal;
        Bitboard doubles = ShiftBB(moves, up) & empty & RankBB[RelativeRank(color, RANK_4)];

        if (pos->checkers)
            moves   &= BetweenBB[kingSq(color)][Lsb(pos->checkers)],
            doubles &= BetweenBB[kingSq(color)][Lsb(pos->checkers)];

        AddPawnMoves(pos, list, moves, up, FLAG_NONE);
        AddPawnMoves(pos, list, doubles, up * 2, FLAG_PAWNSTART);
    }

    // Promotions
    AddPromotions(pos, list, color, type, lCap & promo, up+left);
    AddPromotions(pos, list, color, type, rCap & promo, up+right);
    Bitboard pushPromos = push & promo;
    if (pos->checkers) pushPromos &= BetweenBB[kingSq(color)][Lsb(pos->checkers)];
    AddPromotions(pos, list, color, type, pushPromos, up);

    // Captures
    if (type == NOISY) {

        AddPawnMoves(pos, list, lCap & normal, up+left,  FLAG_NONE);
        AddPawnMoves(pos, list, rCap & normal, up+right, FLAG_NONE);

        // En passant
        if (pos->epSquare) {
            if (pos->checkers && !(pos->checkers & BB(pos->epSquare ^ 8)))
                return;
            Bitboard enPassers = pawns & PawnAttackBB(!color, pos->epSquare);
            while (enPassers)
                AddMove(pos, list, PopLsb(&enPassers), pos->epSquare, EMPTY, FLAG_ENPAS);
        }
    }
}

// Knight, bishop, rook, queen and king except castling
INLINE void GenPieceType(const Position *pos, MoveList *list, const Color color, const int type, const PieceType pt) {

    const bool mustDefendCheck = pos->checkers && pt != KING;
    const Bitboard occupied = pieceBB(ALL);
    const Bitboard enemies  = mustDefendCheck ? pos->checkers : colorBB(!color);
    const Bitboard empty    = mustDefendCheck ? BetweenBB[kingSq(color)][Lsb(pos->checkers)] : ~occupied;
    const Bitboard targets  = type == QUIET ? empty : enemies;

    Bitboard pieces = colorPieceBB(color, pt);

    while (pieces) {
        Square from = PopLsb(&pieces);
        Bitboard moves = targets & AttackBB(pt, from, occupied);
        while (moves)
            AddMove(pos, list, from, PopLsb(&moves), EMPTY, FLAG_NONE);
    }
}

// Generate all quiet or noisy moves for the given color
static void GenMoves(const Position *pos, MoveList *list, const Color color, const int type) {

    if (Multiple(pos->checkers))
        return GenPieceType(pos, list, color, type, KING);

    GenCastling (pos, list, color, type);
    GenPawn     (pos, list, color, type);
    GenPieceType(pos, list, color, type, KNIGHT);
    GenPieceType(pos, list, color, type, ROOK);
    GenPieceType(pos, list, color, type, BISHOP);
    GenPieceType(pos, list, color, type, QUEEN);
    GenPieceType(pos, list, color, type, KING);
}

void GenQuietMoves(const Position *pos, MoveList *list) {
    GenMoves(pos, list, sideToMove, QUIET);
}

void GenNoisyMoves(const Position *pos, MoveList *list) {
    GenMoves(pos, list, sideToMove, NOISY);
}

void GenAllMoves(const Position *pos, MoveList *list) {
    GenNoisyMoves(pos, list);
    GenQuietMoves(pos, list);
}

// Counts the number of legal moves in the position filtered by searchmoves
int LegalMoveCount(Position *pos, Move searchmoves[]) {
    int rootMoveCount = 0;

    MoveList list;
    list.count = list.next = 0;
    GenAllMoves(pos, &list);

    for (int i = 0; i < list.count; ++i) {
        Move move = list.moves[list.next++].move;
        if (NotInSearchMoves(searchmoves, move)) continue;
        if (!MakeMove(pos, move)) continue;
        ++rootMoveCount;
        TakeMove(pos);
    }
    pos->nodes = 0;

    return rootMoveCount;
}
