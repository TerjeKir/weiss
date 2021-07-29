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


/*
  Gradient Decent Tuning for Chess Engines as described by Andrew Grant
  in his paper: Evaluation & Tuning in Chess Engines:

  https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf
*/

#pragma once

#ifdef TUNE

#include "../types.h"


#define TRACE (1)
#define TraceCount(term) T.term[color] += count
#define TraceIncr(term) T.term[color]++
#define TraceScale(s) T.scale = s
#define TraceEval(e) T.eval = e
#define TraceDanger(d) T.danger[color] = d


// #define DATASET      "../../Datasets/Andrew/BIG.book"
// #define NPOSITIONS   ( 42484641) // Total FENS in the book

// #define DATASET      "../../Datasets/lichess-big3-resolved.book"
// #define NPOSITIONS   ( 7153652) // Total FENS in the book

#define DATASET      "../../Datasets/Andrew/COMBO.book"
#define NPOSITIONS   (14669229) // Total FENS in the book


#define NTERMS       (     528) // Number of terms being tuned
#define MAXEPOCHS    (   10000) // Max number of epochs allowed
#define REPORTING    (      50) // How often to print the new parameters
#define NPARTITIONS  (      64) // Total thread partitions
#define LRRATE       (    0.01) // Learning rate
#define LRDROPRATE   (    1.00) // Cut LR by this each LR-step
#define LRSTEPRATE   (     250) // Cut LR after this many epochs
#define BETA_1       (     0.9) // ADAM Momemtum Coefficient
#define BETA_2       (   0.999) // ADAM Velocity Coefficient

#define STACKSIZE ((int)((double) NPOSITIONS * NTERMS / 64))


typedef struct EvalTrace {

    int eval;
    int scale;
    int danger[COLOR_NB];

    int PieceValue[5][COLOR_NB];
    int PSQT[6][64][COLOR_NB];

    int PawnDoubled[COLOR_NB];
    int PawnIsolated[COLOR_NB];
    int PawnSupport[COLOR_NB];
    int PawnThreat[COLOR_NB];
    int PushThreat[COLOR_NB];
    int PawnOpen[COLOR_NB];
    int BishopPair[COLOR_NB];
    int KingAtkPawn[COLOR_NB];
    int OpenForward[COLOR_NB];
    int SemiForward[COLOR_NB];
    int NBBehindPawn[COLOR_NB];

    int PawnPassed[RANK_NB][COLOR_NB];
    int PassedDefended[RANK_NB][COLOR_NB];
    int PassedBlocked[RANK_NB][COLOR_NB];
    int PassedDistUs[RANK_NB][COLOR_NB];
    int PassedDistThem[COLOR_NB];
    int PassedRookBack[COLOR_NB];
    int PawnPhalanx[RANK_NB][COLOR_NB];
    int KingLineDanger[28][COLOR_NB];
    int Mobility[4][28][COLOR_NB];

} EvalTrace;

typedef struct TTuple {
    int16_t index, coeff;
} TTuple;

typedef struct TEntry {
    int16_t seval, phase, turn, ntuples;
    int eval;
    double result, scale, pfactors[2];
    TTuple *tuples;
} TEntry;

typedef double TCoeffs[NTERMS];
typedef double TVector[NTERMS][2];
typedef int TIntVector[NTERMS][2];


extern EvalTrace T;


// Runs the tuner
void Tune();

#else
#define TRACE (0)
#define TraceCount(term)
#define TraceIncr(term)
#define TraceScale(s)
#define TraceEval(e)
#define TraceDanger(d)
#endif
