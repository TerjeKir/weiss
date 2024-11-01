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

#pragma once

#include <setjmp.h>

#include "board.h"
#include "evaluate.h"
#include "types.h"


#define SS_OFFSET 10
#define MULTI_PV_MAX 64
#define PAWN_HISTORY_SIZE 512
#define CORRECTION_HISTORY_SIZE 16384

INLINE int PawnStructure(const Position *pos) {
    return pos->pawnKey & (PAWN_HISTORY_SIZE - 1);
}

INLINE int PawnCorrectionIndex(const Position *pos) {
    return pos->pawnKey & (CORRECTION_HISTORY_SIZE - 1);
}

INLINE int MaterialCorrectionIndex(const Position *pos) {
    return pos->materialKey & (CORRECTION_HISTORY_SIZE - 1);
}

typedef int16_t ButterflyHistory[COLOR_NB][64][64];
typedef int16_t PawnHistory[PAWN_HISTORY_SIZE][PIECE_NB][64];
typedef int16_t CaptureToHistory[PIECE_NB][64][TYPE_NB];
typedef int16_t PieceToHistory[PIECE_NB][64];
typedef PieceToHistory ContinuationHistory[PIECE_NB][64];
typedef int16_t CorrectionHistory[2][CORRECTION_HISTORY_SIZE];


typedef struct {
    PieceToHistory *continuation;
    int staticEval;
    int histScore;
    int doubleExtensions;
    Depth ply;
    Move excluded;
    Move killers[2];
    PV pv;
} Stack;

typedef struct RootMove {
    Move move;
    int score;
    PV pv;
} RootMove;

typedef struct Thread {

    Stack ss[128];
    jmp_buf jumpBuffer;
    uint64_t tbhits;
    RootMove rootMoves[MULTI_PV_MAX];
    Depth depth;
    int rootMoveCount;
    bool doPruning;
    bool uncertain;
    int multiPV;

    // Anything below here is not zeroed out between searches
    Position pos;
    PawnCache pawnCache;
    ButterflyHistory history;
    PawnHistory pawnHistory;
    CaptureToHistory captureHistory;
    ContinuationHistory continuation[2][2];
    CorrectionHistory pawnCorrectionHistory;
    CorrectionHistory materialCorrectionHistory;

    int index;
    int count;

} Thread;


extern Thread *Threads;


void InitThreads(int threadCount);
void SortRootMoves(Thread *thread, int multiPV);
uint64_t TotalNodes();
uint64_t TotalTBHits();
void PrepareSearch(Position *pos, Move searchmoves[]);
void StartMainThread(void *(*func)(void *), Position *pos);
void StartHelpers(void *(*func)(void *));
void WaitForHelpers();
void ResetThreads();
void RunWithAllThreads(void *(*func)(void *));
void Wait(atomic_bool *condition);
void Wake();
