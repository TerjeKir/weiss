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
    Bitboard mobilityArea[2];
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
const int PieceValue[2][PIECE_NB] = {
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
const int PawnDoubled    = S(-11,-26);
const int PawnIsolated   = S(-14,-17);
const int PawnSupport    = S( 13,  6);
const int BishopPair     = S( 19, 97);
const int KingLineDanger = S( -4, -2);

// Passed pawn [rank]
const int PawnPassed[8] = {
    S(  0,  0), S( -8, 19), S(-12, 23), S( -7, 52),
    S( 24, 74), S( 59,134), S(131,186), S(  0,  0),
};

// (Semi) open file for rook and queen [pt-4]
const int OpenFile[2]     = { S( 39, 12), S(  0,  2) };
const int SemiOpenFile[2] = { S( 16, 14), S(  3,  3) };

// Mobility [pt-2][mobility]
const int Mobility[4][28] = {
    // Knight (0-8)
    { S(-68,-53), S(-26,-62), S( -3,-29), S(  6,  7), S( 17, 23), S( 21, 46), S( 29, 51), S( 38, 50),
      S( 53, 32) },
    // Bishop (0-13)
    { S(-49,-77), S(-16,-78), S( -7,-42), S(  0,-11), S( 10,  6), S( 18, 31), S( 22, 48), S( 23, 57),
      S( 23, 64), S( 30, 66), S( 42, 61), S( 63, 50), S( 60, 72), S( 47, 54) },
    // Rook (0-14)
    { S(-59,-69), S(-23,-47), S(-10,-27), S(-11, -5), S( -7, 11), S( -5, 29), S( -3, 44), S(  2, 50),
      S(  7, 56), S( 13, 61), S( 20, 67), S( 26, 68), S( 35, 68), S( 53, 56), S( 77, 36) },
    // Queen (0-27)
    { S(-62,-48), S(-67,-36), S(-56,-46), S(-44,-47), S(-33,-43), S(-16,-42), S( -8,-36), S( -2,-24),
      S(  3, -7), S(  9,  6), S( 14, 19), S( 19, 27), S( 23, 32), S( 26, 42), S( 28, 50), S( 28, 59),
      S( 30, 64), S( 33, 61), S( 32, 70), S( 37, 69), S( 42, 76), S( 61, 68), S( 67, 74), S( 86, 70),
      S(110, 91), S(114, 87), S(104,111), S(108,131) }
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

INLINE Bitboard GetScanning(const Position *pos, const Color color, const PieceType pt){
    Bitboard occupied = pieceBB(ALL);
    switch(pt) {
        // Bishop scans through Bishops and Queens        
        case BISHOP: return occupied ^ colorPieceBB(color, BISHOP) ^ colorPieceBB(color, QUEEN);
        // Rooks scan through Rooks and Queens
        case ROOK  : return occupied ^ colorPieceBB(color, ROOK) ^ colorPieceBB(color, QUEEN);
        // Queens scan through Queens, Rooks and Bishops
        case QUEEN : return occupied ^ colorPieceBB(color, ROOK) ^ colorPieceBB(color, QUEEN) ^ colorPieceBB(color, BISHOP);
        default    : return occupied;
    }
}

// Evaluates knights, bishops, rooks, or queens
INLINE int EvalPiece(const Position *pos, const EvalInfo *ei, const Color color, const PieceType pt) {

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
        Bitboard occupied = GetScanning(pos, color, pt);
        int mob = PopCount(AttackBB(pt, sq, occupied) & ei->mobilityArea[color]);
        eval += Mobility[pt-2][mob];
        if (TRACE) T.Mobility[pt-2][mob][color]++;

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

INLINE int EvalPieces(const Position *pos, const EvalInfo *ei) {
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

// Initializes the eval info struct
INLINE void InitEvalInfo(const Position *pos, EvalInfo *ei, const Color color) {

    const Direction down = (color == WHITE ? SOUTH : NORTH);

    Bitboard b = RankBB[RelativeRank(color, RANK_2)] | ShiftBB(down, pieceBB(ALL));

    b &= colorPieceBB(color, PAWN);

    // Mobility area is defined as any square not attacked by an enemy pawn,
    // nor occupied by our own pawn on its starting square or blocked from advancing.
    ei->mobilityArea[color] = ~(b | PawnBBAttackBB(colorPieceBB(!color, PAWN), !color));
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
