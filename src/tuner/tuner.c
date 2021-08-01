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
#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../evaluate.h"
#include "tuner.h"


// Piece values
extern const int PieceTypeValue[TYPE_NB];

// PSQT values
extern const int PieceSqValue[6][64];

// Misc
extern const int PawnDoubled;
extern const int PawnIsolated;
extern const int PawnSupport;
extern const int PawnThreat;
extern const int PushThreat;
extern const int PawnOpen;
extern const int BishopPair;
extern const int KingAtkPawn;
extern const int OpenForward;
extern const int SemiForward;
extern const int NBBehindPawn;

// Passed pawn
extern const int PawnPassed[RANK_NB];
extern const int PassedDefended[RANK_NB];
extern const int PassedBlocked[4];
extern const int PassedDistUs[4];
extern const int PassedDistThem;
extern const int PassedRookBack;

// Pawn phalanx
extern const int PawnPhalanx[RANK_NB];

// KingLineDanger
extern const int KingLineDanger[28];

// Mobility
extern const int Mobility[4][28];


EvalTrace T, EmptyTrace;

TTuple *TupleStack;
int TupleStackSize;


void PrintSingle_(char *name, TIntVector params, int i, char *filler) {
    printf("const int %s%s = S(%3d,%3d);\n", name, filler, params[i][MG], params[i][EG]);
}

#define PrintElements(elements, perLine, space)             \
    for (int a = 0; a < elements; ++a, ++i) {               \
        if (a && a % perLine == 0) printf("\n    " space);  \
        printf("S(%3d,%3d)", params[i][MG], params[i][EG]); \
        printf("%s", a == (elements - 1) ? "" : ", ");      \
    }

void PrintArray_(char *name, TIntVector params, int i, int A, char *filler) {

    int perLine = A >= 8 ? 4 : 7;

    printf("const int %s%s = {\n    ", name, filler);
    PrintElements(A, perLine, "");
    printf("\n};\n");
}

void PrintPieceValues(TIntVector params, int i) {
    puts("enum PieceValue {");
    printf("    P_MG = %4d, P_EG = %4d,\n", params[i+0][MG], params[i+0][EG]);
    printf("    N_MG = %4d, N_EG = %4d,\n", params[i+1][MG], params[i+1][EG]);
    printf("    B_MG = %4d, B_EG = %4d,\n", params[i+2][MG], params[i+2][EG]);
    printf("    R_MG = %4d, R_EG = %4d,\n", params[i+3][MG], params[i+3][EG]);
    printf("    Q_MG = %4d, Q_EG = %4d\n",  params[i+4][MG], params[i+4][EG]);
    puts("};");
}

void PrintPSQT(TIntVector params, int i) {

    puts("\n// Black's point of view - easier to read as it's not upside down");
    puts("const int PieceSqValue[6][64] = {");

    for (int pt = 0; pt < 6; pt++) {
        printf("\n    { ");
        PrintElements(64, 8, "  ")
        printf(" },\n");
    }
    puts("};");
}

void PrintMobility(TIntVector params, int i) {

    #define PrintSingleMob(piece, max)              \
        printf("    // "#piece" (0-"#max")\n");     \
        printf("    { ");                           \
        PrintElements((max+1), 8, "  ");            \
        printf(#piece[0] == 'Q' ? " }\n" : " },\n")

    printf("\n// Mobility [pt-2][mobility]\n");
    printf("const int Mobility[4][28] = {\n");
    PrintSingleMob(Knight,  8);
    PrintSingleMob(Bishop, 13);
    PrintSingleMob(Rook,   14);
    PrintSingleMob(Queen,  27);
    printf("};\n");
}

void InitBaseParams(TVector tparams) {

    #define InitBaseSingle(term)        \
        tparams[i][MG] = MgScore(term), \
        tparams[i][EG] = EgScore(term), \
        i++

    #define InitBaseArray(term, count)  \
        for (int j = 0; j < count; ++j) \
            InitBaseSingle(term[j])

    int i = 0;

    // Piece values
    for (int pt = PAWN; pt <= QUEEN; ++pt)
        InitBaseSingle(PieceTypeValue[pt]);

    // PSQT
    for (int pt = PAWN; pt <= KING; ++pt)
        InitBaseArray(PieceSqValue[pt-1], 64);

    // Misc
    InitBaseSingle(PawnDoubled);
    InitBaseSingle(PawnIsolated);
    InitBaseSingle(PawnSupport);
    InitBaseSingle(PawnThreat);
    InitBaseSingle(PushThreat);
    InitBaseSingle(PawnOpen);
    InitBaseSingle(BishopPair);
    InitBaseSingle(KingAtkPawn);
    InitBaseSingle(OpenForward);
    InitBaseSingle(SemiForward);
    InitBaseSingle(NBBehindPawn);

    // Passed pawns
    InitBaseArray(PawnPassed, RANK_NB);
    InitBaseArray(PassedDefended, RANK_NB);
    InitBaseArray(PassedBlocked, 4);
    InitBaseArray(PassedDistUs, 4);
    InitBaseSingle(PassedDistThem);
    InitBaseSingle(PassedRookBack);

    // Pawn phalanx
    InitBaseArray(PawnPhalanx, RANK_NB);

    // KingLineDanger
    InitBaseArray(KingLineDanger, 28);

    // Mobility
    InitBaseArray(Mobility[KNIGHT-2],  9);
    InitBaseArray(Mobility[BISHOP-2], 14);
    InitBaseArray(Mobility[  ROOK-2], 15);
    InitBaseArray(Mobility[ QUEEN-2], 28);

    if (i != NTERMS) {
        printf("Error in InitBaseParams(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void PrintParameters(TVector updates, TVector base) {

    #define PrintSingle(term, spaces) \
        PrintSingle_(#term, updated, i++, spaces)

    #define PrintArray(term, length) \
        PrintArray_(#term, updated, i, length, "["#length"]"), i+=length

    TIntVector updated;

    for (int i = 0; i < NTERMS; ++i) {
        updated[i][MG] = round(base[i][MG] + updates[i][MG]);
        updated[i][EG] = round(base[i][EG] + updates[i][EG]);
    }

    int i = 0;
    puts("\n");

    PrintPieceValues(updated, i);
    i+=5;

    PrintPSQT(updated, i);
    i+=6*64;

    puts("\n// Misc bonuses and maluses");
    PrintSingle(PawnDoubled, " ");
    PrintSingle(PawnIsolated, "");
    PrintSingle(PawnSupport, " ");
    PrintSingle(PawnThreat, "  ");
    PrintSingle(PushThreat, "  ");
    PrintSingle(PawnOpen, "    ");
    PrintSingle(BishopPair, "  ");
    PrintSingle(KingAtkPawn, " ");
    PrintSingle(OpenForward, " ");
    PrintSingle(SemiForward, " ");
    PrintSingle(NBBehindPawn, "");

    puts("\n// Passed pawn");
    PrintArray(PawnPassed, RANK_NB);
    PrintArray(PassedDefended, RANK_NB);
    PrintArray(PassedBlocked, 4);
    PrintArray(PassedDistUs, 4);
    PrintSingle(PassedDistThem, "");
    PrintSingle(PassedRookBack, "");

    puts("\n// Pawn phalanx");
    PrintArray(PawnPhalanx, RANK_NB);

    puts("\n// KingLineDanger");
    PrintArray(KingLineDanger, 28);

    PrintMobility(updated, i);
    i+=9+14+15+28;
    puts("");

    if (i != NTERMS) {
        printf("Error in PrintParameters(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void InitCoefficients(TCoeffs coeffs) {

    #define InitCoeffSingle(term) \
        coeffs[i++] = T.term[WHITE] - T.term[BLACK]

    #define InitCoeffArray(term, count) \
        for (int j = 0; j < count; ++j) \
            InitCoeffSingle(term[j])

    #define InitCoeff2DArray(term, count1, count2) \
        for (int k = 0; k < count1; ++k)           \
            InitCoeffArray(term[k], count2)

    int i = 0;

    // Piece values
    InitCoeffArray(PieceValue, QUEEN);

    // PSQT
    InitCoeff2DArray(PSQT, KING, 64);

    // Misc
    InitCoeffSingle(PawnDoubled);
    InitCoeffSingle(PawnIsolated);
    InitCoeffSingle(PawnSupport);
    InitCoeffSingle(PawnThreat);
    InitCoeffSingle(PushThreat);
    InitCoeffSingle(PawnOpen);
    InitCoeffSingle(BishopPair);
    InitCoeffSingle(KingAtkPawn);
    InitCoeffSingle(OpenForward);
    InitCoeffSingle(SemiForward);
    InitCoeffSingle(NBBehindPawn);

    // Passed pawns
    InitCoeffArray(PawnPassed, RANK_NB);
    InitCoeffArray(PassedDefended, RANK_NB);
    InitCoeffArray(PassedBlocked, 4);
    InitCoeffArray(PassedDistUs, 4);
    InitCoeffSingle(PassedDistThem);
    InitCoeffSingle(PassedRookBack);

    // Pawn phalanx
    InitCoeffArray(PawnPhalanx, RANK_NB);

    // KingLineDanger
    InitCoeffArray(KingLineDanger, 28);

    // Mobility
    InitCoeffArray(Mobility[KNIGHT-2],  9);
    InitCoeffArray(Mobility[BISHOP-2], 14);
    InitCoeffArray(Mobility[  ROOK-2], 15);
    InitCoeffArray(Mobility[ QUEEN-2], 28);

    if (i != NTERMS){
        printf("Error in InitCoefficients(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

double LinearEvaluation(TEntry *entry, TVector params, int base) {

    double midgame = MgScore(base);
    double endgame = EgScore(base);

    for (int i = 0; i < entry->ntuples; i++) {
        int termIndex = entry->tuples[i].index;
        midgame += (double) entry->tuples[i].coeff * params[termIndex][MG];
        endgame += (double) entry->tuples[i].coeff * params[termIndex][EG];
    }

    double eval = (  midgame * entry->phase
                   + endgame * (MidGame - entry->phase) * entry->scale)
                  / MidGame;

    return eval + (entry->turn == WHITE ? Tempo : -Tempo);
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

void InitTunerEntry(TEntry *entry, Position *pos, int *danger) {

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
    entry->scale = T.scale / 128.0;
    entry->turn = pos->stm;
    *danger = T.danger[WHITE] - T.danger[BLACK];
}

void InitTunerEntries(TEntry *entries, TVector baseParams) {

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

        TEntry *entry = &entries[i];
        int danger;

        // Set the board with the current FEN and initialize
        ParseFen(line, &pos);
        InitTunerEntry(entry, &pos, &danger);

        int coeffEval = LinearEvaluation(entry, baseParams, -danger);
        int deviation = abs(entry->seval - coeffEval);

        if (deviation > 1) {
            printf("\nDeviation %d between real eval and coeff eval too big: %s", deviation, line);
            exit(0);
        }
    }
}

double Sigmoid(double K, double E) {
    return 1.0 / (1.0 + exp(-K * E / 400.0));
}

double StaticEvaluationErrors(TEntry * entries, double K) {

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

void UpdateSingleGradient(TEntry *entry, TVector gradient, TVector params, double K) {

    double E = LinearEvaluation(entry, params, entry->eval);
    double S = Sigmoid(K, E);
    double X = (entry->result - S) * S * (1 - S);

    double mgBase = X * entry->pfactors[MG];
    double egBase = X * entry->pfactors[EG];

    for (int i = 0; i < entry->ntuples; i++) {
        int index = entry->tuples[i].index;
        int coeff = entry->tuples[i].coeff;

        gradient[index][MG] += mgBase * coeff;
        gradient[index][EG] += egBase * coeff * entry->scale;
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
            total += pow(entries[i].result - Sigmoid(K, LinearEvaluation(&entries[i], params, entries[i].eval)), 2);
    }

    return total / (double) NPOSITIONS;
}

void Tune() {

    TVector baseParams = {0}, params = {0}, momentum = {0}, velocity = {0};
    double K, error, rate = LRRATE;
    TEntry *entries = calloc(NPOSITIONS, sizeof(TEntry));
    TupleStack      = calloc(STACKSIZE,  sizeof(TTuple));

    printf("Tuning %d terms using %d positions from %s\n", NTERMS, NPOSITIONS, DATASET);
    InitBaseParams(baseParams);
    InitTunerEntries(entries, baseParams);
    printf("Allocated:\n");
    printf("Optimal K...\r");
    K = ComputeOptimalK(entries);
    printf("Optimal K: %g\n\n", K);

    for (int epoch = 1; epoch <= MAXEPOCHS; epoch++) {

        TVector gradient = {0};
        ComputeGradient(entries, gradient, params, K);

        for (int i = 0; i < NTERMS; i++) {
            double mg_grad = (-K / 200.0) * gradient[i][MG] / NPOSITIONS;
            double eg_grad = (-K / 200.0) * gradient[i][EG] / NPOSITIONS;

            momentum[i][MG] = BETA_1 * momentum[i][MG] + (1.0 - BETA_1) * mg_grad;
            momentum[i][EG] = BETA_1 * momentum[i][EG] + (1.0 - BETA_1) * eg_grad;

            velocity[i][MG] = BETA_2 * velocity[i][MG] + (1.0 - BETA_2) * pow(mg_grad, 2);
            velocity[i][EG] = BETA_2 * velocity[i][EG] + (1.0 - BETA_2) * pow(eg_grad, 2);

            params[i][MG] -= rate * momentum[i][MG] / (1e-8 + sqrt(velocity[i][MG]));
            params[i][EG] -= rate * momentum[i][EG] / (1e-8 + sqrt(velocity[i][EG]));
        }

        error = TunedEvaluationErrors(entries, params, K);
        printf("\rEpoch [%d] Error = [%.8f], Rate = [%g]", epoch, error, rate);
        if (epoch % 10 == 0) puts("");

        // Pre-scheduled Learning Rate drops
        if (epoch % LRSTEPRATE == 0) rate = rate / LRDROPRATE;
        if (epoch % REPORTING == 0) PrintParameters(params, baseParams);
    }
}

#endif
