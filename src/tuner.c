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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "evaluate.h"
#include "psqt.h"
#include "tuner.h"


// Piece values
extern int PieceTypeValue[7];
extern int PieceValue[2][PIECE_NB];

// PSQT values
extern int PieceSqValue[7][64];

// Misc
extern int PawnDoubled;
extern int PawnIsolated;
extern int PawnSupport;
extern int BishopPair;
extern int KingLineDanger;

// Passed pawn [rank]
extern int PawnPassed[8];

// (Semi) open file for rook and queen [pt-4]
extern int OpenFile[2];
extern int SemiOpenFile[2];

// Mobility
extern int Mobility[4][28];


EvalTrace T, EmptyTrace;


void print_0(char *name, TVector params, int i, char *S) {

    printf("const int %s%s = S(%3d,%3d);\n", name, S, (int) params[i][MG], (int) params[i][EG]);
}

void print_1(char *name, TVector params, int i, int A, char *S) {

    printf("const int %s%s = { ", name, S);

    if (A >= 8) {
        for (int a = 0; a < A; a++, i++) {
            if (a % 4 == 0) printf("\n    ");
            printf("S(%3d,%3d), ", (int) params[i][MG], (int) params[i][EG]);
        }
        printf("\n};\n");
    }

    else {
        for (int a = 0; a < A; a++, i++) {
            printf("S(%3d,%3d)", (int) params[i][MG], (int) params[i][EG]);
            if (a != A - 1) printf(", "); else printf(" };\n");
        }
    }
}

void print_2(char *name, TVector params, int i, int A, int B, char *S) {

    printf("const int %s%s = {\n", name, S);

    for (int a = 0; a < A; a++) {

        printf("   {");

        for (int b = 0; b < B; b++, i++) {
            if (b && b % 8 == 0) printf("\n    ");
            printf("S(%3d,%3d)", (int) params[i][MG], (int) params[i][EG]);
            printf("%s", b == B - 1 ? "" : ", ");
        }
        printf("},\n");
    }
    printf("};\n\n");
}

// Print the PSQTs from black's pov
void PrintPSQT(TVector params, int i) {

    printf("const int PieceSqValue[7][64] = {\n    { 0 },\n");

    for (int pt = 0; pt < 6; pt++) {

        printf("    {\n    ");

        for (int sq = 0; sq < 64; sq++) {
            if (sq && sq % 8 == 0) printf("\n    ");
            printf("S(%3d,%3d)", (int) params[i+MirrorSquare(sq)][MG],
                                 (int) params[i+MirrorSquare(sq)][EG]);
            printf("%s", sq == 64 - 1 ? "" : ", ");
        }
        printf("},\n\n");
        i += 64;
    }
    printf("};\n\n");
}

void InitBaseParams(TVector tparams) {

    int i = 0;

    // Piece values
    for (int pt = PAWN; pt <= QUEEN; ++pt) {
        tparams[i][MG] = PieceValue[MG][pt];
        tparams[i][EG] = PieceValue[EG][pt];
        i++;
    }

    // PSQT
    for (int pt = PAWN; pt <= KING; ++pt) {
        for (int sq = 0; sq < 64; ++sq) {
            tparams[i][MG] = -MgScore(PSQT[pt][MirrorSquare(sq)]) - PieceValue[MG][pt];
            tparams[i][EG] = -EgScore(PSQT[pt][MirrorSquare(sq)]) - PieceValue[EG][pt];
            i++;
        }
    }

    // Mobility
    for (int pt = KNIGHT; pt <= QUEEN; ++pt) {
        for (int mob = 0; mob < 28; ++mob) {
            tparams[i][MG] = MgScore(Mobility[pt-2][mob]);
            tparams[i][EG] = EgScore(Mobility[pt-2][mob]);
            i++;
        }
    }

    // Pawn stuff
    tparams[i][MG] = MgScore(PawnDoubled);
    tparams[i][EG] = EgScore(PawnDoubled);
    i++;

    tparams[i][MG] = MgScore(PawnIsolated);
    tparams[i][EG] = EgScore(PawnIsolated);
    i++;

    tparams[i][MG] = MgScore(PawnSupport);
    tparams[i][EG] = EgScore(PawnSupport);
    i++;

    for (int j = 0; j < 8; ++j) {
        tparams[i][MG] = MgScore(PawnPassed[j]);
        tparams[i][EG] = EgScore(PawnPassed[j]);
        i++;
    }

    // Misc
    tparams[i][MG] = MgScore(BishopPair);
    tparams[i][EG] = EgScore(BishopPair);
    i++;

    for (int j = 0; j < 2; ++j) {
        tparams[i][MG] = MgScore(OpenFile[j]);
        tparams[i][EG] = EgScore(OpenFile[j]);
        i++;
    }

    for (int j = 0; j < 2; ++j) {
        tparams[i][MG] = MgScore(SemiOpenFile[j]);
        tparams[i][EG] = EgScore(SemiOpenFile[j]);
        i++;
    }

    tparams[i][MG] = MgScore(KingLineDanger);
    tparams[i][EG] = EgScore(KingLineDanger);
    i++;

    if (i != NTERMS) {
        printf("Error 1 in printParameters(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void printParameters(TVector params, TVector current) {

    TVector tparams;

    for (int i = 0; i < NTERMS; ++i) {
        tparams[i][MG] = round(params[i][MG] + current[i][MG]);
        tparams[i][EG] = round(params[i][EG] + current[i][EG]);
    }

    int i = 0;
    puts("");

    // Piece values
    print_1("PieceValue", tparams, i, 5, "[5]");
    i+=5;

    // PSQT
    PrintPSQT(tparams, i);
    i+=6*64;

    // Mobility
    print_2("Mobility", tparams, i, 4, 28, "[4][28]");
    i+=4*28;

    // Pawn stuff
    print_0("PawnDoubled", tparams, i, " ");
    i++;
    print_0("PawnIsolated", tparams, i, "");
    i++;
    print_0("PawnSupport", tparams, i, " ");
    i++;
    print_1("PawnPassed", tparams, i, 8, "[8]");
    i+=8;

    // Misc
    print_0("BishopPair", tparams, i, "");
    i++;
    print_1("OpenFile", tparams, i, 2, "[2]    ");
    i+=2;
    print_1("SemiOpenFile", tparams, i, 2, "[2]");
    i+=2;
    print_0("KingLineDanger", tparams, i, "");
    i++;
    puts("");

    if (i != NTERMS) {
        printf("Error 2 in printParameters(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void initCoefficients(TVector coeffs) {

    int i = 0;

    for (int pt = PAWN-1; pt <= QUEEN-1; ++pt) {
        coeffs[i  ][WHITE] = T.PieceValue[pt][WHITE];
        coeffs[i++][BLACK] = T.PieceValue[pt][BLACK];
    }

    for (int pt = PAWN; pt <= KING; ++pt) {
        for (int sq = 0; sq < 64; ++sq) {
            coeffs[i  ][WHITE] = T.PSQT[pt-1][sq][WHITE];
            coeffs[i++][BLACK] = T.PSQT[pt-1][sq][BLACK];
        }
    }

    for (int pt = KNIGHT-2; pt <= QUEEN-2; ++pt) {
        for (int mob = 0; mob < 28; ++mob) {
            coeffs[i  ][WHITE] = T.Mobility[pt][mob][WHITE];
            coeffs[i++][BLACK] = T.Mobility[pt][mob][BLACK];
        }
    }

    coeffs[i  ][WHITE] = T.PawnDoubled[WHITE];
    coeffs[i++][BLACK] = T.PawnDoubled[BLACK];

    coeffs[i  ][WHITE] = T.PawnIsolated[WHITE];
    coeffs[i++][BLACK] = T.PawnIsolated[BLACK];

    coeffs[i  ][WHITE] = T.PawnSupport[WHITE];
    coeffs[i++][BLACK] = T.PawnSupport[BLACK];

    for (int j = 0; j < 8; ++j) {
        coeffs[i  ][WHITE] = T.PawnPassed[j][WHITE];
        coeffs[i++][BLACK] = T.PawnPassed[j][BLACK];
    }

    coeffs[i  ][WHITE] = T.BishopPair[WHITE];
    coeffs[i++][BLACK] = T.BishopPair[BLACK];

    for (int j = 0; j < 2; ++j) {
        coeffs[i  ][WHITE] = T.OpenFile[j][WHITE];
        coeffs[i++][BLACK] = T.OpenFile[j][BLACK];
    }

    for (int j = 0; j < 2; ++j) {
        coeffs[i  ][WHITE] = T.SemiOpenFile[j][WHITE];
        coeffs[i++][BLACK] = T.SemiOpenFile[j][BLACK];
    }

    coeffs[i  ][WHITE] = T.KingLineDanger[WHITE];
    coeffs[i++][BLACK] = T.KingLineDanger[BLACK];

    if (i != NTERMS){
        printf("Error in initCoefficients(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void initTunerTuples(TEntry *entry, TVector coeffs) {

    int length = 0, tidx = 0;

    // Count the needed Coefficients
    for (int i = 0; i < NTERMS; i++)
        length += coeffs[i][WHITE] - coeffs[i][BLACK] != 0.0;

    // Allocate space for new Tuples (Normally, we don’t malloc())
    entry->tuples  = malloc(sizeof(TTuple) * length);
    entry->ntuples = length;

    // Finally setup each of our TTuples for this TEntry
    for (int i = 0; i < NTERMS; i++)
        if (coeffs[i][WHITE] - coeffs[i][BLACK] != 0.0)
            entry->tuples[tidx++] = (TTuple) { i, coeffs[i][WHITE], coeffs[i][BLACK] };
}

void initTunerEntry(TEntry *entry, Position *pos) {

    // Save time by computing phase scalars now
    entry->pfactors[MG] = 0 + pos->phaseValue / 24.0;
    entry->pfactors[EG] = 1 - pos->phaseValue / 24.0;
    entry->phase = pos->phase;

    // Save a white POV static evaluation
    TVector coeffs; T = EmptyTrace;
    entry->seval = pos->stm == WHITE ?  EvalPosition(pos)
                                     : -EvalPosition(pos);

    // evaluate() -> [[NTERMS][COLOUR_NB]]
    initCoefficients(coeffs);
    initTunerTuples(entry, coeffs);

    // Save some of the evaluation modifiers
    entry->eval = T.eval;
    entry->turn = pos->stm;
}

void initTunerEntries(TEntry *entries) {

    Position pos;
    char line[128];
    FILE *fin = fopen(DATASET, "r");

    for (int i = 0; i < NPOSITIONS; i++) {

        if (fgets(line, 128, fin) == NULL) exit(EXIT_FAILURE);

        // Find the result { W, L, D } => { 1.0, 0.0, 0.5 }
        if      (strstr(line, "[1.0]")) entries[i].result = 1.0;
        else if (strstr(line, "[0.5]")) entries[i].result = 0.5;
        else if (strstr(line, "[0.0]")) entries[i].result = 0.0;

        // Set the board with the current FEN and initialize
        ParseFen(line, &pos);
        initTunerEntry(&entries[i], &pos);
    }
}

double sigmoid(double K, double E) {
    return 1.0 / (1.0 + exp(-K * E / 400.0));
}

double staticEvaluationErrors(TEntry * entries, double K) {

    // Compute the error of the dataset using the Static Evaluation.
    // We provide simple speedups that make use of the OpenMP Library.
    double total = 0.0;
    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(entries[i].result - sigmoid(K, entries[i].seval), 2);
    }

    return total / (double) NPOSITIONS;
}

double computeOptimalK(TEntry * entries) {

    double start = 0.0, end = 10, step = 1.0;
    double curr = start, error;
    double best = staticEvaluationErrors(entries, start);

    for (int i = 0; i < KPRECISION; i++) {

        // Find the minimum within [start, end] using the current step
        curr = start - step;
        while (curr < end) {
            curr = curr + step;
            error = staticEvaluationErrors(entries, curr);
            if (error <= best)
                best = error, start = curr;
        }

        // Adjust the search space
        end   = start + step;
        start = start - step;
        step  = step  / 10.0;
    }

    return start;
}

double linearEvaluation(TEntry *entry, TVector params) {

    double mixed;
    double midgame, endgame;
    double mg[COLOR_NB] = {0}, eg[COLOR_NB] = {0};

    // Save any modifications for MG or EG for each evaluation type
    for (int i = 0; i < entry->ntuples; i++) {
        mg[WHITE] += (double) entry->tuples[i].wcoeff * params[entry->tuples[i].index][MG];
        mg[BLACK] += (double) entry->tuples[i].bcoeff * params[entry->tuples[i].index][MG];
        eg[WHITE] += (double) entry->tuples[i].wcoeff * params[entry->tuples[i].index][EG];
        eg[BLACK] += (double) entry->tuples[i].bcoeff * params[entry->tuples[i].index][EG];
    }

    midgame = (double) MgScore(entry->eval) + mg[WHITE] - mg[BLACK];
    endgame = (double) EgScore(entry->eval) + eg[WHITE] - eg[BLACK];

    mixed =  (midgame * entry->phase
           +  endgame * (256 - entry->phase))
           / 256;

    return mixed + (entry->turn == WHITE ? Tempo : -Tempo);
}

void updateSingleGradient(TEntry *entry, TVector gradient, TVector params, double K) {

    double E = linearEvaluation(entry, params);
    double S = sigmoid(K, E);
    double X = (entry->result - S) * S * (1 - S);

    double mgBase = X * entry->pfactors[MG];
    double egBase = X * entry->pfactors[EG];

    for (int i = 0; i < entry->ntuples; i++) {
        int index  = entry->tuples[i].index;
        int wcoeff = entry->tuples[i].wcoeff;
        int bcoeff = entry->tuples[i].bcoeff;

        gradient[index][MG] += mgBase * (wcoeff - bcoeff);
        gradient[index][EG] += egBase * (wcoeff - bcoeff);
    }
}

void computeGradient(TEntry *entries, TVector gradient, TVector params, double K, int batch) {

    // #pragma omp parallel shared(gradient)
    {
        TVector local = {0};
        // #pragma omp for schedule(static, BATCHSIZE / NPARTITIONS)
        for (int i = batch * BATCHSIZE; i < (batch + 1) * BATCHSIZE; i++)
            updateSingleGradient(&entries[i], local, params, K);

        for (int i = 0; i < NTERMS; i++) {
            gradient[i][MG] += local[i][MG];
            gradient[i][EG] += local[i][EG];
        }
    }
}

double tunedEvaluationErrors(TEntry *entries, TVector params, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(entries[i].result - sigmoid(K, linearEvaluation(&entries[i], params)), 2);
    }

    return total / (double) NPOSITIONS;
}

void runTuner() {

    TVector currentParams = {0};
    TVector params = {0}, adagrad = {0};
    double K, error, rate = LRRATE;
    TEntry *entries = calloc(NPOSITIONS, sizeof(TEntry));

    initTunerEntries(entries);
    puts("Init entries: Complete");
    K = computeOptimalK(entries);
    printf("Optimal K: %g\n", K);

    InitBaseParams(currentParams);
    printParameters(params, currentParams);

    for (int epoch = 0; epoch < MAXEPOCHS; epoch++) {
        for (int batch = 0; batch < NPOSITIONS / BATCHSIZE; batch++) {

            TVector gradient = {0};
            computeGradient(entries, gradient, params, K, batch);

            for (int i = 0; i < NTERMS; i++) {
                adagrad[i][MG] += pow((K / 200.0) * gradient[i][MG] / BATCHSIZE, 2.0);
                adagrad[i][EG] += pow((K / 200.0) * gradient[i][EG] / BATCHSIZE, 2.0);
                params[i][MG] += (K / 200.0) * (gradient[i][MG] / BATCHSIZE) * (rate / sqrt(1e-8 + adagrad[i][MG]));
                params[i][EG] += (K / 200.0) * (gradient[i][EG] / BATCHSIZE) * (rate / sqrt(1e-8 + adagrad[i][EG]));
            }
        }

        error = tunedEvaluationErrors(entries, params, K);
        printf("Epoch [%d] Error = [%-8g], Rate = [%g]\n", epoch, error, rate);

        // Pre-scheduled Learning Rate drops
        if (epoch && epoch % LRSTEPRATE == 0) rate = rate / LRDROPRATE;
        if (epoch && epoch % REPORTING == 0) printParameters(params, currentParams);
    }
}
