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

#include <stdlib.h>

#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "psqt.h"


// Eval bit masks
static Bitboard PassedMask[2][64];
static Bitboard IsolatedMask[64];

tuneable_const int PieceValue[2][PIECE_NB] = {
    { 0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0,
      0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0 },
    { 0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0,
      0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0 }
};

// Misc bonuses and maluses
tuneable_static_const int PawnIsolated   = S(-29,-13);
tuneable_static_const int BishopPair     = S( 62, 68);
tuneable_static_const int KingLineDanger = S(-12,  5);

// Passed pawn [rank]
tuneable_static_const int PawnPassed[8] = { 0, S(-13, 4), S(-11, 3), S(11, 23), S(43, 62), S(77, 99), S(127, 135), 0 };

// (Semi) open file for rook and queen [pt-4]
tuneable_static_const int OpenFile[2] =     { S(46, 14), S(-4, 14) };
tuneable_static_const int SemiOpenFile[2] = { S(16, 21), S(14, 12) };

// Mobility [pt-2][mobility]
tuneable_static_const int Mobility[5][15] = {
    // Knight (0-8)
    { S(-95,-46), S(-51,-47), S( -4,-20), S(  2, -7), S( 25,  1), S( 31, 31), S( 42, 35), S( 53, 30), S( 65, 31) },
    // Bishop (0-13)
    { S(-55,-56), S(-41,-41), S( -2, -8), S(  0,-19), S( 10,  4), S( 21, 13), S( 32, 25),
      S( 29, 32), S( 41, 36), S( 42, 33), S( 43, 22), S( 54, 38), S( 61, 85), S( 42, 66) },
    // Rook (0-14)
    { S(-62,-72), S(-42,-35), S(-14,-18), S( -6,-21), S(  3,-10), S( -4, 10), S( 11, 20),
      S(  8, 27), S( 23, 31), S( 33, 41), S( 50, 33), S( 50, 38), S( 56, 46), S( 51, 46), S( 62, 69) },
    // Queen (0-27) (accessed from [QUEEN-2], and overflows into [QUEEN-1])
    { S(-69,-46), S(-61,-43), S(-47,-39), S(-25,-49), S(-59,-44), S(-33,-35), S(-28,-26),
      S(-15,-25), S( -6, -3), S(  6, -7), S(  3, 25), S( 11, 12), S( 20, 23), S( 24, 28), S( 33, 31) },
    { S( 25, 55), S( 28, 52), S( 56, 44), S( 40, 67), S( 41, 60), S( 60, 81), S( 79, 86),
      S( 84, 79), S( 88, 78), S(111, 96), S(105, 85), S(101,105), S(108,126) }
};


// Initialize evaluation bit masks
CONSTR InitEvalMasks() {

    // For each square a pawn can be on
    for (Square sq = A2; sq <= H7; ++sq) {

        // In front
        for (Square tsq = sq + 8; tsq <= H8; tsq += 8)
            PassedMask[WHITE][sq] |= (1ULL << tsq);

        for (Square tsq = sq - 8; tsq <= H8; tsq -= 8)
            PassedMask[BLACK][sq] |= (1ULL << tsq);

        // Left side
        if (FileOf(sq) > FILE_A) {

            IsolatedMask[sq] |= FileBB[FileOf(sq) - 1];

            for (Square tsq = sq + 7; tsq <= H8; tsq += 8)
                PassedMask[WHITE][sq] |= (1ULL << tsq);

            for (Square tsq = sq - 9; tsq <= H8; tsq -= 8)
                PassedMask[BLACK][sq] |= (1ULL << tsq);
        }

        // Right side
        if (FileOf(sq) < FILE_H) {

            IsolatedMask[sq] |= FileBB[FileOf(sq) + 1];

            for (Square tsq = sq + 9; tsq <= H8; tsq += 8)
                PassedMask[WHITE][sq] |= (1ULL << tsq);

            for (Square tsq = sq - 7; tsq <= H8; tsq -= 8)
                PassedMask[BLACK][sq] |= (1ULL << tsq);
        }
    }
}

#ifdef CHECK_MAT_DRAW
// Check if the board is (likely) drawn, logic from sjeng
static bool MaterialDraw(const Position *pos) {

    // No draw with pawns or queens
    if (pieceBB(PAWN) || pieceBB(QUEEN))
        return false;

    // No rooks
    if (!pieceBB(ROOK)) {

        // No bishops
        if (!pieceBB(BISHOP)) {
            // Draw with 0-2 knights each (both 0 => KvK) (all nonpawns if any are knights)
            return pos->nonPawnCount[WHITE] <= 2 && pos->nonPawnCount[BLACK] <= 2;

        // No knights
        } else if (!pieceBB(KNIGHT)) {
            // Draw unless one side has 2 extra bishops (all nonpawns are bishops)
            return abs(pos->nonPawnCount[WHITE] - pos->nonPawnCount[BLACK]) < 2;

        // Draw with 1-2 knights vs 1 bishop (there is at least 1 bishop, and at last 1 knight)
        } else if (Single(pieceBB(BISHOP))) {
            Color bishopOwner = colorPieceBB(WHITE, BISHOP) ? WHITE : BLACK;
            return pos->nonPawnCount[bishopOwner] == 1 && pos->nonPawnCount[!bishopOwner] <= 2;
        }
    // Draw with 1 rook + up to 1 minor each
    } else if (Single(colorPieceBB(WHITE, ROOK)) && Single(colorPieceBB(BLACK, ROOK))) {
        return pos->nonPawnCount[WHITE] <= 2 && pos->nonPawnCount[BLACK] <= 2;

    // Draw with 1 rook vs 1-2 minors
    } else if (Single(pieceBB(ROOK))) {
        Color rookOwner = colorPieceBB(WHITE, ROOK) ? WHITE : BLACK;
        return pos->nonPawnCount[rookOwner] == 1 && pos->nonPawnCount[!rookOwner] >= 1 && pos->nonPawnCount[!rookOwner] <= 2;
    }

    return false;
}
#endif

// Evaluates pawns
INLINE int EvalPawns(const Position *pos, const Color color) {

    int eval = 0;

    Bitboard pieces = colorPieceBB(color, PAWN);
    while (pieces) {
        Square sq = PopLsb(&pieces);

        // Isolation penalty
        if (!(IsolatedMask[sq] & colorPieceBB(color, PAWN)))
            eval += PawnIsolated;
        // Passed bonus
        if (!((PassedMask[color][sq]) & colorPieceBB(!color, PAWN)))
            eval += PawnPassed[RelativeRank(color, RankOf(sq))];
    }

    return eval;
}

// Evaluates knights, bishops, rooks, or queens
INLINE int EvalPiece(const Position *pos, const EvalInfo *ei, const Color color, const PieceType pt) {

    int eval = 0;

    Bitboard pieces = colorPieceBB(color, pt);

    // Bishop pair
    if (pt == BISHOP && Multiple(pieces))
        eval += BishopPair;

    while (pieces) {
        Square sq = PopLsb(&pieces);

        // Mobility
        eval += Mobility[pt-2][PopCount(AttackBB(pt, sq, pieceBB(ALL)) & ei->mobilityArea[color])];

        // if (pt == KNIGHT) {}
        // if (pt == BISHOP) {}
        // if (pt == ROOK) {}
        // if (pt == QUEEN) {}

        if (pt == ROOK || pt == QUEEN) {

            // Open/Semi-open file bonus
            if (!(pieceBB(PAWN) & FileBB[FileOf(sq)]))
                eval += OpenFile[pt-4];
            else if (!(colorPieceBB(color, PAWN) & FileBB[FileOf(sq)]))
                eval += SemiOpenFile[pt-4];
        }
    }

    return eval;
}

// Evaluates kings
INLINE int EvalKings(const Position *pos, const Color color) {

    int eval = 0;

    Square kingSq = Lsb(colorPieceBB(color, KING));

    // King safety
    eval += KingLineDanger * PopCount(AttackBB(QUEEN, kingSq, colorBB(color) | pieceBB(PAWN)));

    return eval;
}

INLINE int EvalPieces(const Position *pos, const EvalInfo *ei) {

    return  EvalPawns(pos, WHITE)
          - EvalPawns(pos, BLACK)
          + EvalPiece(pos, ei, WHITE, KNIGHT)
          - EvalPiece(pos, ei, BLACK, KNIGHT)
          + EvalPiece(pos, ei, WHITE, BISHOP)
          - EvalPiece(pos, ei, BLACK, BISHOP)
          + EvalPiece(pos, ei, WHITE, ROOK)
          - EvalPiece(pos, ei, BLACK, ROOK)
          + EvalPiece(pos, ei, WHITE, QUEEN)
          - EvalPiece(pos, ei, BLACK, QUEEN)
          + EvalKings(pos, WHITE)
          - EvalKings(pos, BLACK);
}

// Initializes the eval info struct
INLINE void InitEvalInfo(const Position *pos, EvalInfo *ei, const Color color) {

    const Direction down = (color == WHITE ? SOUTH : NORTH);

    Bitboard b = RankBB[RelativeRank(color, RANK_2)] | ShiftBB(down, pieceBB(ALL));

    b &= colorPieceBB(color, PAWN);

    // Mobility area is defined as any square not attacked by an enemy pawn,
    // nor occupied by our own pawn on its starting square or blocked from advancing.
    ei->mobilityArea[color] = ~(b | PawnBBAttackBB(colorPieceBB(!color, PAWN), !color));
}

// Calculate a static evaluation of a position
int EvalPosition(const Position *pos) {

#ifdef CHECK_MAT_DRAW
    if (MaterialDraw(pos)) return 0;
#endif

    EvalInfo ei;

    InitEvalInfo(pos, &ei, WHITE);
    InitEvalInfo(pos, &ei, BLACK);

    // Material
    int eval = pos->material;

    // Evaluate pieces
    eval += EvalPieces(pos, &ei);

    // Adjust score by phase
    eval = ((MgScore(eval) * pos->phase)
         +  (EgScore(eval) * (256 - pos->phase)))
         / 256;

    // Static evaluation shouldn't spill into TB- or mate-scores
    assert(-SLOWEST_TB_WIN < eval && eval < SLOWEST_TB_WIN);

    // Return the evaluation, negated if we are black
    return sideToMove == WHITE ? eval : -eval;
}