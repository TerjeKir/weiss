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

#include "../types.h"


#ifdef TUNE
    #define TRACE (1)
#else
    #define TRACE (0)
#endif

#define DATASET      "../../Datasets/Andrew/BIG.book"
#define NPOSITIONS   (22494809) // Total FENS in the book
#define BATCHSIZE    (22494809) // FENs per mini-batch

#define NTERMS       (     472) // Number of terms being tuned
#define MAXEPOCHS    (   10000) // Max number of epochs allowed
#define REPORTING    (      50) // How often to print the new parameters
#define NPARTITIONS  (      64) // Total thread partitions
#define KPRECISION   (      10) // Iterations for computing K
#define LRRATE       (    1.00) // Global Learning rate
#define LRDROPRATE   (    1.00) // Cut LR by this each LR-step
#define LRSTEPRATE   (     250) // Cut LR after this many epochs

#define STACKSIZE ((int)((double) NPOSITIONS * NTERMS / 64))


typedef struct EvalTrace {

    int eval;

    int PieceValue[5][COLOR_NB];
    int PSQT[6][64][COLOR_NB];
    int Mobility[4][28][COLOR_NB];

    int PawnDoubled[COLOR_NB];
    int PawnIsolated[COLOR_NB];
    int PawnSupport[COLOR_NB];
    int BishopPair[COLOR_NB];
    int KingLineDanger[COLOR_NB];

    int PawnPassed[8][COLOR_NB];

    int OpenFile[2][COLOR_NB];
    int SemiOpenFile[2][COLOR_NB];

} EvalTrace;

#ifdef TUNE

typedef struct TTuple {
    int16_t index, coeff;
} TTuple;

typedef struct TEntry {
    int16_t seval, phase, turn, ntuples;
    int eval;
    double result, pfactors[2];
    TTuple *tuples;
} TEntry;

typedef double TCoeffs[NTERMS];
typedef double TVector[NTERMS][2];

// Runs the tuner
void Tune();

#endif
