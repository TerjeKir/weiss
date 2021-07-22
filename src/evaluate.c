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
    Bitboard passedPawns;
    Bitboard mobilityArea[COLOR_NB];
    Bitboard kingZone[COLOR_NB];
    int16_t attackPower[COLOR_NB];
    int16_t attackCount[COLOR_NB];
} EvalInfo;


// Phase value for each piece [piecetype]
const int PhaseValue[TYPE_NB] = { 0, 0, 1, 1, 2, 4, 0, 0 };

// Piecetype values, combines with PSQTs [piecetype]
const int PieceTypeValue[TYPE_NB] = {
    0, S(P_MG, P_EG), S(N_MG, N_EG), S(B_MG, B_EG), S(R_MG, R_EG), S(Q_MG, Q_EG), 0, 0
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
const int PawnDoubled  = S(-13,-42);
const int PawnIsolated = S( -8,-16);
const int PawnSupport  = S( 19, 10);
const int PawnThreat   = S( 51, 70);
const int PushThreat   = S( 18,  2);
const int PawnOpen     = S(-14,-15);
const int BishopPair   = S( 25,117);
const int KingAtkPawn  = S( 38, 74);
const int OpenForward  = S( 25, 22);
const int SemiForward  = S(  8, 21);
const int NBBehindPawn = S(  6, 44);

// Passed pawn
const int PawnPassed[RANK_NB] = {
    S(  0,  0), S(-14, 30), S(-13, 37), S( -9, 75),
    S( 17, 99), S( 46,176), S(137,253), S(  0,  0),
};

// Passed pawn
const int PassedDistUs[RANK_NB] = {
    S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0),
    S( -9,-11), S(-11, -9), S(-11, -6), S(  0,  0),
};

// Passed pawn
const int PassedDistThem[RANK_NB] = {
    S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0),
    S( 11, 16), S( 12, 20), S( 12, 23), S(  0,  0),
};

// Pawn phalanx
const int PawnPhalanx[RANK_NB] = {
    S(  0,  0), S(  8, -1), S( 18,  6), S( 24, 30),
    S( 59,114), S(160,259), S(176,323), S(  0,  0),
};

// KingLineDanger
const int KingLineDanger[28] = {
    S(  0,  0), S(  0,  0), S( -4, 14), S(-13, 50),
    S(-28, 50), S(-32, 37), S(-30, 32), S(-37, 41),
    S(-41, 41), S(-52, 44), S(-47, 37), S(-61, 46),
    S(-63, 43), S(-72, 43), S(-77, 44), S(-73, 41),
    S(-79, 39), S(-75, 29), S(-77, 24), S(-81, 19),
    S(-85, 14), S(-101,  7), S(-101, -3), S(-128,-16),
    S(-118,-34), S(-119,-51), S(-118,-34), S(-123,-36),
};

// Mobility [pt-2][mobility]
const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-35,-139), S(-27,-29), S( -6, 33), S(  3, 66), S( 13, 76), S( 16, 94), S( 23, 94), S( 32, 88),
      S( 45, 62) },
    // Bishop (0-13)
    { S(-39,-100), S(-20,-61), S( -9, -3), S( -2, 34), S(  8, 50), S( 17, 76), S( 21, 94), S( 21,100),
      S( 20,111), S( 26,111), S( 30,109), S( 53, 93), S( 50,101), S(101, 55) },
    // Rook (0-14)
    { S(-102,-132), S(-22,-21), S( -5, 44), S( -4, 49), S( -3, 85), S(  2,101), S(  0,121), S(  7,122),
      S( 12,129), S( 20,134), S( 28,139), S( 30,142), S( 32,145), S( 44,136), S( 80,110) },
    // Queen (0-27)
    { S(-63,-48), S(-91,-52), S(-89,-103), S(-28,-127), S(  0,-88), S(  0, 12), S(  2, 70), S(  5,104),
      S(  9,128), S( 13,147), S( 15,162), S( 17,176), S( 20,180), S( 21,188), S( 22,193), S( 23,199),
      S( 22,206), S( 20,210), S( 17,216), S( 15,222), S( 22,217), S( 17,223), S( 34,209), S( 42,194),
      S( 96,148), S(107,130), S(118,130), S(104,134) }
};

// KingSafety [pt-2]
const int AttackPower[4] = {  35, 20, 40, 80 };
const int CheckPower[4]  = { 100, 35, 65, 65 };
const int CountModifier[8] = { 0, 0, 64, 96, 113, 120, 124, 128 };


// Evaluates pawns
INLINE int EvalPawns(const Position *pos, EvalInfo *ei, const Color color) {

    const Direction down = color == WHITE ? SOUTH : NORTH;

    int count, eval = 0;

    Bitboard pawns = colorPieceBB(color, PAWN);

    // Doubled pawns (only when one is blocking the other from moving)
    count = PopCount(pawns & ShiftBB(pawns, NORTH));
    eval += PawnDoubled * count;
    TraceCount(PawnDoubled);

    // Pawns defending pawns
    count = PopCount(pawns & PawnBBAttackBB(pawns, !color));
    eval += PawnSupport * count;
    TraceCount(PawnSupport);

    // Open pawns
    Bitboard open = ~Fill(colorPieceBB(!color, PAWN), down);
    count = PopCount(pawns & open & ~PawnBBAttackBB(pawns, color));
    eval += PawnOpen * count;
    TraceCount(PawnOpen);

    // Phalanx
    Bitboard phalanx = pawns & ShiftBB(pawns, WEST);
    while (phalanx) {
        int rank = RelativeRank(color, RankOf(PopLsb(&phalanx)));
        eval += PawnPhalanx[rank];
        TraceIncr(PawnPhalanx[rank]);
    }

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

            ei->passedPawns |= BB(sq);
        }
    }

    return eval;
}

// Tries to get pawn eval from cache, otherwise evaluates and saves
int ProbePawnCache(const Position *pos, EvalInfo *ei, PawnCache pc) {

    // Can't cache when tuning as full trace is needed
    if (TRACE) return EvalPawns(pos, ei, WHITE) - EvalPawns(pos, ei, BLACK);

    Key key = pos->pawnKey;
    PawnEntry *pe = &pc[key % PAWN_CACHE_SIZE];

    if (pe->key != key) {
        pe->key  = key;
        pe->eval = EvalPawns(pos, ei, WHITE) - EvalPawns(pos, ei, BLACK);
        pe->passedPawns = ei->passedPawns;
    }

    return ei->passedPawns = pe->passedPawns, pe->eval;
}

// Evaluates knights, bishops, rooks, or queens
INLINE int EvalPiece(const Position *pos, EvalInfo *ei, const Color color, const PieceType pt) {

    const Direction up   = color == WHITE ? NORTH : SOUTH;
    const Direction down = color == WHITE ? SOUTH : NORTH;

    int eval = 0;

    Bitboard pieces = colorPieceBB(color, pt);

    // Bishop pair
    if (pt == BISHOP && Multiple(pieces)) {
        eval += BishopPair;
        TraceIncr(BishopPair);
    }

    // Minor behind pawn
    if (pt == KNIGHT || pt == BISHOP) {
        int count = PopCount(pieces & ShiftBB(pieceBB(PAWN), down));
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
        int attacks = PopCount(mobilityBB & ei->kingZone[!color]);
        int checks  = PopCount(mobilityBB & AttackBB(pt, kingSq(!color), pieceBB(ALL)));

        if (attacks > 0 || checks > 0) {
            ei->attackCount[color]++;
            ei->attackPower[color] +=  attacks * AttackPower[pt-2]
                                     +  checks *  CheckPower[pt-2];
        }

        // Forward mobility for rooks
        if (pt == ROOK) {
            Bitboard forward = Fill(BB(sq), up);
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
    TraceDanger(danger / 128);

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

INLINE int EvalPassedPawns(const Position *pos, const EvalInfo *ei, const Color color) {

    int eval = 0, count;

    Bitboard passers = colorBB(color) & ei->passedPawns;

    while (passers) {

        Square sq = PopLsb(&passers);
        int rank = RelativeRank(color, RankOf(sq));
        if (rank < RANK_5) continue;

        count = Distance(sq, kingSq( color));
        eval += count * PassedDistUs[rank];
        TraceCount(PassedDistUs[rank]);

        count = Distance(sq, kingSq(!color));
        eval += count * PassedDistThem[rank];
        TraceCount(PassedDistThem[rank]);
    }

    return eval;
}

// Evaluates threats
INLINE int EvalThreats(const Position *pos, const Color color) {

    const Direction up = color == WHITE ? NORTH : SOUTH;

    int count, eval = 0;

    // Our pawns threatening their non-pawns
    Bitboard ourPawns = colorPieceBB(color, PAWN);
    Bitboard theirNonPawns = colorBB(!color) ^ colorPieceBB(!color, PAWN);

    count = PopCount(PawnBBAttackBB(ourPawns, color) & theirNonPawns);
    eval += PawnThreat * count;
    TraceCount(PawnThreat);

    // Our pawns that can step forward to threaten their non-pawns
    Bitboard pawnPushes = ShiftBB(ourPawns, up) & ~pieceBB(ALL);

    count = PopCount(PawnBBAttackBB(pawnPushes, color) & theirNonPawns);
    eval += PushThreat * count;
    TraceCount(PushThreat);

    return eval;
}

// Initializes the eval info struct
INLINE void InitEvalInfo(const Position *pos, EvalInfo *ei, const Color color) {

    const Direction down = (color == WHITE ? SOUTH : NORTH);

    Bitboard b, pawns = colorPieceBB(color, PAWN);

    // Mobility area is defined as any square not attacked by an enemy pawn, nor
    // occupied by our own pawn either on its starting square or blocked from advancing.
    b = pawns & (RankBB[RelativeRank(color, RANK_2)] | ShiftBB(pieceBB(ALL), down));
    ei->mobilityArea[color] = ~(b | PawnBBAttackBB(colorPieceBB(!color, PAWN), !color));

    // King Safety
    ei->kingZone[color] = AttackBB(KING, kingSq(color), 0);

    ei->attackPower[color] = -30;
    ei->attackCount[color] = 0;

    // Clear passed pawns, filled in during pawn eval
    ei->passedPawns = 0;
}

// Calculate scale factor to lower overall eval based on various features
int ScaleFactor(const Position *pos, const int eval) {

    // Scale down eval for opposite-colored bishops endgames
    if (  !pieceBB(QUEEN) && !pieceBB(ROOK) && !pieceBB(KNIGHT)
        && pos->nonPawnCount[WHITE] == 1
        && pos->nonPawnCount[BLACK] == 1
        && (Single(pieceBB(BISHOP) & BlackSquaresBB)))
        return 64;

    // Scale down eval the fewer pawns the stronger side has
    Color strong = eval > 0 ? WHITE : BLACK;
    int strongPawnCount = PopCount(colorPieceBB(strong, PAWN));
    int x = 8 - strongPawnCount;
    return 128 - x * x;
}

// Calculate a static evaluation of a position
int EvalPosition(const Position *pos, PawnCache pc) {

    EvalInfo ei;
    InitEvalInfo(pos, &ei, WHITE);
    InitEvalInfo(pos, &ei, BLACK);

    // Material (includes PSQT)
    int eval = pos->material + pos->trend;

    // Evaluate pawns
    eval += ProbePawnCache(pos, &ei, pc);

    // Evaluate pieces
    eval += EvalPieces(pos, &ei);

    // Evaluate passed pawns
    eval +=  EvalPassedPawns(pos, &ei, WHITE)
           - EvalPassedPawns(pos, &ei, BLACK);

    // Evaluate threats
    eval +=  EvalThreats(pos, WHITE)
           - EvalThreats(pos, BLACK);

    TraceEval(eval);

    // Adjust eval by scale factor
    int scale = ScaleFactor(pos, eval);
    TraceScale(scale);

    // Adjust score by phase
    eval =  ((MgScore(eval) * pos->phase)
          +  (EgScore(eval) * (MidGame - pos->phase)) * scale / 128)
          / MidGame;

    // Return the evaluation, negated if we are black
    return (sideToMove == WHITE ? eval : -eval) + Tempo;
}
