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

#include "bitboard.h"
#include "board.h"
#include "makemove.h"
#include "move.h"


enum { QUIET, NOISY };

static int MvvLvaScores[PIECE_NB][PIECE_NB];


// Initializes the MostValuableVictim-LeastValuableAttacker scores used for ordering captures
CONSTR InitMvvLva() {

    const int VictimScore[PIECE_NB]   = {0, 106, 206, 306, 406, 506, 606, 0, 0, 106, 206, 306, 406, 506, 606, 0};
    const int AttackerScore[PIECE_NB] = {0,   1,   2,   3,   4,   5,   6, 0, 0,   1,   2,   3,   4,   5,   6, 0};

    for (Piece Attacker = PIECE_MIN; Attacker < PIECE_NB; ++Attacker)
        for (Piece Victim = PIECE_MIN; Victim < PIECE_NB; ++Victim)
            MvvLvaScores[Victim][Attacker] = VictimScore[Victim] - AttackerScore[Attacker];
}

#ifndef NDEBUG
static bool MoveListOk(const MoveList *list, const Position *pos) {

    if (list->count >= MAXPOSITIONMOVES)
        return false;

    for (int i = 0; i < list->count; ++i)
        if (!MoveIsPseudoLegal(pos, list->moves[i].move))
            return PrintBoard(pos), false;

    return true;
}
#endif

// Constructs and adds a move to the move list
INLINE void AddMove(const Position *pos, MoveList *list, const Square from, const Square to, const Piece promo, const int flag, const int type) {

    const Move move = MOVE(from, to, pieceOn(to), promo, flag);

    // Add scores to help move ordering based on search history heuristics / mvvlva
    int *moveScore = &list->moves[list->count].score;

    if (type == NOISY)
        *moveScore = MvvLvaScores[pieceOn(to)][pieceOn(from)];

    if (type == QUIET)
        *moveScore = move == killer1 ? 900000
                   : move == killer2 ? 800000
                                     : pos->history[pieceOn(from)][to];

    list->moves[list->count++].move = move;
}

// Adds en passants
INLINE void AddEnPas(MoveList *list, const Square from, const Square to) {

    list->moves[list->count].move = MOVE(from, to, EMPTY, EMPTY, FLAG_ENPAS);
    list->moves[list->count++].score = 105;
}

// Adds promotions
INLINE void AddPromotions(const Position *pos, MoveList *list, const Square from, const Square to, const Color color, const int type) {

    if (type == NOISY)
        AddMove(pos, list, from, to, MakePiece(color, QUEEN), FLAG_NONE, NOISY);

    if (type == QUIET) {
        AddMove(pos, list, from, to, MakePiece(color, KNIGHT), FLAG_NONE, NOISY);
        AddMove(pos, list, from, to, MakePiece(color, ROOK  ), FLAG_NONE, NOISY);
        AddMove(pos, list, from, to, MakePiece(color, BISHOP), FLAG_NONE, NOISY);
    }
}

// Castling is now a bit less of a mess
INLINE void GenCastling(const Position *pos, MoveList *list, const Color color, const int type) {

    if (type != QUIET) return;

    const Square from = color == WHITE ? E1 : E8;

    // King side castle
    if (CastlePseudoLegal(pos, color, OO))
        AddMove(pos, list, from, from+2, EMPTY, FLAG_CASTLE, QUIET);

    // Queen side castle
    if (CastlePseudoLegal(pos, color, OOO))
        AddMove(pos, list, from, from-2, EMPTY, FLAG_CASTLE, QUIET);
}

// Pawns are a mess
INLINE void GenPawn(const Position *pos, MoveList *list, const Color color, const int type) {

    const Direction up    = color == WHITE ? NORTH : SOUTH;
    const Direction left  = color == WHITE ? WEST  : EAST;
    const Direction right = color == WHITE ? EAST  : WEST;

    const Bitboard empty   = ~pieceBB(ALL);
    const Bitboard enemies =  colorBB(!color);
    const Bitboard pawns   =  colorPieceBB(color, PAWN);

    const Bitboard on7th  = pawns & RankBB[RelativeRank(color, RANK_7)];
    const Bitboard not7th = pawns ^ on7th;

    // Normal moves forward
    if (type == QUIET) {

        Bitboard pawnMoves  = empty & ShiftBB(up, not7th);
        Bitboard pawnStarts = empty & ShiftBB(up, pawnMoves)
                                    & RankBB[RelativeRank(color, RANK_4)];

        // Normal pawn moves
        while (pawnMoves) {
            Square to = PopLsb(&pawnMoves);
            AddMove(pos, list, to - up, to, EMPTY, FLAG_NONE, QUIET);
        }
        // Pawn starts
        while (pawnStarts) {
            Square to = PopLsb(&pawnStarts);
            AddMove(pos, list, to - up * 2, to, EMPTY, FLAG_PAWNSTART, QUIET);
        }
    }

    // Promotions
    if (on7th) {

        Bitboard promotions = empty & ShiftBB(up, on7th);
        Bitboard lPromoCap = enemies & ShiftBB(up+left, on7th);
        Bitboard rPromoCap = enemies & ShiftBB(up+right, on7th);

        // Promoting captures
        while (lPromoCap) {
            Square to = PopLsb(&lPromoCap);
            AddPromotions(pos, list, to - (up+left), to, color, type);
        }
        while (rPromoCap) {
            Square to = PopLsb(&rPromoCap);
            AddPromotions(pos, list, to - (up+right), to, color, type);
        }
        // Promotions
        while (promotions) {
            Square to = PopLsb(&promotions);
            AddPromotions(pos, list, to - up, to, color, type);
        }
    }
    // Captures
    if (type == NOISY) {

        Bitboard lAttacks = enemies & ShiftBB(up+left, not7th);
        Bitboard rAttacks = enemies & ShiftBB(up+right, not7th);

        while (lAttacks) {
            Square to = PopLsb(&lAttacks);
            AddMove(pos, list, to - (up+left), to, EMPTY, FLAG_NONE, NOISY);
        }
        while (rAttacks) {
            Square to = PopLsb(&rAttacks);
            AddMove(pos, list, to - (up+right), to, EMPTY, FLAG_NONE, NOISY);
        }
        // En passant
        if (pos->epSquare != NO_SQ) {
            Bitboard enPassers = not7th & PawnAttackBB(!color, pos->epSquare);
            while (enPassers)
                AddEnPas(list, PopLsb(&enPassers), pos->epSquare);
        }
    }
}

// Knight, bishop, rook, queen and king except castling
INLINE void GenPieceType(const Position *pos, MoveList *list, const Color color, const int type, const PieceType pt) {

    const Bitboard occupied = pieceBB(ALL);
    const Bitboard enemies  = colorBB(!color);
    const Bitboard targets  = type == NOISY ? enemies : ~occupied;

    Bitboard pieces = colorPieceBB(color, pt);

    while (pieces) {

        Square from = PopLsb(&pieces);

        Bitboard moves = targets & AttackBB(pt, from, occupied);

        while (moves)
            AddMove(pos, list, from, PopLsb(&moves), EMPTY, 0, type);
    }
}

// Generate moves
static void GenMoves(const Position *pos, MoveList *list, const Color color, const int type) {

    GenCastling (pos, list, color, type);
    GenPawn     (pos, list, color, type);
    GenPieceType(pos, list, color, type, KNIGHT);
    GenPieceType(pos, list, color, type, ROOK);
    GenPieceType(pos, list, color, type, BISHOP);
    GenPieceType(pos, list, color, type, QUEEN);
    GenPieceType(pos, list, color, type, KING);

    assert(MoveListOk(list, pos));
}

// Generate quiet moves
void GenQuietMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, sideToMove, QUIET);
}

// Generate noisy moves
void GenNoisyMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, sideToMove, NOISY);
}