/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2022 Terje Kirstihagen

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
const int PawnDoubled  = S(-11,-49);
const int PawnIsolated = S( -8,-16);
const int PawnSupport  = S( 22, 17);
const int PawnThreat   = S( 82, 32);
const int PushThreat   = S( 27,  2);
const int PawnOpen     = S(-14,-18);
const int BishopPair   = S( 36,116);
const int KingAtkPawn  = S(-35, 53);
const int OpenForward  = S( 33, 29);
const int SemiForward  = S( 16, 19);
const int NBBehindPawn = S( 10, 35);

// Passed pawn
const int PawnPassed[RANK_NB] = {
    S(  0,  0), S(-15, 25), S(-19, 40), S(-74,121),
    S(-49,166), S( 76,237), S(319,276), S(  0,  0)
};
const int PassedDefended[RANK_NB] = {
    S(  0,  0), S(  0,  0), S(  3,-14), S(  4,-11),
    S(  2, 32), S( 39, 88), S(161, 77), S(  0,  0)
};
const int PassedBlocked[4] = {
    S(  6,-19), S(  3,-24), S(  5,-99), S(-46,-136)
};
const int PassedDistUs[4] = {
    S( 18,-32), S( 18,-45), S( -4,-47), S(-22,-37)
};
const int PassedDistThem = S( -5, 23);
const int PassedRookBack = S( 24, 38);
const int PassedSquare   = S( -8,343);

// Pawn phalanx
const int PawnPhalanx[RANK_NB] = {
    S(  0,  0), S( 11, -3), S( 19, 14), S( 35, 38),
    S( 74,119), S(225,327), S(169,361), S(  0,  0)
};

// KingLineDanger
const int KingLineDanger[28] = {
    S(  0,  0), S(  0,  0), S(  0,  0), S(  3, 29),
    S(-21, 38), S(-28, 33), S(-32, 31), S(-40, 39),
    S(-51, 42), S(-66, 42), S(-63, 38), S(-76, 41),
    S(-80, 40), S(-90, 41), S(-100, 43), S(-97, 37),
    S(-108, 38), S(-112, 33), S(-117, 31), S(-122, 26),
    S(-129, 25), S(-144, 21), S(-139, 18), S(-164,  4),
    S(-143,-12), S(-133,-29), S(-130,-32), S(-135,-34)
};

// Mobility [pt-2][mobility]
const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-35,-142), S(-28,-19), S( -7, 46), S(  3, 81), S( 13, 92), S( 16,111), S( 23,111), S( 32,106),
      S( 46, 77) },
    // Bishop (0-13)
    { S(-40,-93), S(-21,-54), S( -9,  6), S( -2, 44), S(  8, 61), S( 17, 89), S( 22,107), S( 22,114),
      S( 20,126), S( 26,126), S( 31,125), S( 55,107), S( 51,119), S(103, 71) },
    // Rook (0-14)
    { S(-106,-146), S(-22,  4), S( -5, 70), S( -3, 77), S( -3,113), S(  2,131), S(  0,151), S(  7,155),
      S( 12,162), S( 20,168), S( 28,174), S( 30,178), S( 31,182), S( 45,172), S( 86,140) },
    // Queen (0-27)
    { S(-63,-48), S(-96,-54), S(-91,-107), S(-21,-129), S(  3,-58), S( -1, 65), S( -1,135), S(  2,177),
      S(  5,204), S(  9,224), S( 11,241), S( 14,256), S( 17,260), S( 17,269), S( 18,276), S( 19,282),
      S( 17,290), S( 15,295), S( 11,302), S(  9,310), S( 17,303), S( 13,307), S( 38,287), S( 49,268),
      S(116,213), S(130,187), S(143,163), S(123,160) }
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
        TraceIncr(PSQT[PAWN-1][BlackRelativeSquare(color, sq)]);

        // Isolated pawns
        if (!(IsolatedMask[sq] & colorPieceBB(color, PAWN))) {
            eval += PawnIsolated;
            TraceIncr(PawnIsolated);
        }

        // Passed pawns
        if (!((PassedMask[color][sq]) & colorPieceBB(!color, PAWN))) {

            int rank = RelativeRank(color, RankOf(sq));

            eval += PawnPassed[rank];
            TraceIncr(PawnPassed[rank]);

            if (BB(sq) & PawnBBAttackBB(colorPieceBB(color, PAWN), color)) {
                eval += PassedDefended[rank];
                TraceIncr(PassedDefended[rank]);
            }

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
        TraceIncr(PSQT[pt-1][BlackRelativeSquare(color, sq)]);

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

    TraceIncr(PSQT[KING-1][BlackRelativeSquare(color, kingSq)]);

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
    TraceDanger(S(danger / 128, 0));

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

    const Direction up   = color == WHITE ? NORTH : SOUTH;
    const Direction down = color == WHITE ? SOUTH : NORTH;

    int eval = 0, count;

    Bitboard passers = colorBB(color) & ei->passedPawns;

    while (passers) {

        Square sq = PopLsb(&passers);
        Square forward = sq + up;
        int rank = RelativeRank(color, RankOf(sq));
        int file = FileOf(sq);
        int r = rank - RANK_4;

        if (rank < RANK_4) continue;

        Square promoSq = RelativeSquare(color, MakeSquare(RANK_8, file));
        if (pos->nonPawnCount[!color] == 0 && Distance(sq, promoSq) < Distance(kingSq(!color), promoSq) - ((!color) == sideToMove)) {
            eval += PassedSquare;
            TraceIncr(PassedSquare);
        }

        count = Distance(forward, kingSq( color));
        eval += count * PassedDistUs[r];
        TraceCount(PassedDistUs[r]);

        count = (rank - RANK_3) * Distance(forward, kingSq(!color));
        eval += count * PassedDistThem;
        TraceCount(PassedDistThem);

        if (pieceOn(forward)) {
            eval += PassedBlocked[r];
            TraceIncr(PassedBlocked[r]);
        }

        if (colorPieceBB(color, ROOK) & Fill(BB(sq), down)) {
            eval += PassedRookBack;
            TraceIncr(PassedRookBack);
        }
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

    const Direction down = color == WHITE ? SOUTH : NORTH;

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

    // Scale down eval the fewer pawns the stronger side has
    Color strong = eval > 0 ? WHITE : BLACK;
    Bitboard strongPawns = colorPieceBB(strong, PAWN);

    int strongPawnCount = PopCount(strongPawns);
    int x = 8 - strongPawnCount;
    int pawnScale = 128 - x * x;

    if (!(strongPawns & QueenSideBB) || !(strongPawns & KingSideBB))
        pawnScale -= 20;

    if (   pos->nonPawnCount[WHITE] <= 2
        && pos->nonPawnCount[BLACK] <= 2
        && pos->nonPawnCount[WHITE] == pos->nonPawnCount[BLACK]
        && Single(colorPieceBB(WHITE, BISHOP))
        && Single(colorPieceBB(BLACK, BISHOP))
        && Single(pieceBB(BISHOP) & BlackSquaresBB))
        return MIN((pos->nonPawnCount[WHITE] == 1 ? 64 : 96), pawnScale);

    return pawnScale;
}

// Calculate a static evaluation of a position
int EvalPosition(const Position *pos, PawnCache pc) {

    EvalInfo ei;
    InitEvalInfo(pos, &ei, WHITE);
    InitEvalInfo(pos, &ei, BLACK);

    // Material (includes PSQT) + trend
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
    eval =  (MgScore(eval) * pos->phase
          +  EgScore(eval) * (MidGame - pos->phase) * scale / 128)
          / MidGame;

    // Return the evaluation, negated if we are black
    return (sideToMove == WHITE ? eval : -eval) + Tempo;
}
