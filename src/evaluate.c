// evaluate.c

#include <stdlib.h>
#include <string.h>

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "psqt.h"
#include "validate.h"


// Eval bit masks
static bitboard PassedMask[2][64];
static bitboard IsolatedMask[64];

// Various bonuses and maluses
static const int PawnPassed[8] = { 0, S(5, 5), S(10, 10), S(20, 20), S(35, 35), S(60, 60), S(100, 100), 0 };
static const int PawnIsolated = S(-15, -14);

static const int  RookOpenFile = S(30, 15);
static const int QueenOpenFile = S(20, 30);
static const int  RookSemiOpenFile = S(10, 20);
static const int QueenSemiOpenFile = S(20, 15);

static const int BishopPair = S(50, 50);

static const int KingLineVulnerability = S(-8, 0);

// Mobility
static const int KnightMobility[9] = {
    S(-50, -50), S(-25, -25), S(-15, -15), S(0, 0), S(15, 15), S(25, 25), S(35, 35), S(40, 40), S(50, 50)
};

static const int BishopMobility[14] = {
    S(-50, -50), S(-35, -35), S(-25, -25), S(-10, -10), S( 0,  0), S(10, 10), S(15, 15),
    S( 20,  20), S( 25,  25), S( 30,  30), S( 35,  35), S(40, 40), S(45, 45), S(50, 50)
};

static const int RookMobility[15] = {
    S(-50, -50), S(-35, -35), S(-25, -25), S(-10, -10), S( 0,  0), S(10, 10), S(15, 15),
    S( 20,  20), S( 25,  25), S( 30,  30), S( 35,  35), S(40, 40), S(45, 45), S(50, 50), S(55, 55)
};

static const int QueenMobility[28] = {
    S(-50, -50), S(-45, -45), S(-40, -40), S(-35, -35), S(-30, -30), S(-25, -25), S(-20, -20),
    S(-15, -15), S(-10, -10), S( -5,  -5), S(  0,   0), S(  5,   5), S( 10,  10), S( 15,  15),
    S( 20,  20), S( 25,  25), S( 30,  30), S( 35,  35), S( 40,  40), S( 45,  45), S( 50,  50),
    S( 55,  55), S( 60,  60), S( 65,  65), S( 70,  70), S( 75,  75), S( 80,  80), S( 85,  85)
};


// Initialize evaluation bit masks
CONSTR InitEvalMasks() {

    int sq, tsq;

    // For each square a pawn can be on
    for (sq = A2; sq <= H7; ++sq) {

        // In front
        for (tsq = sq + 8; tsq <= H8; tsq += 8)
            PassedMask[WHITE][sq] |= (1ULL << tsq);

        for (tsq = sq - 8; tsq >= A1; tsq -= 8)
            PassedMask[BLACK][sq] |= (1ULL << tsq);

        // Left side
        if (fileOf(sq) > FILE_A) {

            IsolatedMask[sq] |= fileBBs[fileOf(sq) - 1];

            for (tsq = sq + 7; tsq <= H8; tsq += 8)
                PassedMask[WHITE][sq] |= (1ULL << tsq);

            for (tsq = sq - 9; tsq >= A1; tsq -= 8)
                PassedMask[BLACK][sq] |= (1ULL << tsq);
        }

        // Right side
        if (fileOf(sq) < FILE_H) {

            IsolatedMask[sq] |= fileBBs[fileOf(sq) + 1];

            for (tsq = sq + 9; tsq <= H8; tsq += 8)
                PassedMask[WHITE][sq] |= (1ULL << tsq);

            for (tsq = sq - 7; tsq >= A1; tsq -= 8)
                PassedMask[BLACK][sq] |= (1ULL << tsq);
        }
    }
}

#ifdef CHECK_MAT_DRAW
// Check if the board is (likely) drawn, logic from sjeng
static bool MaterialDraw(const Position *pos) {

    assert(CheckBoard(pos));

    // No draw with queens or pawns
    if (pos->pieceCounts[wQ] || pos->pieceCounts[bQ] || pos->pieceCounts[wP] || pos->pieceCounts[bP])
        return false;

    // No rooks
    if (!pos->pieceCounts[wR] && !pos->pieceCounts[bR]) {

        // No bishops
        if (!pos->pieceCounts[bB] && !pos->pieceCounts[wB]) {
            // Draw with 1-2 knights (or 0 => KvK)
            if (pos->pieceCounts[wN] < 3 && pos->pieceCounts[bN] < 3)
                return true;

        // No knights
        } else if (!pos->pieceCounts[wN] && !pos->pieceCounts[bN]) {
            // Draw unless one side has 2 extra bishops
            if (abs(pos->pieceCounts[wB] - pos->pieceCounts[bB]) < 2)
                return true;

        // Draw with 1-2 knights vs 1 bishop
        } else if ((pos->pieceCounts[wN] < 3 && !pos->pieceCounts[wB]) || (pos->pieceCounts[wB] == 1 && !pos->pieceCounts[wN]))
            if ((pos->pieceCounts[bN] < 3 && !pos->pieceCounts[bB]) || (pos->pieceCounts[bB] == 1 && !pos->pieceCounts[bN]))
                return true;

    // Draw with 1 rook each + up to 1 N or B each
    } else if (pos->pieceCounts[wR] == 1 && pos->pieceCounts[bR] == 1) {
        if ((pos->pieceCounts[wN] + pos->pieceCounts[wB]) < 2 && (pos->pieceCounts[bN] + pos->pieceCounts[bB]) < 2)
            return true;

    // Draw with white having 1 rook vs 1-2 N/B
    } else if (pos->pieceCounts[wR] == 1 && !pos->pieceCounts[bR]) {
        if ((pos->pieceCounts[wN] + pos->pieceCounts[wB] == 0) && (((pos->pieceCounts[bN] + pos->pieceCounts[bB]) == 1) || ((pos->pieceCounts[bN] + pos->pieceCounts[bB]) == 2)))
            return true;

    // Draw with black having 1 rook vs 1-2 N/B
    } else if (pos->pieceCounts[bR] == 1 && !pos->pieceCounts[wR])
        if ((pos->pieceCounts[bN] + pos->pieceCounts[bB] == 0) && (((pos->pieceCounts[wN] + pos->pieceCounts[wB]) == 1) || ((pos->pieceCounts[wN] + pos->pieceCounts[wB]) == 2)))
            return true;

    return false;
}
#endif

// Evaluates pawns
INLINE int evalPawns(const EvalInfo *ei, const Position *pos, const int pawns, const int color) {

    int eval = 0;

    for (int i = 0; i < pos->pieceCounts[pawns]; ++i) {
        int sq = pos->pieceList[pawns][i];

        // Isolation penalty
        if (!(IsolatedMask[sq] & ei->pawnsBB[color]))
            eval += PawnIsolated;
        // Passed bonus
        if (!((PassedMask[color][sq]) & ei->pawnsBB[!color]))
            eval += PawnPassed[color ? rankOf(sq) : 7 - rankOf(sq)];
    }

    return eval;
}

// Evaluates knights
INLINE int evalKnights(const EvalInfo *ei, const Position *pos, const int knights, const int color) {

    int eval = 0;

    for (int i = 0; i < pos->pieceCounts[knights]; ++i) {
        int sq = pos->pieceList[knights][i];

        // Mobility
        eval += KnightMobility[PopCount(knight_attacks[sq] & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates bishops
INLINE int evalBishops(const EvalInfo *ei, const Position *pos, const int bishops, const int color) {

    int eval = 0;

    for (int i = 0; i < pos->pieceCounts[bishops]; ++i) {
        int sq = pos->pieceList[bishops][i];

        // Mobility
        eval += BishopMobility[PopCount(BishopAttacks(sq, pos->colorBBs[BOTH]) & ei->mobilityArea[color])];
    }

    // Bishop pair
    if (pos->pieceCounts[bishops] >= 2)
        eval += BishopPair;

    return eval;
}

// Evaluates rooks
INLINE int evalRooks(const EvalInfo *ei, const Position *pos, const int rooks, const int color) {

    int eval = 0;

    for (int i = 0; i < pos->pieceCounts[rooks]; ++i) {
        int sq = pos->pieceList[rooks][i];

        // Open/Semi-open file bonus
        if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
            eval += RookOpenFile;
        else if (!(ei->pawnsBB[color] & fileBBs[fileOf(sq)]))
            eval += RookSemiOpenFile;

        // Mobility
        eval += RookMobility[PopCount(RookAttacks(sq, pos->colorBBs[BOTH]) & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates queens
INLINE int evalQueens(const EvalInfo *ei, const Position *pos, const int queens, const int color) {

    int eval = 0;

    for (int i = 0; i < pos->pieceCounts[queens]; ++i) {
        int sq = pos->pieceList[queens][i];

        // Open/Semi-open file bonus
        if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
            eval += QueenOpenFile;
        else if (!(ei->pawnsBB[color] & fileBBs[fileOf(sq)]))
            eval += QueenSemiOpenFile;

        // Mobility
        eval += QueenMobility[PopCount((BishopAttacks(sq, pos->colorBBs[BOTH])
                                        | RookAttacks(sq, pos->colorBBs[BOTH])) & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates kings
INLINE int evalKings(const Position *pos, const int color) {

    int eval = 0;

    // King safety
    eval += KingLineVulnerability * PopCount(BishopAttacks(pos->kingSq[color], pos->colorBBs[color] | pos->pieceBBs[PAWN])
                                             | RookAttacks(pos->kingSq[color], pos->colorBBs[color] | pos->pieceBBs[PAWN]));

    return eval;
}

INLINE int evalPieces(const EvalInfo ei, const Position *pos) {

    return  evalPawns  (&ei, pos, wP, WHITE)
          - evalPawns  (&ei, pos, bP, BLACK)
          + evalKnights(&ei, pos, wN, WHITE)
          - evalKnights(&ei, pos, bN, BLACK)
          + evalBishops(&ei, pos, wB, WHITE)
          - evalBishops(&ei, pos, bB, BLACK)
          + evalRooks  (&ei, pos, wR, WHITE)
          - evalRooks  (&ei, pos, bR, BLACK)
          + evalQueens (&ei, pos, wQ, WHITE)
          - evalQueens (&ei, pos, bQ, BLACK)
          + evalKings  (pos, WHITE)
          - evalKings  (pos, BLACK);
}

// Calculate a static evaluation of a position
int EvalPosition(const Position *pos) {

#ifdef CHECK_MAT_DRAW
    if (MaterialDraw(pos)) return 0;
#endif

    EvalInfo ei;

    bitboard blockedPawns[2], unmovedPawns[2], attackedByPawns[2];

    ei.pawnsBB[WHITE] = pos->colorBBs[WHITE] & pos->pieceBBs[PAWN];
    ei.pawnsBB[BLACK] = pos->colorBBs[BLACK] & pos->pieceBBs[PAWN];

    // Mobility area
    blockedPawns[BLACK] = ei.pawnsBB[BLACK] & pos->colorBBs[BOTH] << 8;
    blockedPawns[WHITE] = ei.pawnsBB[WHITE] & pos->colorBBs[BOTH] >> 8;

    unmovedPawns[BLACK] = ei.pawnsBB[BLACK] & rank7BB;
    unmovedPawns[WHITE] = ei.pawnsBB[WHITE] & rank2BB;

    attackedByPawns[BLACK] = ((ei.pawnsBB[BLACK] & ~fileABB) >> 9)
                           | ((ei.pawnsBB[BLACK] & ~fileHBB) >> 7);
    attackedByPawns[WHITE] = ((ei.pawnsBB[WHITE] & ~fileABB) << 7)
                           | ((ei.pawnsBB[WHITE] & ~fileHBB) << 9);

    ei.mobilityArea[BLACK] = ~(blockedPawns[BLACK] | unmovedPawns[BLACK] | attackedByPawns[WHITE]);
    ei.mobilityArea[WHITE] = ~(blockedPawns[WHITE] | unmovedPawns[WHITE] | attackedByPawns[BLACK]);

    // Material
    int eval = pos->material;

    // Evaluate pieces
    eval += evalPieces(ei, pos);

    // Adjust score by phase
    const int phase = pos->phase;
    eval = ((MgScore(eval) * (256 - phase)) + (EgScore(eval) * phase)) / 256;

    assert(eval > -INFINITE && eval < INFINITE);

    // Return the evaluation, negated if we are black
    return pos->side == WHITE ? eval : -eval;
}