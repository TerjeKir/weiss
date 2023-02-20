/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2023 Terje Kirstihagen

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
#include "material.h"


typedef struct EvalInfo {
    Bitboard attackedBy[COLOR_NB][TYPE_NB];
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
const int PawnDoubled  = S(-11,-48);
const int PawnDoubled2 = S(-10,-25);
const int PawnIsolated = S( -8,-16);
const int PawnSupport  = S( 22, 17);
const int PawnThreat   = S( 80, 34);
const int PushThreat   = S( 25,  6);
const int PawnOpen     = S(-14,-19);
const int BishopPair   = S( 33,110);
const int KingAtkPawn  = S(-16, 45);
const int OpenForward  = S( 28, 31);
const int SemiForward  = S( 17, 15);
const int NBBehindPawn = S(  9, 32);
const int BishopBadP   = S( -1, -5);
const int Shelter      = S( 31,-12);

// Passed pawn
const int PawnPassed[RANK_NB] = {
    S(  0,  0), S(-14, 25), S(-19, 40), S(-72,115),
    S(-38,146), S( 60,175), S(311,218), S(  0,  0)
};
const int PassedDefended[RANK_NB] = {
    S(  0,  0), S(  0,  0), S(  3,-14), S(  3,-10),
    S(  0, 33), S( 32,103), S(158, 96), S(  0,  0)
};
const int PassedBlocked[4] = {
    S(  1, -4), S( -6,  6), S(-11,-11), S(-54,-52)
};
const int PassedFreeAdv[4] = {
    S( -4, 25), S(-12, 58), S(-23,181), S(-70,277)
};
const int PassedDistUs[4] = {
    S( 18,-32), S( 17,-44), S(  2,-49), S(-19,-37)
};
const int PassedDistThem = S( -4, 19);
const int PassedRookBack = S( 21, 46);
const int PassedSquare   = S(-26,422);

// Pawn phalanx
const int PawnPhalanx[RANK_NB] = {
    S(  0,  0), S( 10, -2), S( 20, 14), S( 35, 37),
    S( 72,121), S(231,367), S(168,394), S(  0,  0)
};

// Threats
const int ThreatByMinor[8] = {
    S(  0,  0), S(  0,  0), S( 27, 42), S( 39, 42),
    S( 67, 11), S( 59,-17), S(  0,  0), S(  0,  0)
};
const int ThreatByRook[8] = {
    S(  0,  0), S(  0,  0), S( 24, 43), S( 31, 45),
    S(-17, 47), S( 93,-73), S(  0,  0), S(  0,  0)
};

// KingLineDanger
const int KingLineDanger[28] = {
    S(  0,  0), S(  0,  0), S( 15,  0), S( 11, 21),
    S(-16, 35), S(-25, 30), S(-29, 29), S(-37, 38),
    S(-48, 41), S(-67, 43), S(-67, 40), S(-80, 43),
    S(-85, 43), S(-97, 44), S(-109, 46), S(-106, 41),
    S(-116, 41), S(-123, 37), S(-126, 34), S(-131, 29),
    S(-138, 28), S(-155, 26), S(-149, 23), S(-172,  9),
    S(-148, -8), S(-134,-26), S(-130,-32), S(-135,-34)
};

// Mobility [pt-2][mobility]
const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-44,-139), S(-31, -7), S(-10, 60), S(  0, 86), S( 13, 89), S( 22,102), S( 32,102), S( 43,101),
      S( 54, 81) },
    // Bishop (0-13)
    { S(-51,-81), S(-26,-40), S(-11, 19), S( -3, 53), S(  9, 65), S( 21, 83), S( 26, 98), S( 32,104),
      S( 32,114), S( 35,116), S( 41,115), S( 57,106), S( 50,115), S(100, 76) },
    // Rook (0-14)
    { S(-105,-146), S(-15, 18), S( -1, 82), S(  5, 88), S(  2,121), S(  6,133), S(  5,144), S( 12,146),
      S( 14,152), S( 19,157), S( 24,164), S( 23,171), S( 25,177), S( 36,177), S( 72,154) },
    // Queen (0-27)
    { S(-63,-48), S(-97,-54), S(-89,-107), S(-17,-127), S(  0,-52), S( -8, 72), S( -2,142), S( -2,184),
      S(  1,215), S(  5,230), S(  7,243), S(  9,254), S( 15,255), S( 15,268), S( 16,279), S( 18,283),
      S( 16,294), S( 16,302), S( 12,313), S( 13,321), S( 23,314), S( 22,318), S( 48,298), S( 58,279),
      S(122,221), S(135,193), S(146,166), S(125,162) }
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
    Bitboard pawnAttacks = PawnBBAttackBB(pawns, color);

    // Doubled pawns (one directly in front of the other)
    count = PopCount(pawns & ShiftBB(pawns, NORTH));
    eval += PawnDoubled * count;
    TraceCount(PawnDoubled);

    // Doubled pawns (one square between them)
    count = PopCount(pawns & ShiftBB(pawns, 2 * NORTH));
    eval += PawnDoubled2 * count;
    TraceCount(PawnDoubled2);

    // Pawns defending pawns
    count = PopCount(pawns & PawnBBAttackBB(pawns, !color));
    eval += PawnSupport * count;
    TraceCount(PawnSupport);

    // Open pawns
    Bitboard open = ~Fill(colorPieceBB(!color, PAWN), down);
    count = PopCount(pawns & open & ~pawnAttacks);
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

            if (BB(sq) & pawnAttacks) {
                eval += PassedDefended[rank];
                TraceIncr(PassedDefended[rank]);
            }

            ei->passedPawns |= BB(sq);
        }
    }

    return eval;
}

// Tries to get pawn eval from cache, otherwise evaluates and saves
static int ProbePawnCache(const Position *pos, EvalInfo *ei, PawnCache pc) {

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

    ei->attackedBy[color][pt] = 0;

    // Evaluate each individual piece
    while (pieces) {

        Square sq = PopLsb(&pieces);

        TraceIncr(PieceValue[pt-1]);
        TraceIncr(PSQT[pt-1][BlackRelativeSquare(color, sq)]);

        // Mobility
        Bitboard attackBB = XRayAttackBB(pos, color, pt, sq);
        Bitboard mobilityBB = attackBB & ei->mobilityArea[color];
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

        ei->attackedBy[color][pt]  |= attackBB;
        ei->attackedBy[color][ALL] |= attackBB;

        if (pt == BISHOP) {
            Bitboard bishopSquares   = (BB(sq) & BlackSquaresBB) ? BlackSquaresBB : ~BlackSquaresBB;
            Bitboard badPawns        = colorPieceBB(color, PAWN) & bishopSquares;
            Bitboard blockedBadPawns = ShiftBB(pieceBB(ALL), down) & colorPieceBB(color, PAWN) & ~(FILE_A | FILE_B | FILE_G | FILE_H);

            int count = PopCount(badPawns) * PopCount(blockedBadPawns);
            eval += count * BishopBadP;
            TraceCount(BishopBadP);
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

    Bitboard pawnsInFront = pieceBB(PAWN) & PassedMask[color][kingSq];
    Bitboard ourPawns = pawnsInFront & colorBB(color) & ~PawnBBAttackBB(colorPieceBB(!color, PAWN), !color);

    count = PopCount(ourPawns);
    eval += count * Shelter;
    TraceCount(Shelter);

    return eval;
}

// Evaluates all non-pawns
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

// Evaluates passed pawns
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
        Square promoSq = RelativeSquare(color, MakeSquare(RANK_8, file));

        if (rank < RANK_4) continue;

        // Square rule
        if (pos->nonPawnCount[!color] == 0 && Distance(sq, promoSq) < Distance(kingSq(!color), promoSq) - ((!color) == sideToMove)) {
            eval += PassedSquare;
            TraceIncr(PassedSquare);
        }

        // Distance to own king
        count = Distance(forward, kingSq(color));
        eval += count * PassedDistUs[r];
        TraceCount(PassedDistUs[r]);

        // Distance to enemy king
        count = (rank - RANK_3) * Distance(forward, kingSq(!color));
        eval += count * PassedDistThem;
        TraceCount(PassedDistThem);

        // Blocked from advancing
        if (pieceOn(forward)) {
            eval += PassedBlocked[r];
            TraceIncr(PassedBlocked[r]);

        // Free to advance
        } else if (!(BB(forward) & ei->attackedBy[!color][ALL])) {
            eval += PassedFreeAdv[r];
            TraceIncr(PassedFreeAdv[r]);
        }

        // Rook supporting from behind
        if (colorPieceBB(color, ROOK) & Fill(BB(sq), down)) {
            eval += PassedRookBack;
            TraceIncr(PassedRookBack);
        }
    }

    return eval;
}

// Evaluates threats
INLINE int EvalThreats(const Position *pos, const EvalInfo *ei, const Color color) {

    const Direction up = color == WHITE ? NORTH : SOUTH;

    int count, eval = 0;
    Bitboard threats;

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

    // Threats by minor pieces
    Bitboard targets = theirNonPawns & ~pieceBB(KING);
    threats = targets & (ei->attackedBy[color][KNIGHT] | ei->attackedBy[color][BISHOP]);
    while (threats) {
        Square sq = PopLsb(&threats);
        eval += ThreatByMinor[pieceTypeOn(sq)];
        TraceIncr(ThreatByMinor[pieceTypeOn(sq)]);
    }

    // Threats by rooks
    targets &= ~ei->attackedBy[!color][PAWN];
    threats = targets & ei->attackedBy[color][ROOK];
    while (threats) {
        Square sq = PopLsb(&threats);
        eval += ThreatByRook[pieceTypeOn(sq)];
        TraceIncr(ThreatByRook[pieceTypeOn(sq)]);
    }

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

    ei->attackedBy[color][KING] = AttackBB(KING, kingSq(color), 0);
    ei->attackedBy[color][PAWN] = PawnBBAttackBB(pawns, color);
    ei->attackedBy[color][ALL] = ei->attackedBy[color][KING] | ei->attackedBy[color][PAWN];
}

// Calculate scale factor to lower overall eval based on various features
static int ScaleFactor(const Position *pos, const int eval) {

    // Scale down eval the fewer pawns the stronger side has
    Color strong = eval > 0 ? WHITE : BLACK;
    Bitboard strongPawns = colorPieceBB(strong, PAWN);

    int strongPawnCount = PopCount(strongPawns);
    int x = 8 - strongPawnCount;
    int pawnScale = 128 - x * x;

    if (!(strongPawns & QueenSideBB) || !(strongPawns & KingSideBB))
        pawnScale -= 20;

    // Opposite-colored bishop
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

    Endgame *eg = &endgameTable[EndgameIndex(pos->materialKey)];

    if (eg->key == pos->materialKey && eg->evalFunc != NULL)
        return eg->evalFunc(pos, sideToMove);

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
    eval +=  EvalThreats(pos, &ei, WHITE)
           - EvalThreats(pos, &ei, BLACK);

    TraceEval(eval);

    // Adjust eval by scale factor
    int scale = ScaleFactor(pos, eval);
    TraceScale(scale);

    // Adjust score by phase
    eval = (  MgScore(eval) * pos->phase
            + EgScore(eval) * (MidGame - pos->phase) * scale / 128)
          / MidGame;

    // Return the evaluation, negated if we are black + tempo bonus
    return (sideToMove == WHITE ? eval : -eval) + Tempo;
}
