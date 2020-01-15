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
INLINE int evalPawns(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;
    int pawns = makePiece(color, PAWN);

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
INLINE int evalKnights(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;
    int knights = makePiece(color, KNIGHT);

    for (int i = 0; i < pos->pieceCounts[knights]; ++i) {
        int sq = pos->pieceList[knights][i];

        // Mobility
        eval += KnightMobility[PopCount(knight_attacks[sq] & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates bishops
INLINE int evalBishops(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;
    int bishops = makePiece(color, BISHOP);

    for (int i = 0; i < pos->pieceCounts[bishops]; ++i) {
        int sq = pos->pieceList[bishops][i];

        // Mobility
        eval += BishopMobility[PopCount(BishopAttacks(sq, pieceBB(ALL)) & ei->mobilityArea[color])];
    }

    // Bishop pair
    if (pos->pieceCounts[bishops] >= 2)
        eval += BishopPair;

    return eval;
}

// Evaluates rooks
INLINE int evalRooks(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;
    int rooks = makePiece(color, ROOK);

    for (int i = 0; i < pos->pieceCounts[rooks]; ++i) {
        int sq = pos->pieceList[rooks][i];

        // Open/Semi-open file bonus
        if (!(pieceBB(PAWN) & fileBBs[fileOf(sq)]))
            eval += RookOpenFile;
        else if (!(ei->pawnsBB[color] & fileBBs[fileOf(sq)]))
            eval += RookSemiOpenFile;

        // Mobility
        eval += RookMobility[PopCount(RookAttacks(sq, pieceBB(ALL)) & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates queens
INLINE int evalQueens(const EvalInfo *ei, const Position *pos, const int color) {

    int eval = 0;
    int queens = makePiece(color, QUEEN);

    for (int i = 0; i < pos->pieceCounts[queens]; ++i) {
        int sq = pos->pieceList[queens][i];

        // Open/Semi-open file bonus
        if (!(pieceBB(PAWN) & fileBBs[fileOf(sq)]))
            eval += QueenOpenFile;
        else if (!(ei->pawnsBB[color] & fileBBs[fileOf(sq)]))
            eval += QueenSemiOpenFile;

        // Mobility
        eval += QueenMobility[PopCount((BishopAttacks(sq, pieceBB(ALL))
                                        | RookAttacks(sq, pieceBB(ALL))) & ei->mobilityArea[color])];
    }

    return eval;
}

// Evaluates kings
INLINE int evalKings(const Position *pos, const int color) {

    int eval = 0;
    int kingSq = pos->pieceList[makePiece(color, KING)][0];

    // King safety
    eval += KingLineVulnerability * PopCount(BishopAttacks(kingSq, colorBB(color) | pieceBB(PAWN))
                                             | RookAttacks(kingSq, colorBB(color) | pieceBB(PAWN)));

    return eval;
}

INLINE int evalPieces(const EvalInfo ei, const Position *pos) {

    return  evalPawns  (&ei, pos, WHITE)
          - evalPawns  (&ei, pos, BLACK)
          + evalKnights(&ei, pos, WHITE)
          - evalKnights(&ei, pos, BLACK)
          + evalBishops(&ei, pos, WHITE)
          - evalBishops(&ei, pos, BLACK)
          + evalRooks  (&ei, pos, WHITE)
          - evalRooks  (&ei, pos, BLACK)
          + evalQueens (&ei, pos, WHITE)
          - evalQueens (&ei, pos, BLACK)
          + evalKings  (pos, WHITE)
          - evalKings  (pos, BLACK);
}

// Calculate a static evaluation of a position
int EvalPosition(const Position *pos) {

#ifdef CHECK_MAT_DRAW
    if (MaterialDraw(pos)) return 0;
#endif

    EvalInfo ei;

    Bitboard blockedPawns[2], unmovedPawns[2], attackedByPawns[2];

    ei.pawnsBB[WHITE] = colorBB(WHITE) & pieceBB(PAWN);
    ei.pawnsBB[BLACK] = colorBB(BLACK) & pieceBB(PAWN);

    // Mobility area
    blockedPawns[BLACK] = ei.pawnsBB[BLACK] & pieceBB(ALL) << 8;
    blockedPawns[WHITE] = ei.pawnsBB[WHITE] & pieceBB(ALL) >> 8;

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
    return sideToMove() == WHITE ? eval : -eval;
}