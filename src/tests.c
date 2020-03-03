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

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "search.h"
#include "time.h"
#include "transposition.h"


#define PERFT_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"


/* Benchmark heavily inspired by Ethereal*/
static const char *BenchmarkFENs[] = {
    #include "bench.csv"
    ""
};

void Benchmark(Depth depth, Position *pos, SearchInfo *info) {

    uint64_t nodes = 0ULL;

    Limits.depth = depth;
    Limits.timelimit = false;

    TimePoint startTime = Now();

    for (int i = 0; strcmp(BenchmarkFENs[i], ""); ++i) {
        printf("Bench %d: %s\n", i + 1, BenchmarkFENs[i]);
        ParseFen(BenchmarkFENs[i], pos);
        Limits.start = Now();
        SearchPosition(pos, info);
        nodes += info->nodes;
        ClearTT();
    }

    TimePoint elapsed = Now() - startTime + 1;

    printf("Benchmark complete:"
           "\nTime : %" PRId64 "ms"
           "\nNodes: %" PRIu64
           "\nNPS  : %" PRId64 "\n",
           elapsed, nodes, 1000 * nodes / elapsed);
}

#ifdef DEV

/* Perft */
static uint64_t leafNodes;

static void RecursivePerft(const Depth depth, Position *pos) {

    assert(CheckBoard(pos));

    if (depth == 0) {
        leafNodes++;
        return;
    }

    MoveList list[1];
    GenAllMoves(pos, list);

    for (unsigned MoveNum = 0; MoveNum < list->count; ++MoveNum) {

        if (!MakeMove(pos, list->moves[MoveNum].move))
            continue;

        RecursivePerft(depth - 1, pos);
        TakeMove(pos);
    }

    return;
}

// Counts number of moves that can be made in a position to some depth
void Perft(char *line) {

    Position pos[1];
    Depth depth = 5;
    sscanf(line, "perft %d", &depth);
    depth = MIN(6, depth);
    char *perftFen = line + 8;

    !*perftFen ? ParseFen(PERFT_FEN, pos)
               : ParseFen(perftFen,  pos);

    assert(CheckBoard(pos));

    printf("\nStarting Test To Depth:%d\n\n", depth);
    fflush(stdout);

    const TimePoint start = Now();
    leafNodes = 0;

    MoveList list[1];
    GenAllMoves(pos, list);

    for (unsigned MoveNum = 0; MoveNum < list->count; ++MoveNum) {

        Move move = list->moves[MoveNum].move;

        if (!MakeMove(pos, move)){
            printf("move %d : %s : Illegal\n", MoveNum + 1, MoveToStr(move));
            fflush(stdout);
            continue;
        }

        uint64_t oldCount = leafNodes;
        RecursivePerft(depth - 1, pos);
        uint64_t newNodes = leafNodes - oldCount;
        printf("move %d : %s : %" PRId64 "\n", MoveNum + 1, MoveToStr(move), newNodes);
        fflush(stdout);

        TakeMove(pos);
    }

    const int timeElapsed = Now() - start;
    printf("\nPerft Complete : %" PRId64 " nodes visited in %dms\n", leafNodes, timeElapsed);
    if (timeElapsed > 0)
        printf("               : %" PRId64 " nps\n", ((leafNodes * 1000) / timeElapsed));
    fflush(stdout);

    return;
}

void PrintEval(Position *pos) {

    printf("Eval     : %d\n", EvalPosition(pos)); MirrorBoard(pos);
    printf("Mirrored : %d\n", EvalPosition(pos)); MirrorBoard(pos);
}

// Checks evaluation is symmetric
void MirrorEvalTest(Position *pos) {

    const char filename[] = "../EPDs/all.epd";

    FILE *file;
    if ((file = fopen(filename, "r")) == NULL) {
        printf("MirrorEvalTest: %s not found.\n", filename);
        fflush(stdout);
        return;
    }

    char lineIn[1024];
    int ev1, ev2;
    int positions = 0;

    while (fgets(lineIn, 1024, file) != NULL) {

        ParseFen(lineIn, pos);
        positions++;
        ev1 = EvalPosition(pos);
        MirrorBoard(pos);
        ev2 = EvalPosition(pos);

        if (ev1 != ev2) {
            printf("\n\n\n");
            ParseFen(lineIn, pos);
            PrintBoard(pos);
            MirrorBoard(pos);
            PrintBoard(pos);
            printf("\n\nMirrorEvalTest Fail: %d - %d\n%s\n", ev1, ev2, lineIn);
            fflush(stdout);
            return;
        }

        if (positions % 100 == 0) {
            printf("position %d\n", positions);
            fflush(stdout);
        }

        memset(&lineIn[0], 0, sizeof(lineIn));
    }
    printf("\n\nMirrorEvalTest Successful\n\n");
    fflush(stdout);
    fclose(file);
}
#endif