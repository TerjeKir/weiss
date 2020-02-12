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
static const int PawnPassed[8] = { 0, S(6, 6), S(13, 13), S(25, 25), S(45, 45), S(75, 75), S(130, 130), 0 };
static const int PawnIsolated = S(-20, -18);

static const int  RookOpenFile = S(25, 13);
static const int QueenOpenFile = S(13, 20);
static const int  RookSemiOpenFile = S(13, 20);
static const int QueenSemiOpenFile = S(10, 6);

static const int BishopPair = S(65, 65);

static const int KingLineVulnerability = S(-10, 0);

// Mobility
static const int KnightMobility[9] = {
    S(-65, -65), S(-32, -32), S(-20, -20), S(0, 0), S(20, 20), S(32, 32), S(45, 45), S(50, 50), S(65, 65)
};

static const int BishopMobility[14] = {
    S(-65, -65), S(-45, -45), S(-32, -32), S(-13, -13), S( 0,  0), S(13, 13), S(20, 20),
    S( 25,  25), S( 32,  32), S( 38,  38), S( 45,  45), S(50, 50), S(57, 57), S(65, 65)
};

static const int RookMobility[15] = {
    S(-65, -65), S(-45, -45), S(-32, -32), S(-13, -13), S( 0,  0), S(13, 13), S(20, 20),
    S( 25,  25), S( 32,  32), S( 38,  38), S( 45,  45), S(50, 50), S(57, 57), S(65, 65), S(70, 70)
};

static const int QueenMobility[28] = {
    S(-65, -65), S(-57, -57), S(-50, -50), S(-45, -45), S(-38, -38), S(-32, -32), S(-25, -25),
    S(-20, -20), S(-13, -13), S( -6,  -6), S(  0,   0), S(  6,   6), S( 13,  13), S( 20,  20),
    S( 25,  25), S( 32,  32), S( 38,  38), S( 45,  45), S( 50,  50), S( 57,  57), S( 65,  65),
    S( 70,  70), S( 75,  75), S( 83,  83), S( 90,  90), S( 95,  95), S( 100,  100), S( 105,  105)
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
            return pos->nonPawns[WHITE] <= 2 && pos->nonPawns[BLACK] <= 2;

        // No knights
        } else if (!pieceBB(KNIGHT)) {
            // Draw unless one side has 2 extra bishops (all nonpawns are bishops)
            return abs(pos->nonPawns[WHITE] - pos->nonPawns[BLACK]) < 2;

        // Draw with 1-2 knights vs 1 bishop (there is at least 1 bishop, and at last 1 knight)
        } else if (Single(pieceBB(BISHOP))) {
            int bishopOwner = pieceBB(BISHOP) & colorBB(WHITE) ? WHITE : BLACK;
            return pos->nonPawns[bishopOwner] == 1 && pos->nonPawns[!bishopOwner] <= 2;
        }
    // Draw with 1 rook + up to 1 minor each
    } else if (Single(pieceBB(ROOK) & colorBB(WHITE)) && Single(pieceBB(ROOK) & colorBB(BLACK))) {
        return pos->nonPawns[WHITE] <= 2 && pos->nonPawns[BLACK] <= 2;

    // Draw with 1 rook vs 1-2 minors
    } else if (Single(pieceBB(ROOK))) {
        int rookOwner = pieceBB(ROOK) & colorBB(WHITE) ? WHITE : BLACK;
        return pos->nonPawns[rookOwner] == 1 && pos->nonPawns[!rookOwner] >= 1 && pos->nonPawns[!rookOwner] <= 2;
    }

    return false;
}
#endif

// Evaluates pawns
INLINE int EvalPawns(const Position *pos, const int color) {

    int eval = 0;

    Bitboard pieces = colorBB(color) & pieceBB(PAWN);
    while (pieces) {
        Square sq = PopLsb(&pieces);

        // Isolation penalty
        if (!(IsolatedMask[sq] & colorBB(color) & pieceBB(PAWN)))
            eval += PawnIsolated;
        // Passed bonus
        if (!((PassedMask[color][sq]) & colorBB(!color) & pieceBB(PAWN)))
            eval += PawnPassed[RelativeRank(color, RankOf(sq))];
    }

    return eval;
}

// Evaluates knights
INLINE int EvalKnights(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;

    Bitboard pieces = colorBB(color) & pieceBB(KNIGHT);
    while (pieces) {
        Square sq = PopLsb(&pieces);

        // Mobility
        eval += KnightMobility[PopCount(AttackBB(KNIGHT, sq, pieceBB(ALL)) & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates bishops
INLINE int EvalBishops(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;

    Bitboard pieces = colorBB(color) & pieceBB(BISHOP);

    // Bishop pair
    if (PopCount(pieces) >= 2)
        eval += BishopPair;

    while (pieces) {
        Square sq = PopLsb(&pieces);

        // Mobility
        eval += BishopMobility[PopCount(AttackBB(BISHOP, sq, pieceBB(ALL)) & ei->mobilityArea[color])];
    }



    return eval;
}

// Evaluates rooks
INLINE int EvalRooks(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;

    Bitboard pieces = colorBB(color) & pieceBB(ROOK);
    while (pieces) {
        Square sq = PopLsb(&pieces);

        // Open/Semi-open file bonus
        if (!(pieceBB(PAWN) & FileBB[FileOf(sq)]))
            eval += RookOpenFile;
        else if (!(colorBB(color) & pieceBB(PAWN) & FileBB[FileOf(sq)]))
            eval += RookSemiOpenFile;

        // Mobility
        eval += RookMobility[PopCount(AttackBB(ROOK, sq, pieceBB(ALL)) & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates queens
INLINE int EvalQueens(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;

    Bitboard pieces = colorBB(color) & pieceBB(QUEEN);
    while (pieces) {
        Square sq = PopLsb(&pieces);

        // Open/Semi-open file bonus
        if (!(pieceBB(PAWN) & FileBB[FileOf(sq)]))
            eval += QueenOpenFile;
        else if (!(colorBB(color) & pieceBB(PAWN) & FileBB[FileOf(sq)]))
            eval += QueenSemiOpenFile;

        // Mobility
        eval += QueenMobility[PopCount(AttackBB(QUEEN, sq, pieceBB(ALL)) & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates kings
INLINE int EvalKings(const Position *pos, const int color) {

    int eval = 0;

    Square kingSq = Lsb(colorBB(color) & pieceBB(KING));

    // King safety
    eval += KingLineVulnerability * PopCount(AttackBB(QUEEN, kingSq, colorBB(color) | pieceBB(PAWN)));

    return eval;
}

INLINE int EvalPieces(const EvalInfo ei, const Position *pos) {

    return  EvalPawns  (pos, WHITE)
          - EvalPawns  (pos, BLACK)
          + EvalKnights(&ei, pos, WHITE)
          - EvalKnights(&ei, pos, BLACK)
          + EvalBishops(&ei, pos, WHITE)
          - EvalBishops(&ei, pos, BLACK)
          + EvalRooks  (&ei, pos, WHITE)
          - EvalRooks  (&ei, pos, BLACK)
          + EvalQueens (&ei, pos, WHITE)
          - EvalQueens (&ei, pos, BLACK)
          + EvalKings  (pos, WHITE)
          - EvalKings  (pos, BLACK);
}

// Initializes the eval info struct
INLINE void InitEvalInfo(const Position *pos, EvalInfo *ei, const int color) {

    const int down = (color == WHITE ? SOUTH : NORTH);

    Bitboard b = RankBB[RelativeRank(color, RANK_2)] | ShiftBB(down, pieceBB(ALL));

    b &= colorBB(color) & pieceBB(PAWN);

    // Mobility area is defined as any square not attacked by an enemy pawn,
    // nor occupied by our own pawn on its starting square or blocked from advancing.
    ei->mobilityArea[color] = ~(b | PawnBBAttackBB(colorBB(!color) & pieceBB(PAWN), !color));
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
    eval += EvalPieces(ei, pos);

    // Adjust score by phase
    const int phase = pos->phase;
    eval = ((MgScore(eval) * phase)
         +  (EgScore(eval) * (256 - phase)))
         / 256;

    assert(-INFINITE < eval && eval < INFINITE);

    // Return the evaluation, negated if we are black
    return sideToMove() == WHITE ? eval : -eval;
}