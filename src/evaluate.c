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


// Phase value for each piece [piece]
const int PhaseValue[TYPE_NB] = { 0, 0, 1, 1, 2, 4, 0, 0 };

// Piecetype values, combines with PSQTs [piecetype]
const int PieceTypeValue[7] = {
    0, S(P_MG, P_EG), S(N_MG, N_EG), S(B_MG, B_EG), S(R_MG, R_EG), S(Q_MG, Q_EG), 0
};

// Phase piece values, lookup used for futility pruning [phase][piece]
const int PieceValue[2][PIECE_NB] = {
    { 0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0,
      0, P_MG, N_MG, B_MG, R_MG, Q_MG, 0, 0 },
    { 0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0,
      0, P_EG, N_EG, B_EG, R_EG, Q_EG, 0, 0 }
};

// Bonus for being the side to move
const int Tempo = 15;

// Misc bonuses and maluses
const int PawnDoubled  = S(-18,-31);
const int PawnIsolated = S(-14,-17);
const int PawnSupport  = S( 12,  4);
const int PawnThreat   = S( 45, 66);
const int PawnOpen     = S(-13, -8);
const int BishopPair   = S( 26,102);
const int KingAtkPawn  = S( 56, 81);
const int OpenForward  = S( 27, 11);
const int SemiForward  = S(  7, 16);
const int NBBehindPawn = S( 13, 31);

// Passed pawn [rank]
const int PawnPassed[8] = {
    S(  0,  0), S(-15, 23), S(-18, 27), S(-11, 61),
    S( 23, 91), S( 48,159), S(154,216), S(  0,  0),
};

// KingLineDanger
const int KingLineDanger[28] = {
    S(  0,  0), S(  0,  0), S(  0,  0), S(-10, 44),
    S(-28, 42), S(-34, 31), S(-33, 28), S(-35, 32),
    S(-39, 32), S(-50, 35), S(-44, 29), S(-55, 36),
    S(-55, 32), S(-57, 32), S(-58, 31), S(-50, 27),
    S(-42, 22), S(-32, 14), S(-30,  9), S(-35,  4),
    S(-42,  2), S(-49, -6), S(-59,-13), S(-73,-25),
    S(-92,-35), S(-106,-41), S(-107,-43), S(-112,-45),
};

// Mobility [pt-2][mobility]
const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-42,-73), S(-24,-71), S( -5,-13), S(  6, 22), S( 16, 35), S( 19, 54), S( 28, 55), S( 38, 50),
      S( 55, 27) },
    // Bishop (0-13)
    { S(-53,-130), S(-18,-102), S( -3,-30), S(  5,  5), S( 14, 23), S( 22, 47), S( 26, 63), S( 24, 70),
      S( 23, 79), S( 28, 79), S( 29, 78), S( 54, 62), S( 61, 68), S( 70, 44) },
    // Rook (0-14)
    { S(-67,-77), S(-23,-85), S( -6,-37), S( -8,-14), S( -1, 21), S(  1, 42), S(  1, 61), S(  6, 63),
      S( 13, 68), S( 21, 72), S( 29, 77), S( 33, 78), S( 36, 80), S( 50, 69), S(106, 36) },
    // Queen (0-27)
    { S(-62,-48), S(-83,-42), S(-73,-67), S(-29,-69), S( -8,-68), S( 10,-62), S( 16,-36), S( 21, -6),
      S( 25, 14), S( 29, 33), S( 31, 49), S( 34, 59), S( 37, 64), S( 37, 72), S( 37, 77), S( 37, 83),
      S( 34, 89), S( 30, 94), S( 26, 98), S( 25, 97), S( 28, 94), S( 34, 82), S( 44, 73), S( 61, 57),
      S( 82, 61), S( 97, 65), S( 98,103), S(105,127) }
};

// KingSafety [pt-2]
const int AttackPower[4] = {  35, 20, 40, 80 };
const int CheckPower[4]  = { 100, 35, 65, 65 };
const int CountModifier[8] = { 0, 0, 64, 96, 113, 120, 124, 128 };


// Evaluates pawns
INLINE int EvalPawns(const Position *pos, const Color color) {

    const Direction down = color == WHITE ? SOUTH : NORTH;

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

    // Open pawns
    Bitboard open = ~Fill(colorPieceBB(!color, PAWN), down);
    count = PopCount(pawns & open & ~PawnBBAttackBB(pawns, color));
    eval += PawnOpen * count;
    TraceCount(PawnOpen);

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

    // Minor behind pawn
    if (pt == KNIGHT || pt == BISHOP) {
        int count = PopCount(pieces & ShiftBB(pieceBB(PAWN), color == WHITE ? SOUTH : NORTH));
        eval += count * NBBehindPawn;
        TraceCount(NBBehindPawn);
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
        int attacks = PopCount(mobilityBB & ei->enemyKingZone[color]);
        int checks  = PopCount(mobilityBB & AttackBB(pt, kingSq(!color), pieceBB(ALL)));

        if (attacks > 0 || checks > 0) {
            ei->attackCount[color]++;
            ei->attackPower[color] +=  attacks * AttackPower[pt-2]
                                     +  checks *  CheckPower[pt-2];
        }

        // Forward mobility for rooks
        if (pt == ROOK) {
            Bitboard forward = Fill(BB(sq), color == WHITE ? NORTH : SOUTH);
            if (!(forward & pieceBB(PAWN))) {
                eval += OpenForward;
                TraceIncr(OpenForward);
            } else if (!(forward & colorPieceBB(color, PAWN))) {
                eval += SemiForward;
                TraceIncr(SemiForward);
            }
        }
    }

    return eval;
}

// Evaluates kings
INLINE int EvalKings(const Position *pos, EvalInfo *ei, const Color color) {

    int eval = 0;

    Square kingSq = kingSq(color);

    TraceIncr(PSQT[KING-1][RelativeSquare(color, kingSq)]);

    // Open lines from the king
    Bitboard SafeLine = RankBB[RelativeRank(color, RANK_1)];
    int count = PopCount(~SafeLine & AttackBB(QUEEN, kingSq, colorBB(color) | pieceBB(PAWN)));
    eval += KingLineDanger[count];
    TraceIncr(KingLineDanger[count]);

    // King threatening a pawn
    if (AttackBB(KING, kingSq, 0) & colorPieceBB(!color, PAWN)) {
        eval += KingAtkPawn;
        TraceIncr(KingAtkPawn);
    }

    // King safety
    ei->attackPower[!color] += (count - 3) * 8;

    int danger =  ei->attackPower[!color]
                * CountModifier[MIN(7, ei->attackCount[!color])];

    eval -= S(danger / 128, 0);

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

// Evaluates threats
INLINE int EvalThreats(const Position *pos, const Color color) {

    int eval = 0;

    Bitboard ourPawns = colorPieceBB(color, PAWN);
    Bitboard theirNonPawns = colorBB(!color) ^ colorPieceBB(!color, PAWN);

    int count = PopCount(PawnBBAttackBB(ourPawns, color) & theirNonPawns);
    eval += PawnThreat * count;
    TraceCount(PawnThreat);

    return eval;
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
    b = AttackBB(KING, kingSq(!color), 0);
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
