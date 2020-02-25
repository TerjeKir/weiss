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

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "psqt.h"
#include "validate.h"


// Eval bit masks
static Bitboard PassedMask[2][64];
static Bitboard IsolatedMask[64];

// Various bonuses and maluses
static const int PawnIsolated          = S(-20,-18);
static const int BishopPair            = S( 65, 65);
static const int KingLineVulnerability = S(-10,  0);

// Passed pawn [rank]
static const int PawnPassed[8] = { 0, S(6, 6), S(13, 13), S(25, 25), S(45, 45), S(75, 75), S(130, 130), 0 };

// (Semi) open file for rook and queen [pt-4]
static const int OpenFile[2] =     { S(25, 13), S(13, 20) };
static const int SemiOpenFile[2] = { S(13, 20), S(10,  6) };

// Mobility [pt-2][mobility]
static const int Mobility[5][15] = {
    // Knight (0-8)
    { S(-65,-65), S(-32,-32), S(-20,-20), S(  0,  0), S( 20, 20), S( 32, 32), S( 45, 45), S( 50, 50), S( 65, 65) },
    // Bishop (0-13)
    { S(-65,-65), S(-45,-45), S(-32,-32), S(-13,-13), S(  0,  0), S( 13, 13), S( 20, 20),
      S( 25, 25), S( 32, 32), S( 38, 38), S( 45, 45), S( 50, 50), S( 57, 57), S( 65, 65) },
    // Rook (0-14)
    { S(-65,-65), S(-45,-45), S(-32,-32), S(-13,-13), S(  0,  0), S( 13, 13), S( 20, 20),
      S( 25, 25), S( 32, 32), S( 38, 38), S( 45, 45), S( 50, 50), S( 57, 57), S( 65, 65), S( 70, 70) },
    // Queen (0-27) (accessed from [QUEEN-2], and overflows into [QUEEN-1])
    { S(-65,-65), S(-57,-57), S(-50,-50), S(-45,-45), S(-38,-38), S(-32,-32), S(-25,-25),
      S(-20,-20), S(-13,-13), S( -6, -6), S(  0,  0), S(  6,  6), S( 13, 13), S( 20, 20), S( 25, 25) },
    { S( 32, 32), S( 38, 38), S( 45, 45), S( 50, 50), S( 57, 57), S( 65, 65), S( 70, 70),
      S( 75, 75), S( 83, 83), S( 90, 90), S( 95, 95), S(100,100), S(105,105) }
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

    assert(CheckBoard(pos));

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
    if (pt == BISHOP && PopCount(pieces) >= 2)
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
    eval += KingLineVulnerability * PopCount(AttackBB(QUEEN, kingSq, colorBB(color) | pieceBB(PAWN)));

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

    assert(-INFINITE < eval && eval < INFINITE);

    // Return the evaluation, negated if we are black
    return sideToMove() == WHITE ? eval : -eval;
}