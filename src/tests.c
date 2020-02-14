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
#include "time.h"
#include "move.h"
#include "movegen.h"
#include "search.h"
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

// Checks engine can find mates
void MateInXTest(Position *pos) {

    char filename[] = "../EPDs/mate_-_.epd";                 // '_'s are placeholders
    FILE *file;

    SearchInfo info[1];
    char lineIn[1024];

    int bestMoves[20];
    int foundScore;
    int correct, bmCnt, bestFound, extensions, mateDepth, foundDepth;

    for (char depth = '1'; depth < '9'; ++depth) {
        filename[12] = depth;
        for (char side = 'b'; side >= 'b'; side += 21) { // Alternates 'b' <-> 'w', overflowing goes below 'b'
            filename[14] = side;

            if ((file = fopen(filename, "r")) == NULL) {
                printf("MateInXTest: %s not found.\n", filename);
                fflush(stdout);
                return;
            }

            int lineCnt = 0;

            while (fgets(lineIn, 1024, file) != NULL) {

                // Reset
                memset(bestMoves, 0, sizeof(bestMoves));
                foundScore = 0;
                bestFound = 0;

                // Read next line
                ParseFen(lineIn, pos);
                lineCnt++;
                printf("Line %d in file: %s\n", lineCnt, filename);
                fflush(stdout);

                char *bm = strstr(lineIn, "bm") + 3;
                char *ce = strstr(lineIn, "ce");

                // Read in the mate depth
                mateDepth = depth - '0';

                // Read in the best move(s)
                bmCnt = 0;
                while (bm < ce) {
                    bestMoves[bmCnt] = ParseEPDMove(bm, pos);
                    bm = strstr(bm, " ") + 1;
                    bmCnt++;
                }

                // Search setup
                Limits.depth = (depth - '0') * 2 - 1;
                Limits.start = Now();
                extensions = 0;
search:
                SearchPosition(pos, info);

                bestFound = info->pv.line[0];

                // Check if correct move is found
                correct = 0;
                for (int i = 0; i <= bmCnt; ++i)
                    if (bestFound == bestMoves[i]){
                        printf("\nBest Found: %s\n\n", MoveToStr(bestFound));
                        fflush(stdout);
                        correct = 1;
                    }

                // Extend search if not found
                if (!correct && extensions < 5) {
                    Limits.depth += 2;
                    extensions += 1;
                    goto search;
                }

                // Correct move not found
                if (!correct) {
                    printf("\n\nMateInXTest Fail: Not found.\n%s", lineIn);
                    printf("Line %d in file: %s\n", lineCnt, filename);
                    for (int i = 0; i < bmCnt; ++i)
                        printf("Accepted answer: %s\n", MoveToStr(bestMoves[i]));
                    fflush(stdout);
                    getchar();
                    continue;
                }

                // Get pv score
                TTEntry *tte = &TT.table[((uint32_t)pos->key * (uint64_t)TT.count) >> 32];
                if (tte->posKey == pos->key)
                    foundScore = tte->score;

                // Translate score to mate depth
                foundDepth = (foundScore >  ISMATE) ? (INFINITE - foundScore) / 2 + 1
                           : (foundScore < -ISMATE) ? (INFINITE + foundScore) / 2
                                                    : 0;

                // Extend search if shortest mate not found
                if (foundDepth != mateDepth && extensions < 5) {
                    Limits.depth += 2;
                    extensions += 1;
                    goto search;
                }

                // Failed to find correct move and/or correct depth
                if (foundDepth != mateDepth) {
                    printf("\n\nMateInXTest Fail: Wrong depth.\n%s", lineIn);
                    printf("Line %d in file: %s\n", lineCnt, filename);
                    printf("Mate depth should be %d, was %d.\n", mateDepth, foundDepth);
                    fflush(stdout);
                    getchar();
                    continue;
                }

                // Clear lineIn for reuse
                memset(&lineIn[0], 0, sizeof(lineIn));
                // Clear TT
                ClearTT();
            }
        }
    }
    fclose(file);
}
#endif