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
const int PhaseValue[TYPE_NB] = {
    0, 0, 1, 1, 2, 4, 0, 0
};

// Bonus for being the side to move
const int Tempo = 15;

// Misc bonuses and maluses
const int PawnDoubled    = S(-13,-25);
const int PawnIsolated   = S(-14,-18);
const int PawnSupport    = S( 13,  5);
const int BishopPair     = S( 26, 99);
const int BishopOutPost  = S(  4, -2);
const int KnightOutPost  = S( 21, 14);

// Passed pawn [rank]
const int PawnPassed[8] = {
    S(  0,  0), S(-15, 22), S(-16, 25), S( -7, 55),
    S( 26, 79), S( 59,138), S(134,194), S(  0,  0),
};

// (Semi) open file for rook and queen [pt-4]
const int OpenFile[2]     = { S( 27,  9), S( -9,  4) };
const int SemiOpenFile[2] = { S(  9, 15), S(  1,  4) };

// KingSafety [pt-2]
const uint16_t PieceAttackPower[4] = {35, 20, 40, 80};
const uint16_t PieceCheckPower[4]  = {100, 35, 65, 65};
const uint16_t PieceCountModifier[8] = {0, 0, 50, 75, 80, 88, 95, 100};

// KingLineDanger
const int KingLineDanger[28] = {
    S(  0,  0), S( -5, -2), S(-17,-20), S(-17, -8),
    S(-35,-13), S(-40,-23), S(-38,-26), S(-39,-21),
    S(-42,-20), S(-51,-19), S(-46,-21), S(-54,-15),
    S(-57,-16), S(-60,-14), S(-66,-14), S(-69,-13),
    S(-73,-16), S(-77,-20), S(-82,-24), S(-89,-32),
    S(-96,-34), S(-102,-43), S(-109,-51), S(-115,-63),
    S(-121,-62), S(-126,-58), S(-130,-53), S(-135,-55),
};

// Mobility [pt-2][mobility]
const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-59,-54), S(-24,-66), S( -4,-23), S(  6, 14), S( 15, 29), S( 18, 48), S( 26, 52), S( 36, 47),
      S( 51, 28) },
    // Bishop (0-13)
    { S(-55,-93), S(-19,-91), S( -1,-37), S(  6, -4), S( 14, 13), S( 21, 37), S( 24, 53), S( 22, 60),
      S( 22, 68), S( 26, 68), S( 31, 65), S( 58, 51), S( 59, 68), S( 49, 51) },
    // Rook (0-14)
    { S(-59,-69), S(-28,-57), S(-13,-35), S(-12,-17), S( -6, 14), S( -3, 34), S( -3, 54), S(  2, 56),
      S(  9, 60), S( 16, 66), S( 25, 70), S( 28, 70), S( 28, 72), S( 43, 60), S( 87, 35) },
    // Queen (0-27)
    { S(-62,-48), S(-70,-36), S(-65,-49), S(-45,-50), S(-28,-46), S(-10,-45), S(  1,-37), S(  8,-23),
      S( 13, -7), S( 18,  8), S( 21, 23), S( 24, 33), S( 27, 38), S( 27, 48), S( 28, 54), S( 28, 62),
      S( 26, 68), S( 26, 68), S( 25, 72), S( 30, 71), S( 35, 74), S( 52, 63), S( 61, 69), S( 80, 66),
      S(107, 86), S(112, 84), S(104,111), S(108,131) }
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

        int checks = PopCount(AttackBB(pt, Lsb(colorPieceBB(!color, KING)), pieceBB(ALL)) & mobilityBB);

        if (kingAttack > 0 || checks > 0) {
            ei->KingAttackCount[color]++;
            ei->KingAttackPower[color] += kingAttack * PieceAttackPower[pt - 2]
                                        + checks * PieceCheckPower[pt - 2];
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

        if (pt == BISHOP || pt == KNIGHT ){
            Bitboard piece = 1 << sq;
            Bitboard oPost = color == WHITE ? WhiteOutpost : BlackOutpost;
            if ((piece & oPost) && ((colorPieceBB(!color, PAWN) & OutpostMasks[color][sq]) == 0) && (piece & PawnBBAttackBB(colorPieceBB(color, PAWN), color))){
                if (pt == BISHOP) {
                    eval += BishopOutPost;
                    if (TRACE) T.BishopOutPost[color]++;
                } else {
                    eval += KnightOutPost;
                    if (TRACE) T.KnightOutPost[color]++;
                }
            }
        }
    }

    return eval;
}

// Evaluates kings
INLINE int EvalKings(const Position *pos, EvalInfo *ei, const Color color) {

    int eval = 0;

    Square kingSq = Lsb(colorPieceBB(color, KING));

    if (TRACE) T.PSQT[KING-1][RelativeSquare(color, kingSq)][color]++;

    // Open lines from the king
    Bitboard SafeLine = RankBB[RelativeRank(color, RANK_1)];
    int count = PopCount((~SafeLine) & AttackBB(QUEEN, kingSq, colorBB(color) | pieceBB(PAWN)));
    eval += KingLineDanger[count];
    ei->KingAttackPower[!color] += (count - 3) * 8;
    if (TRACE) T.KingLineDanger[count][color]++;

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
          + EvalKings(pos, ei, WHITE)
          - EvalKings(pos, ei, BLACK);
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

    // Evaluate king safety
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
