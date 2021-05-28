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

#ifdef TUNE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../evaluate.h"
#include "../psqt.h"
#include "tuner.h"


// Piece values
extern const int PieceValue[COLOR_NB][PIECE_NB];

// PSQT values
extern const int PieceSqValue[7][64];

// Misc
extern const int PawnDoubled;
extern const int PawnIsolated;
extern const int PawnSupport;
extern const int BishopPair;

// Passed pawn [rank]
extern const int PawnPassed[RANK_NB];

// (Semi) open file for rook and queen [pt-4]
extern const int OpenFile[2];
extern const int SemiOpenFile[2];

extern const int KingLineDanger[28];

// Mobility
extern const int Mobility[4][28];


EvalTrace T, EmptyTrace;

TTuple *TupleStack;
int TupleStackSize;


void PrintSingle(char *name, TVector params, int i, char *S) {
    printf("const int %s%s = S(%3d,%3d);\n", name, S, (int) params[i][MG], (int) params[i][EG]);
}

void PrintArray(char *name, TVector params, int i, int A, char *S) {

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

void PrintPSQT(TVector params, int i) {

    puts("\n// Black's point of view - easier to read as it's not upside down");
    puts("const int PieceSqValue[7][64] = {\n    { 0 },");

    for (int pt = 0; pt < 6; pt++) {

        printf("\n    {");

        for (int sq = 0; sq < 64; sq++) {
            if (sq && sq % 8 == 0) printf("\n     ");
            printf(" S(%3d,%3d)", (int) params[i+MirrorSquare(sq)][MG],
                                  (int) params[i+MirrorSquare(sq)][EG]);
            printf("%s", sq == 64 - 1 ? "" : ",");
        }
        printf(" },\n");
        i += 64;
    }
    puts("};");
}

void PrintMob(TVector params, int i) {

    printf("\n// Mobility [pt-2][mobility]\n");
    printf("const int Mobility[4][28] = {\n");

    printf("    // Knight (0-8)\n");
    printf("    {");
    for (int mob = 0; mob <= 8; ++mob, ++i) {
        if (mob && mob % 8 == 0) printf("\n     ");
        printf(" S(%3d,%3d)", (int) params[i][MG], (int) params[i][EG]);
        printf("%s", mob == 8 ? "" : ",");
    }
    printf(" },\n");

    printf("    // Bishop (0-13)\n");
    printf("    {");
    for (int mob = 0; mob <= 13; ++mob, ++i) {
        if (mob && mob % 8 == 0) printf("\n     ");
        printf(" S(%3d,%3d)", (int) params[i][MG], (int) params[i][EG]);
        printf("%s", mob == 13 ? "" : ",");
    }
    printf(" },\n");

    printf("    // Rook (0-14)\n");
    printf("    {");
    for (int mob = 0; mob <= 14; ++mob, ++i) {
        if (mob && mob % 8 == 0) printf("\n     ");
        printf(" S(%3d,%3d)", (int) params[i][MG], (int) params[i][EG]);
        printf("%s", mob == 14 ? "" : ",");
    }
    printf(" },\n");

    printf("    // Queen (0-27)\n");
    printf("    {");
    for (int mob = 0; mob <= 27; ++mob, ++i) {
        if (mob && mob % 8 == 0) printf("\n     ");
        printf(" S(%3d,%3d)", (int) params[i][MG], (int) params[i][EG]);
        printf("%s", mob == 27 ? "" : ",");
    }
    printf(" }\n");

    printf("};\n");
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
            if (pt == KNIGHT && mob >  8) break;
            if (pt == BISHOP && mob > 13) break;
            if (pt == ROOK   && mob > 14) break;
            tparams[i][MG] = MgScore(Mobility[pt-2][mob]);
            tparams[i][EG] = EgScore(Mobility[pt-2][mob]);
            i++;
        }
    }

    // Misc
    tparams[i][MG] = MgScore(PawnDoubled);
    tparams[i][EG] = EgScore(PawnDoubled);
    i++;

    tparams[i][MG] = MgScore(PawnIsolated);
    tparams[i][EG] = EgScore(PawnIsolated);
    i++;

    tparams[i][MG] = MgScore(PawnSupport);
    tparams[i][EG] = EgScore(PawnSupport);
    i++;

    tparams[i][MG] = MgScore(BishopPair);
    tparams[i][EG] = EgScore(BishopPair);
    i++;

    // Passed pawns
    for (int j = 0; j < 8; ++j) {
        tparams[i][MG] = MgScore(PawnPassed[j]);
        tparams[i][EG] = EgScore(PawnPassed[j]);
        i++;
    }

    // Semi-open and open files
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

    for (int j = 0; j < 28; ++j) {
        tparams[i][MG] = MgScore(KingLineDanger[j]);
        tparams[i][EG] = EgScore(KingLineDanger[j]);
        i++;
    }

    if (i != NTERMS) {
        printf("Error 1 in printParameters(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void PrintParameters(TVector params, TVector current) {

    TVector tparams;

    for (int i = 0; i < NTERMS; ++i) {
        tparams[i][MG] = round(params[i][MG] + current[i][MG]);
        tparams[i][EG] = round(params[i][EG] + current[i][EG]);
    }

    int i = 0;
    puts("\n");

    // Piece values
    PrintArray("PieceValue", tparams, i, 5, "[5]");
    i+=5;

    // PSQT
    PrintPSQT(tparams, i);
    i+=6*64;

    // Mobility
    PrintMob(tparams, i);
    i+=9+14+15+28;

    puts("\n// Misc bonuses and maluses");
    PrintSingle("PawnDoubled", tparams, i++, "   ");
    PrintSingle("PawnIsolated", tparams, i++, "  ");
    PrintSingle("PawnSupport", tparams, i++, "   ");
    PrintSingle("BishopPair", tparams, i++, "    ");

    puts("\n// Passed pawn [rank]");
    PrintArray("PawnPassed", tparams, i, 8, "[8]");
    i+=8;

    puts("\n// (Semi) open file for rook and queen [pt-4]");
    PrintArray("OpenFile", tparams, i, 2, "[2]    ");
    i+=2;
    PrintArray("SemiOpenFile", tparams, i, 2, "[2]");
    i+=2;
    puts("");
    PrintArray("KingLineDanger", tparams, i, 28, "[28]");
    i+=28;
    puts("");

    if (i != NTERMS) {
        printf("Error 2 in printParameters(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void InitCoefficients(TCoeffs coeffs) {

    int i = 0;

    for (int pt = PAWN-1; pt <= QUEEN-1; ++pt)
        coeffs[i++] = T.PieceValue[pt][WHITE] - T.PieceValue[pt][BLACK];

    for (int pt = PAWN; pt <= KING; ++pt)
        for (int sq = 0; sq < 64; ++sq)
            coeffs[i++] = T.PSQT[pt-1][sq][WHITE] - T.PSQT[pt-1][sq][BLACK];

    for (int pt = KNIGHT; pt <= QUEEN; ++pt)
        for (int mob = 0; mob < 28; ++mob) {
            if (pt == KNIGHT && mob >  8) break;
            if (pt == BISHOP && mob > 13) break;
            if (pt == ROOK   && mob > 14) break;
            coeffs[i++] = T.Mobility[pt-2][mob][WHITE] - T.Mobility[pt-2][mob][BLACK];
        }

    coeffs[i++] = T.PawnDoubled[WHITE]    - T.PawnDoubled[BLACK];
    coeffs[i++] = T.PawnIsolated[WHITE]   - T.PawnIsolated[BLACK];
    coeffs[i++] = T.PawnSupport[WHITE]    - T.PawnSupport[BLACK];
    coeffs[i++] = T.BishopPair[WHITE]     - T.BishopPair[BLACK];

    for (int j = 0; j < 8; ++j)
        coeffs[i++] = T.PawnPassed[j][WHITE] - T.PawnPassed[j][BLACK];

    for (int j = 0; j < 2; ++j)
        coeffs[i++] = T.OpenFile[j][WHITE] - T.OpenFile[j][BLACK];

    for (int j = 0; j < 2; ++j)
        coeffs[i++] = T.SemiOpenFile[j][WHITE] - T.SemiOpenFile[j][BLACK];

    for (int j = 0; j < 28; ++j)
        coeffs[i++] = T.KingLineDanger[j][WHITE] - T.KingLineDanger[j][BLACK];

    if (i != NTERMS){
        printf("Error in InitCoefficients(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void InitTunerTuples(TEntry *entry, TCoeffs coeffs) {

    static int allocs = 0;
    int length = 0, tidx = 0;

    // Count the needed Coefficients
    for (int i = 0; i < NTERMS; i++)
        length += coeffs[i] != 0.0;

    // Allocate additional memory if needed
    if (length > TupleStackSize) {
        TupleStackSize = STACKSIZE;
        TupleStack = calloc(STACKSIZE, sizeof(TTuple));
        int ttupleMB = STACKSIZE * sizeof(TTuple) / (1 << 20);
        printf("Allocating [%dMB] x%d\r", ttupleMB, ++allocs);
    }

    // Claim part of the Tuple Stack
    entry->tuples   = TupleStack;
    entry->ntuples  = length;
    TupleStack     += length;
    TupleStackSize -= length;

    // Finally setup each of our TTuples for this TEntry
    for (int i = 0; i < NTERMS; i++)
        if (coeffs[i] != 0.0)
            entry->tuples[tidx++] = (TTuple) { i, coeffs[i] };
}

void InitTunerEntry(TEntry *entry, Position *pos) {

    // Save time by computing phase scalars now
    entry->pfactors[MG] = 0 + pos->phaseValue / 24.0;
    entry->pfactors[EG] = 1 - pos->phaseValue / 24.0;
    entry->phase = pos->phase;

    // Save a white POV static evaluation
    TCoeffs coeffs; T = EmptyTrace;
    entry->seval = pos->stm == WHITE ?  EvalPosition(pos, NULL)
                                     : -EvalPosition(pos, NULL);

    // evaluate() -> [[NTERMS][COLOUR_NB]]
    InitCoefficients(coeffs);
    InitTunerTuples(entry, coeffs);

    // Save some of the evaluation modifiers
    entry->eval = T.eval;
    entry->turn = pos->stm;
}

void InitTunerEntries(TEntry *entries) {

    Position pos;
    char line[128];
    FILE *fin = fopen(DATASET, "r");

    for (int i = 0; i < NPOSITIONS; i++) {

        if (fgets(line, 128, fin) == NULL) exit(EXIT_FAILURE);

        // Find the result { W, L, D } => { 1.0, 0.0, 0.5 }
        if      (strstr(line, "[1.0]")) entries[i].result = 1.0;
        else if (strstr(line, "[0.5]")) entries[i].result = 0.5;
        else if (strstr(line, "[0.0]")) entries[i].result = 0.0;
        else    {printf("Cannot Parse %s\n", line); exit(EXIT_FAILURE);}

        // Set the board with the current FEN and initialize
        ParseFen(line, &pos);
        InitTunerEntry(&entries[i], &pos);
    }
}

double Sigmoid(double K, double E) {
    return 1.0 / (1.0 + exp(-K * E / 400.0));
}

double StaticEvaluationErrors(TEntry * entries, double K) {

    // Compute the error of the dataset using the Static Evaluation.
    // We provide simple speedups that make use of the OpenMP Library.
    double total = 0.0;
    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(entries[i].result - Sigmoid(K, entries[i].seval), 2);
    }

    return total / (double) NPOSITIONS;
}

double ComputeOptimalK(TEntry *entries) {

    const double rate = 100, delta = 1e-5, deviation_goal = 1e-6;
    double K = 2, deviation = 1;

    while (fabs(deviation) > deviation_goal) {
        double up   = StaticEvaluationErrors(entries, K + delta);
        double down = StaticEvaluationErrors(entries, K - delta);
        deviation = (up - down) / (2 * delta);
        K -= deviation * rate;
    }

    return K;
}

double LinearEvaluation(TEntry *entry, TVector params) {

    double midgame = MgScore(entry->eval);
    double endgame = EgScore(entry->eval);

    // Save any modifications for MG or EG for each evaluation type
    for (int i = 0; i < entry->ntuples; i++) {
        midgame += (double) entry->tuples[i].coeff * params[entry->tuples[i].index][MG];
        endgame += (double) entry->tuples[i].coeff * params[entry->tuples[i].index][EG];
    }

    double mixed =  (midgame * entry->phase
                  +  endgame * (256 - entry->phase))
                  / 256;

    return mixed + (entry->turn == WHITE ? Tempo : -Tempo);
}

void UpdateSingleGradient(TEntry *entry, TVector gradient, TVector params, double K) {

    double E = LinearEvaluation(entry, params);
    double S = Sigmoid(K, E);
    double X = (entry->result - S) * S * (1 - S);

    double mgBase = X * entry->pfactors[MG];
    double egBase = X * entry->pfactors[EG];

    for (int i = 0; i < entry->ntuples; i++) {
        int index = entry->tuples[i].index;
        int coeff = entry->tuples[i].coeff;

        gradient[index][MG] += mgBase * coeff;
        gradient[index][EG] += egBase * coeff;
    }
}

void ComputeGradient(TEntry *entries, TVector gradient, TVector params, double K) {

    #pragma omp parallel shared(gradient)
    {
        TVector local = {0};
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS)
        for (int i = 0; i < NPOSITIONS; i++)
            UpdateSingleGradient(&entries[i], local, params, K);

        for (int i = 0; i < NTERMS; i++) {
            gradient[i][MG] += local[i][MG];
            gradient[i][EG] += local[i][EG];
        }
    }
}

double TunedEvaluationErrors(TEntry *entries, TVector params, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(entries[i].result - Sigmoid(K, LinearEvaluation(&entries[i], params)), 2);
    }

    return total / (double) NPOSITIONS;
}

void Tune() {

    TVector currentParams = {0};
    TVector params = {0}, adagrad = {0};
    double K, error, rate = LRRATE;
    TEntry *entries = calloc(NPOSITIONS, sizeof(TEntry));
    TupleStack      = calloc(STACKSIZE,  sizeof(TTuple));

    printf("Tuning %d terms using %s\n", NTERMS, DATASET);
    InitTunerEntries(entries);
    printf("Allocated:\n");
    printf("Optimal K...\r");
    K = ComputeOptimalK(entries);
    printf("Optimal K: %g\n\n", K);

    InitBaseParams(currentParams);
    // PrintParameters(params, currentParams);

    for (int epoch = 1; epoch <= MAXEPOCHS; epoch++) {

        TVector gradient = {0};
        ComputeGradient(entries, gradient, params, K);

        for (int i = 0; i < NTERMS; i++) {
            adagrad[i][MG] += pow((K / 200.0) * gradient[i][MG] / NPOSITIONS, 2.0);
            adagrad[i][EG] += pow((K / 200.0) * gradient[i][EG] / NPOSITIONS, 2.0);
            params[i][MG] += (K / 200.0) * (gradient[i][MG] / NPOSITIONS) * (rate / sqrt(1e-8 + adagrad[i][MG]));
            params[i][EG] += (K / 200.0) * (gradient[i][EG] / NPOSITIONS) * (rate / sqrt(1e-8 + adagrad[i][EG]));
        }

        error = TunedEvaluationErrors(entries, params, K);
        printf("\rEpoch [%d] Error = [%.8f], Rate = [%g]", epoch, error, rate);

        // Pre-scheduled Learning Rate drops
        if (epoch % LRSTEPRATE == 0) rate = rate / LRDROPRATE;
        if (epoch % REPORTING == 0) PrintParameters(params, currentParams);
    }
}

#endif
