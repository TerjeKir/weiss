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

#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "makemove.h"
#include "move.h"
#include "validate.h"


enum { QUIET, NOISY };

static int MvvLvaScores[PIECE_NB][PIECE_NB];


// Initializes the MostValuableVictim-LeastValuableAttacker scores used for ordering captures
CONSTR InitMvvLva() {

    const int VictimScore[PIECE_NB]   = {0, 106, 206, 306, 406, 506, 606, 0, 0, 106, 206, 306, 406, 506, 606, 0};
    const int AttackerScore[PIECE_NB] = {0,   1,   2,   3,   4,   5,   6, 0, 0,   1,   2,   3,   4,   5,   6, 0};

    for (int Attacker = PIECE_MIN; Attacker < PIECE_NB; ++Attacker)
        for (int Victim = PIECE_MIN; Victim < PIECE_NB; ++Victim)
            MvvLvaScores[Victim][Attacker] = VictimScore[Victim] - AttackerScore[Attacker];
}

// Constructs and adds a move to the move list
INLINE void AddMove(const Position *pos, MoveList *list, const Square from, const Square to, const int promo, const int flag, const int type) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));

    int *moveScore = &list->moves[list->count].score;

    const Move move = MOVE(from, to, pieceOn(to), promo, flag);

    // Add scores to help move ordering based on search history heuristics / mvvlva
    if (type == NOISY)
        *moveScore = MvvLvaScores[pieceOn(to)][pieceOn(from)];

    if (type == QUIET) {
        if (killer1 == move)
            *moveScore = 900000;
        else if (killer2 == move)
            *moveScore = 800000;
        else
            *moveScore = pos->searchHistory[pieceOn(from)][to];
    }

    list->moves[list->count++].move = move;
}

// Adds promotions and en passant pawn moves
INLINE void AddSpecialPawn(const Position *pos, MoveList *list, const Square from, const Square to, const int color, const int flag, const int type) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));

    if (flag == FLAG_ENPAS) {
        list->moves[list->count].move = MOVE(from, to, EMPTY, EMPTY, FLAG_ENPAS);
        list->moves[list->count++].score = 105;
    }
    if (!flag) {
        if (type == NOISY)
            AddMove(pos, list, from, to, MakePiece(color, QUEEN), FLAG_NONE, NOISY);
        if (type == QUIET) {
            AddMove(pos, list, from, to, MakePiece(color, KNIGHT), FLAG_NONE, NOISY);
            AddMove(pos, list, from, to, MakePiece(color, ROOK  ), FLAG_NONE, NOISY);
            AddMove(pos, list, from, to, MakePiece(color, BISHOP), FLAG_NONE, NOISY);
        }
    }
}

// Castling is now a bit less of a mess
INLINE void GenCastling(const Position *pos, MoveList *list, const int color, const int type) {

    if (type != QUIET) return;

    const int KCA = color            == WHITE ? WKCA      : BKCA;
    const int QCA = color            == WHITE ? WQCA      : BQCA;
    const Square from = color        == WHITE ? E1        : E8;
    const Bitboard betweenKS = color == WHITE ? bitF1G1   : bitF8G8;
    const Bitboard betweenQS = color == WHITE ? bitB1C1D1 : bitB8C8D8;

    // King side castle
    if (CastlePseudoLegal(pos, betweenKS, KCA, from, from+1, color))
        AddMove(pos, list, from, from+2, EMPTY, FLAG_CASTLE, QUIET);

    // Queen side castle
    if (CastlePseudoLegal(pos, betweenQS, QCA, from, from-1, color))
        AddMove(pos, list, from, from-2, EMPTY, FLAG_CASTLE, QUIET);
}

// Pawns are a mess
INLINE void GenPawn(const Position *pos, MoveList *list, const int color, const int type) {

    const int up    = (color == WHITE ? NORTH : SOUTH);
    const int left  = (color == WHITE ? WEST  : EAST);
    const int right = (color == WHITE ? EAST  : WEST);

    const Bitboard empty   = ~pieceBB(ALL);
    const Bitboard enemies =  colorBB(!color);
    const Bitboard pawns   =  colorPieceBB(color, PAWN);

    Bitboard on7th  = pawns & RankBB[RelativeRank(color, RANK_7)];
    Bitboard not7th = pawns ^ on7th;

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
            AddSpecialPawn(pos, list, to - (up+left), to, color, FLAG_NONE, type);
        }
        while (rPromoCap) {
            Square to = PopLsb(&rPromoCap);
            AddSpecialPawn(pos, list, to - (up+right), to, color, FLAG_NONE, type);
        }
        // Promotions
        while (promotions) {
            Square to = PopLsb(&promotions);
            AddSpecialPawn(pos, list, to - up, to, color, FLAG_NONE, type);
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
        if (pos->enPas != NO_SQ) {
            Bitboard enPassers = not7th & PawnAttackBB(!color, pos->enPas);
            while (enPassers)
                AddSpecialPawn(pos, list, PopLsb(&enPassers), pos->enPas, color, FLAG_ENPAS, NOISY);
        }
    }
}

// Knight, bishop, rook, queen and king except castling
INLINE void GenPieceType(const Position *pos, MoveList *list, const int color, const int type, const int pt) {

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
static void GenMoves(const Position *pos, MoveList *list, const int color, const int type) {

    assert(CheckBoard(pos));

    GenCastling (pos, list, color, type);
    GenPawn     (pos, list, color, type);
    GenPieceType(pos, list, color, type, KNIGHT);
    GenPieceType(pos, list, color, type, ROOK);
    GenPieceType(pos, list, color, type, BISHOP);
    GenPieceType(pos, list, color, type, QUEEN);
    GenPieceType(pos, list, color, type, KING);

    assert(MoveListOk(list, pos));
}

// Generate all quiet moves
void GenQuietMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, sideToMove(), QUIET);
}

// Generate all noisy moves
void GenNoisyMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, sideToMove(), NOISY);
}

// Generate all pseudo legal moves
void GenAllMoves(const Position *pos, MoveList *list) {

    list->count = list->next = 0;

    GenNoisyMoves(pos, list);
    GenQuietMoves(pos, list);
}