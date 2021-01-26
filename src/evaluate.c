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

#include "tuner/tuner.h"
#include "bitboard.h"
#include "evaluate.h"


typedef struct EvalInfo {
    Bitboard mobilityArea[COLOR_NB];
    Bitboard enemyKingZone[COLOR_NB];
    int16_t KingAttackPower[COLOR_NB];
    int16_t KingAttackCount[COLOR_NB];
} EvalInfo;


extern EvalTrace T;


// Piecetype values, combines with PSQTs [piecetype]
const int PieceTypeValue[7] = { 0,
    S(P_MG, P_EG),
    S(N_MG, N_EG),
    S(B_MG, B_EG),
    S(R_MG, R_EG),
    S(Q_MG, Q_EG)
};

// Phase piece values, lookup used for futility pruning [phase][piece]
const int PieceValue[COLOR_NB][PIECE_NB] = {
    { 0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0,
      0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0 },
    { 0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0,
      0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0 }
};

// Phase value for each piece [piece]
const int PhaseValue[PIECE_NB] = {
    0, 0, 1, 1, 2, 4, 0, 0,
    0, 0, 1, 1, 2, 4, 0, 0
};

// Bonus for being the side to move
const int Tempo = 15;

// Misc bonuses and maluses
const int PawnDoubled    = S(-10,-24);
const int PawnIsolated   = S(-14,-17);
const int PawnSupport    = S( 12,  5);
const int BishopPair     = S( 20, 98);
const int KingLineDanger = S( -5, -2);

// Passed pawn [rank]
const int PawnPassed[RANK_NB] = {
    S(  0,  0), S(-10, 19), S(-13, 23), S( -9, 53),
    S( 21, 75), S( 53,133), S(128,185), S(  0,  0),
};

// (Semi) open file for rook and queen [pt-4]
const int OpenFile[2]     = { S( 31, 10), S( -4,  2) };
const int SemiOpenFile[2] = { S(  9, 16), S(  2,  2) };

// KingSafety [pt-2]
const uint16_t PieceAttackPower[4] = {35, 20, 40, 80};
const uint16_t PieceCountModifier[8] = {0, 0, 50, 75, 80, 88, 95, 100};

// Mobility [pt-2][mobility]
const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-64,-53), S(-25,-64), S( -5,-28), S(  6,  8), S( 15, 25), S( 19, 47), S( 26, 51), S( 36, 50),
      S( 52, 32) },
    // Bishop (0-13)
    { S(-58,-84), S(-24,-85), S( -6,-42), S(  2, -9), S( 11,  8), S( 19, 33), S( 23, 50), S( 23, 58),
      S( 24, 67), S( 30, 68), S( 39, 63), S( 63, 52), S( 60, 71), S( 48, 53) },
    // Rook (0-14)
    { S(-59,-69), S(-29,-52), S(-15,-32), S(-15,-14), S( -8,  9), S( -5, 27), S( -8, 47), S( -2, 51),
      S(  4, 57), S( 11, 64), S( 21, 69), S( 30, 70), S( 37, 72), S( 53, 62), S( 85, 40) },
    // Queen (0-27)
    { S(-62,-48), S(-68,-36), S(-60,-47), S(-46,-48), S(-33,-44), S(-17,-43), S( -7,-37), S(  0,-24),
      S(  6, -7), S( 12,  6), S( 16, 19), S( 20, 28), S( 25, 33), S( 26, 43), S( 28, 51), S( 29, 60),
      S( 29, 65), S( 33, 64), S( 31, 71), S( 36, 70), S( 41, 75), S( 58, 66), S( 65, 72), S( 84, 68),
      S(109, 89), S(113, 86), S(104,111), S(108,131) }
};


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

// Evaluates pawns
INLINE int EvalPawns(const Position *pos, const Color color) {

    int eval = 0, count;

    Bitboard pawns = colorPieceBB(color, PAWN);

    // Doubled pawns (only when one is blocking the other from moving)
    count = PopCount(pawns & ShiftBB(NORTH, pawns));
    eval += PawnDoubled * count;
    if (TRACE) T.PawnDoubled[color] += count;

    // Supported pawns
    count = PopCount(pawns & PawnBBAttackBB(pawns, color));
    eval += PawnSupport * count;
    if (TRACE) T.PawnSupport[color] += count;

    // Evaluate each individual pawn
    while (pawns) {

        Square sq = PopLsb(&pawns);

        if (TRACE) T.PieceValue[PAWN-1][color]++;
        if (TRACE) T.PSQT[PAWN-1][RelativeSquare(color, sq)][color]++;

        // Isolated pawns
        if (!(IsolatedMask[sq] & colorPieceBB(color, PAWN))) {
            eval += PawnIsolated;
            if (TRACE) T.PawnIsolated[color]++;
        }

        // Passed pawns
        if (!((PassedMask[color][sq]) & colorPieceBB(!color, PAWN))) {
            eval += PawnPassed[RelativeRank(color, RankOf(sq))];
            if (TRACE) T.PawnPassed[RelativeRank(color, RankOf(sq))][color]++;
        }
    }

    return eval;
}

// Tries to get pawn eval from cache, otherwise evaluates and saves
int ProbePawnCache(const Position *pos, PawnCache pc) {

    // Can't cache when tuning as full trace is needed
    if (TRACE || !pc) return EvalPawns(pos, WHITE) - EvalPawns(pos, BLACK);

    Key key = pos->pawnKey;
    PawnEntry *pe = &pc[key % PAWN_CACHE_SIZE];

    if (pe->key != key)
        pe->key  = key,
        pe->eval = EvalPawns(pos, WHITE) - EvalPawns(pos, BLACK);

    return pe->eval;
}

// Evaluates knights, bishops, rooks, or queens
INLINE int EvalPiece(const Position *pos, EvalInfo *ei, const Color color, const PieceType pt) {

    int eval = 0;

    Bitboard pieces = colorPieceBB(color, pt);

    // Bishop pair
    if (pt == BISHOP && Multiple(pieces)) {
        eval += BishopPair;
        if (TRACE) T.BishopPair[color]++;
    }

    // Evaluate each individual piece
    while (pieces) {

        Square sq = PopLsb(&pieces);

        if (TRACE) T.PieceValue[pt-1][color]++;
        if (TRACE) T.PSQT[pt-1][RelativeSquare(color, sq)][color]++;

        // Mobility
        Bitboard mobilityBB = XRayAttackBB(pos, color, pt, sq) & ei->mobilityArea[color];
        int mob = PopCount(mobilityBB);
        eval += Mobility[pt-2][mob];
        if (TRACE) T.Mobility[pt-2][mob][color]++;

        // Attacks for king safety calculations
        int kingAttack = PopCount(mobilityBB & ei->enemyKingZone[color]);

        Bitboard checks = AttackBB(pt,  Lsb(colorPieceBB(!color, KING)), pieceBB(ALL)) & mobilityBB;

        if (kingAttack > 0 || checks != 0) {
            ei->KingAttackCount[color]++;
            ei->KingAttackPower[color] += kingAttack * PieceAttackPower[pt - 2];
        }

        if (pt == ROOK || pt == QUEEN) {

            // Open file
            if (!(pieceBB(PAWN) & FileBB[FileOf(sq)])) {
                eval += OpenFile[pt-4];
                if (TRACE) T.OpenFile[pt-4][color]++;
            // Semi-open file
            } else if (!(colorPieceBB(color, PAWN) & FileBB[FileOf(sq)])) {
                eval += SemiOpenFile[pt-4];
                if (TRACE) T.SemiOpenFile[pt-4][color]++;
            }
        }
    }

    return eval;
}

// Evaluates kings
INLINE int EvalKings(const Position *pos, const Color color) {

    int eval = 0;

    Square kingSq = Lsb(colorPieceBB(color, KING));

    if (TRACE) T.PSQT[KING-1][RelativeSquare(color, kingSq)][color]++;

    // Open lines from the king
    int count = PopCount(AttackBB(QUEEN, kingSq, colorBB(color) | pieceBB(PAWN)));
    eval += KingLineDanger * count;
    if (TRACE) T.KingLineDanger[color] += count;

    return eval;
}

INLINE int EvalPieces(const Position *pos, EvalInfo *ei) {
    return  EvalPiece(pos, ei, WHITE, KNIGHT)
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

INLINE int EvalSafety(const Color color ,const EvalInfo *ei) {
    int16_t safetyScore =  ei->KingAttackPower[color]
                         * PieceCountModifier[MIN(7, ei->KingAttackCount[color])]
                         / 100;

    return S(safetyScore, 0);
}

// Initializes the eval info struct
INLINE void InitEvalInfo(const Position *pos, EvalInfo *ei, const Color color) {

    const Direction up   = (color == WHITE ? NORTH : SOUTH);
    const Direction down = (color == WHITE ? SOUTH : NORTH);

    // Mobility area is defined as any square not attacked by an enemy pawn, nor
    // occupied by our own pawn either on its starting square or blocked from advancing.
    Bitboard b = RankBB[RelativeRank(color, RANK_2)] | ShiftBB(down, pieceBB(ALL));
    b &= colorPieceBB(color, PAWN);

    ei->mobilityArea[color] = ~(b | PawnBBAttackBB(colorPieceBB(!color, PAWN), !color));

    // King Safety
    ei->KingAttackCount[color] = 0;
    ei->KingAttackPower[color] = 0;

    Square kingSq = Lsb(colorPieceBB(!color, KING));
    Bitboard kingZone = AttackBB(KING, kingSq, 0);
    ei->enemyKingZone[color] = kingZone | ShiftBB(up, kingZone);
}

// Calculate a static evaluation of a position
int EvalPosition(const Position *pos, PawnCache pc) {

    if (MaterialDraw(pos)) return 0;

    EvalInfo ei;
    InitEvalInfo(pos, &ei, WHITE);
    InitEvalInfo(pos, &ei, BLACK);

    // Material (includes PSQT)
    int eval = pos->material;

    // Evaluate pawns
    eval += ProbePawnCache(pos, pc);

    // Evaluate pieces
    eval += EvalPieces(pos, &ei);

    // Evaluate KingSafety
    eval += EvalSafety(WHITE, &ei) - EvalSafety(BLACK, &ei);

    if (TRACE) T.eval = eval;

    // Adjust score by phase
    eval =  ((MgScore(eval) * pos->phase)
          +  (EgScore(eval) * (MidGame - pos->phase)))
          / MidGame;

    // Scale down eval for opposite-colored bishops endgames
    if (  !pieceBB(QUEEN) && !pieceBB(ROOK) && !pieceBB(KNIGHT)
        && pos->nonPawnCount[WHITE] == 1
        && pos->nonPawnCount[BLACK] == 1
        && (Single(pieceBB(BISHOP) & BlackSquaresBB)))
        eval /= 2;

    // Return the evaluation, negated if we are black
    return (sideToMove == WHITE ? eval : -eval) + Tempo;
}
