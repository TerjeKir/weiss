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
    int16_t attackPower[COLOR_NB];
    int16_t attackCount[COLOR_NB];
} EvalInfo;


// Piecetype values, combines with PSQTs [piecetype]
const int PieceTypeValue[7] = {
    0, S(P_MG, P_EG), S(N_MG, N_EG), S(B_MG, B_EG), S(R_MG, R_EG), S(Q_MG, Q_EG), 0
};

// Phase piece values, lookup used for futility pruning [phase][piece]
const int PieceValue[COLOR_NB][PIECE_NB] = {
    { 0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0,
      0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0 },
    { 0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0,
      0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0 }
};

// Phase value for each piece [piece]
const int PhaseValue[TYPE_NB] = { 0, 0, 1, 1, 2, 4, 0, 0 };

// Bonus for being the side to move
const int Tempo = 15;

// Misc bonuses and maluses
const int PawnDoubled  = S(-13,-25);
const int PawnIsolated = S(-14,-18);
const int PawnSupport  = S( 13,  5);
const int PawnThreat   = S( 50, 30);
const int BishopPair   = S( 25,100);
const int KingAtkPawn  = S( 23, 71);

// Passed pawn [rank]
const int PawnPassed[8] = {
    S(  0,  0), S(-16, 23), S(-16, 25), S( -7, 56),
    S( 25, 81), S( 58,138), S(137,196), S(  0,  0),
};

// (Semi) open file for rook and queen [pt-4]
const int OpenFile[2]     = { S( 28, 10), S( -8,  6) };
const int SemiOpenFile[2] = { S(  9, 16), S(  1,  6) };

// KingLineDanger
const int KingLineDanger[28] = {
    S(  0,  0), S(  0,  0), S(  0,  0), S( -1, 17),
    S(-19, 14), S(-24,  4), S(-23,  1), S(-24,  6),
    S(-28,  7), S(-37,  8), S(-30,  5), S(-38, 12),
    S(-40, 11), S(-43, 12), S(-49, 12), S(-50, 13),
    S(-54, 10), S(-57,  5), S(-62,  1), S(-69, -7),
    S(-77, -9), S(-83,-19), S(-91,-28), S(-98,-43),
    S(-104,-42), S(-109,-36), S(-113,-28), S(-118,-30),
};

// Mobility [pt-2][mobility]
const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-57,-54), S(-24,-68), S( -5,-22), S(  6, 15), S( 14, 30), S( 18, 49), S( 26, 53), S( 37, 48),
      S( 53, 28) },
    // Bishop (0-13)
    { S(-54,-98), S(-19,-94), S( -1,-37), S(  6, -4), S( 14, 14), S( 21, 38), S( 25, 54), S( 24, 61),
      S( 23, 70), S( 28, 70), S( 31, 68), S( 58, 53), S( 59, 68), S( 49, 50) },
    // Rook (0-14)
    { S(-59,-69), S(-28,-60), S(-13,-36), S(-12,-17), S( -5, 15), S( -3, 36), S( -3, 55), S(  2, 57),
      S(  9, 61), S( 16, 66), S( 25, 71), S( 29, 71), S( 30, 73), S( 43, 62), S( 89, 35) },
    // Queen (0-27)
    { S(-62,-48), S(-71,-36), S(-67,-49), S(-44,-50), S(-25,-47), S( -7,-45), S(  4,-37), S( 10,-23),
      S( 15, -7), S( 19, 10), S( 22, 25), S( 25, 35), S( 28, 40), S( 28, 50), S( 29, 56), S( 29, 64),
      S( 26, 70), S( 26, 71), S( 24, 75), S( 28, 73), S( 33, 74), S( 50, 63), S( 59, 68), S( 78, 65),
      S(105, 84), S(112, 84), S(104,111), S(108,131) }
};

// KingSafety [pt-2]
const uint16_t AttackPower[4] = {  35, 20, 40, 80 };
const uint16_t CheckPower[4]  = { 100, 35, 65, 65 };
const uint16_t CountModifier[8] = { 0, 0, 50, 75, 80, 88, 95, 100 };


// Evaluates pawns
INLINE int EvalPawns(const Position *pos, const Color color) {

    int eval = 0, count;

    Bitboard pawns = colorPieceBB(color, PAWN);

    // Doubled pawns (only when one is blocking the other from moving)
    count = PopCount(pawns & ShiftBB(pawns, NORTH));
    eval += PawnDoubled * count;
    TraceCount(PawnDoubled);

    // Supported pawns
    count = PopCount(pawns & PawnBBAttackBB(pawns, color));
    eval += PawnSupport * count;
    TraceCount(PawnSupport);

    // Evaluate each individual pawn
    while (pawns) {

        Square sq = PopLsb(&pawns);

        TraceIncr(PieceValue[PAWN-1]);
        TraceIncr(PSQT[PAWN-1][RelativeSquare(color, sq)]);

        // Isolated pawns
        if (!(IsolatedMask[sq] & colorPieceBB(color, PAWN))) {
            eval += PawnIsolated;
            TraceIncr(PawnIsolated);
        }

        // Passed pawns
        if (!((PassedMask[color][sq]) & colorPieceBB(!color, PAWN))) {
            eval += PawnPassed[RelativeRank(color, RankOf(sq))];
            TraceIncr(PawnPassed[RelativeRank(color, RankOf(sq))]);
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
        TraceIncr(BishopPair);
    }

    // Evaluate each individual piece
    while (pieces) {

        Square sq = PopLsb(&pieces);

        TraceIncr(PieceValue[pt-1]);
        TraceIncr(PSQT[pt-1][RelativeSquare(color, sq)]);

        // Mobility
        Bitboard mobilityBB = XRayAttackBB(pos, color, pt, sq) & ei->mobilityArea[color];
        int mob = PopCount(mobilityBB);
        eval += Mobility[pt-2][mob];
        TraceIncr(Mobility[pt-2][mob]);

        // Attacks for king safety calculations
        int kingAttack = PopCount(mobilityBB & ei->enemyKingZone[color]);

        int checks = PopCount(mobilityBB & AttackBB(pt, Lsb(colorPieceBB(!color, KING)), pieceBB(ALL)));

        if (kingAttack > 0 || checks > 0) {
            ei->attackCount[color]++;
            ei->attackPower[color] +=  kingAttack * AttackPower[pt-2]
                                     +     checks * CheckPower[pt-2];
        }

        if (pt == ROOK || pt == QUEEN) {

            // Open file
            if (!(pieceBB(PAWN) & FileBB[FileOf(sq)])) {
                eval += OpenFile[pt-4];
                TraceIncr(OpenFile[pt-4]);
            // Semi-open file
            } else if (!(colorPieceBB(color, PAWN) & FileBB[FileOf(sq)])) {
                eval += SemiOpenFile[pt-4];
                TraceIncr(SemiOpenFile[pt-4]);
            }
        }
    }

    return eval;
}

// Evaluates kings
INLINE int EvalKings(const Position *pos, EvalInfo *ei, const Color color) {

    int eval = 0;

    Square kingSq = Lsb(colorPieceBB(color, KING));

    TraceIncr(PSQT[KING-1][RelativeSquare(color, kingSq)]);

    // Open lines from the king
    Bitboard SafeLine = RankBB[RelativeRank(color, RANK_1)];
    int count = PopCount(~SafeLine & AttackBB(QUEEN, kingSq, colorBB(color) | pieceBB(PAWN)));
    eval += KingLineDanger[count];
    TraceIncr(KingLineDanger[count]);

    // Add to enemy's attack power based on open lines
    ei->attackPower[!color] += (count - 3) * 8;

    // King threatening a pawn
    if (AttackBB(KING, kingSq, 0) & colorPieceBB(!color, PAWN)) {
        eval += KingAtkPawn;
        TraceIncr(KingAtkPawn);
    }

    return eval;
}

// Evaluates threads
INLINE int EvalThreats(const Position *pos, const Color color) {

    int eval = 0;

    Bitboard ourPawns = colorPieceBB(color, PAWN);
    Bitboard theirNonPawns = colorBB(!color) ^ colorPieceBB(!color, PAWN);

    int count = PopCount(PawnBBAttackBB(ourPawns, color) & theirNonPawns);
    eval += PawnThreat * count;
    TraceCount(PawnThreat);

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
    int16_t safetyScore =  ei->attackPower[color]
                         * CountModifier[MIN(7, ei->attackCount[color])]
                         / 100;

    return S(safetyScore, 0);
}

// Initializes the eval info struct
INLINE void InitEvalInfo(const Position *pos, EvalInfo *ei, const Color color) {

    const Direction up   = (color == WHITE ? NORTH : SOUTH);
    const Direction down = (color == WHITE ? SOUTH : NORTH);

    Bitboard b, pawns = colorPieceBB(color, PAWN);

    // Mobility area is defined as any square not attacked by an enemy pawn, nor
    // occupied by our own pawn either on its starting square or blocked from advancing.
    b = pawns & (RankBB[RelativeRank(color, RANK_2)] | ShiftBB(pieceBB(ALL), down));
    ei->mobilityArea[color] = ~(b | PawnBBAttackBB(colorPieceBB(!color, PAWN), !color));

    // King Safety
    b = AttackBB(KING, Lsb(colorPieceBB(!color, KING)), 0);
    ei->enemyKingZone[color] = b | ShiftBB(b, up);

    ei->attackPower[color] = -30;
    ei->attackCount[color] = 0;
}

// Calculate a static evaluation of a position
int EvalPosition(const Position *pos, PawnCache pc) {

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
    eval +=  EvalSafety(WHITE, &ei)
           - EvalSafety(BLACK, &ei);

    // Evaluate threats
    eval +=  EvalThreats(pos, WHITE)
           - EvalThreats(pos, BLACK);

    TraceEval(eval);

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
