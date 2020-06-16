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


tuneable_const int PieceTypeValue[7] = { 0,
    S(P_MG, P_EG),
    S(N_MG, N_EG),
    S(B_MG, B_EG),
    S(R_MG, R_EG),
    S(Q_MG, Q_EG)
};

tuneable_const int PieceValue[2][PIECE_NB] = {
    { 0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0,
      0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0 },
    { 0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0,
      0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0 }
};

// Bonus for being the side to move
tuneable_const int Tempo = 20;

// Misc bonuses and maluses
tuneable_static_const int PawnDoubled    = S( -6,-32);
tuneable_static_const int PawnIsolated   = S(-28,-16);
tuneable_static_const int PawnSupport    = S(  5,  5);
tuneable_static_const int BishopPair     = S( 52, 72);
tuneable_static_const int KingLineDanger = S(-12,  4);

// Passed pawn [rank]
tuneable_static_const int PawnPassed[8] = { 0, S(-10, 1), S(-18, 10), S( -2, 33), S( 28, 76), S( 77,116), S(121,153), 0 };

// (Semi) open file for rook and queen [pt-4]
tuneable_static_const int OpenFile[2] =     { S(51, 20), S(-13, 16) };
tuneable_static_const int SemiOpenFile[2] = { S(17, 21), S(  8, 10) };

// Mobility [pt-2][mobility]
tuneable_static_const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-89,-52), S(-48,-43), S( -3,-15), S(  9, -1), S( 26, 10), S( 34, 34), S( 47, 39), S( 63, 38), S( 66, 26) },
    // Bishop (0-13)
    { S(-61,-60), S(-37,-43), S( -9,-14), S(  1, -8), S( 13,  3), S( 30, 17), S( 36, 34),
      S( 41, 32), S( 40, 39), S( 42, 43), S( 45, 35), S( 55, 37), S( 62, 90), S( 45, 66) },
    // Rook (0-14)
    { S(-59,-69), S(-44,-29), S(-10,-13), S( -5,-13), S( -1,  2), S(  2, 14), S(  9, 26),
      S( 15, 30), S( 23, 36), S( 31, 45), S( 49, 42), S( 51, 48), S( 54, 48), S( 55, 49), S( 60, 61) },
    // Queen (0-27)
    { S(-62,-48), S(-63,-35), S(-50,-44), S(-30,-43), S(-62,-40), S(-40,-37), S(-25,-30),
      S(-14,-23), S( -5,  7), S(  6, -2), S(  8, 22), S( 19, 10), S( 20, 26), S( 23, 32), S( 34, 36),
      S( 34, 50), S( 31, 51), S( 49, 45), S( 37, 63), S( 41, 63), S( 60, 92), S( 76, 83),
      S( 76, 83), S( 92, 77), S(114, 98), S(116, 89), S(104,111), S(108,131) }
};


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
            return pos->nonPawnCount[WHITE] <= 2
                && pos->nonPawnCount[BLACK] <= 2;

        // No knights
        } else if (!pieceBB(KNIGHT)) {
            // Draw unless one side has 2 extra bishops (all nonpawns are bishops)
            return abs(  pos->nonPawnCount[WHITE]
                       - pos->nonPawnCount[BLACK]) < 2;

        // Draw with 1-2 knights vs 1 bishop (there is at least 1 bishop, and at last 1 knight)
        } else if (Single(pieceBB(BISHOP))) {
            Color bishopOwner = colorPieceBB(WHITE, BISHOP) ? WHITE : BLACK;
            return pos->nonPawnCount[ bishopOwner] == 1
                && pos->nonPawnCount[!bishopOwner] <= 2;
        }
    // Draw with 1 rook + up to 1 minor each
    } else if (Single(colorPieceBB(WHITE, ROOK)) && Single(colorPieceBB(BLACK, ROOK))) {
        return pos->nonPawnCount[WHITE] <= 2
            && pos->nonPawnCount[BLACK] <= 2;

    // Draw with 1 rook vs 1-2 minors
    } else if (Single(pieceBB(ROOK))) {
        Color rookOwner = colorPieceBB(WHITE, ROOK) ? WHITE : BLACK;
        return pos->nonPawnCount[ rookOwner] == 1
            && pos->nonPawnCount[!rookOwner] >= 1
            && pos->nonPawnCount[!rookOwner] <= 2;
    }

    return false;
}
#endif

// Evaluates pawns
INLINE int EvalPawns(const Position *pos, const Color color) {

    int eval = 0;

    Bitboard pawns = colorPieceBB(color, PAWN);

    // Doubled pawns
    eval += PawnDoubled * PopCount(pawns & ShiftBB(NORTH, pawns));

    eval += PawnSupport * PopCount(pawns & PawnBBAttackBB(pawns, color));

    while (pawns) {
        Square sq = PopLsb(&pawns);

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
    assert(abs(eval) < TBWIN_IN_MAX);

    // Return the evaluation, negated if we are black
    return (sideToMove == WHITE ? eval : -eval) + Tempo;
}
