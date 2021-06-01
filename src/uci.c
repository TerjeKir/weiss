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
#include <stdlib.h>

#include "pyrrhic/tbprobe.h"
#include "noobprobe/noobprobe.h"
#include "tuner/tuner.h"
#include "board.h"
#include "makemove.h"
#include "move.h"
#include "search.h"
#include "tests.h"
#include "threads.h"
#include "time.h"
#include "transposition.h"
#include "uci.h"


// Parses the time controls
static void ParseTimeControl(char *str, Color color) {

    memset(&Limits, 0, sizeof(SearchLimits));
    Limits.start = Now();

    // Read in relevant search constraints
    Limits.infinite = strstr(str, "infinite");
    if (color == WHITE)
        SetLimit(str, "wtime", &Limits.time),
        SetLimit(str, "winc",  &Limits.inc);
    else
        SetLimit(str, "btime", &Limits.time),
        SetLimit(str, "binc",  &Limits.inc);
    SetLimit(str, "movestogo", &Limits.movestogo);
    SetLimit(str, "movetime",  &Limits.movetime);
    SetLimit(str, "depth",     &Limits.depth);
    SetLimit(str, "mate",      &Limits.mate);

    Limits.timelimit = Limits.time || Limits.movetime;
    Limits.depth = !Limits.depth ? MAX_PLY : MIN(Limits.depth, MAX_PLY);
}

// Parses the given limits and creates a new thread to start the search
INLINE void Go(Position *pos, char *str) {
    ABORT_SIGNAL = false;
    InitTT();
    TT.dirty = true;
    ParseTimeControl(str, pos->stm);
    StartMainThread(SearchPosition, pos);
}

// Parses a 'position' and sets up the board
static void Pos(Position *pos, char *str) {

    // Set up original position. This will either be a
    // position given as FEN, or the normal start position
    BeginsWith(str, "position fen") ? ParseFen(str + 13, pos)
                                    : ParseFen(START_FEN, pos);

    // Check if there are moves to be made from the initial position
    if ((str = strstr(str, "moves")) == NULL) return;

    // Loop over the moves and make them in succession
    char *move = strtok(str, " ");
    while ((move = strtok(NULL, " "))) {

        // Parse and make move
        MakeMove(pos, ParseMove(move, pos));

        // Keep track of how many moves have been played
        pos->gameMoves += sideToMove == WHITE;

        // Reset histPly so long games don't go out of bounds of arrays
        if (pos->rule50 == 0)
            pos->histPly = 0;
    }

    pos->nodes = 0;
}

// Parses a 'setoption' and updates settings
static void SetOption(char *str) {

    // Sets the size of the transposition table
    if (OptionName(str, "Hash")) {
        TT.requestedMB = atoi(OptionValue(str));
        puts("info string Hash will resize after next 'isready'.");

    // Sets number of threads to use for searching
    } else if (OptionName(str, "Threads")) {
        InitThreads(atoi(OptionValue(str)));

    // Sets the syzygy tablebase path
    } else if (OptionName(str, "SyzygyPath")) {
        tb_init(OptionValue(str));

    // Sets max depth for using NoobBook
    } else if (OptionName(str, "NoobBookLimit")) {
        noobLimit = atoi(OptionValue(str));

    // Toggles probing of Chess Cloud Database
    } else if (OptionName(str, "NoobBook")) {
        noobbook = !strncmp(OptionValue(str), "true", 4);

    } else {
        puts("No such option.");
    }

    fflush(stdout);
}

// Prints UCI info
static void Info() {
    printf("id name %s\n", NAME);
    printf("id author Terje Kirstihagen\n");
    printf("option name Hash type spin default %d min %d max %d\n", DEFAULTHASH, MINHASH, MAXHASH);
    printf("option name Threads type spin default %d min %d max %d\n", 1, 1, 2048);
    printf("option name SyzygyPath type string default <empty>\n");
    printf("option name NoobBook type check default false\n");
    printf("option name NoobBookLimit type spin default 0 min 0 max 1000\n");
    printf("uciok\n"); fflush(stdout);
}

// Stops searching
static void Stop() {
    ABORT_SIGNAL = true;
    Wake();
}

// Signals the engine is ready
static void IsReady() {
    InitTT();
    printf("readyok\n");
    fflush(stdout);
}

// Reset for a new game
static void NewGame() {
    ClearTT();
    ResetThreads();
    failedQueries = 0;
}

// Hashes the first token in a string
static int HashInput(char *str) {
    int hash = 0;
    int len = 1;
    while (*str && *str != ' ')
        hash ^= *(str++) ^ len++;
    return hash;
}

// Sets up the engine and follows UCI protocol commands
int main(int argc, char **argv) {

    // Benchmark
    if (argc > 1 && strstr(argv[1], "bench"))
        return Benchmark(argc, argv), 0;

    // Tuner
#ifdef TUNE
    if (argc > 1 && strstr(argv[1], "tune"))
        return Tune(), 0;
#endif

    // Init engine
    InitThreads(1);
    Position pos;
    ParseFen(START_FEN, &pos);

    // Input loop
    char str[INPUT_SIZE];
    while (GetInput(str)) {
        switch (HashInput(str)) {
            case GO         : Go(&pos, str);  break;
            case UCI        : Info();         break;
            case ISREADY    : IsReady();      break;
            case POSITION   : Pos(&pos, str); break;
            case SETOPTION  : SetOption(str); break;
            case UCINEWGAME : NewGame();      break;
            case STOP       : Stop();         break;
            case QUIT       : Stop();         return 0;
#ifdef DEV
            // Non-UCI commands
            case EVAL       : PrintEval(&pos);  break;
            case PRINT      : PrintBoard(&pos); break;
            case PERFT      : Perft(str);       break;
#endif
        }
    }
}

// Translates an internal mate score into distance to mate
INLINE int MateScore(const int score) {
    return score > 0 ?  ((MATE - score) / 2) + 1
                     : -((MATE + score) / 2);
}

// Print thinking
void PrintThinking(const Thread *thread, const Stack *ss, int score, int alpha, int beta) {

    const Position *pos = &thread->pos;
    const PV *pv = &ss->pv;

    // Determine whether we have a centipawn or mate score
    char *type = abs(score) >= MATE_IN_MAX ? "mate" : "cp";

    // Determine if score is an upper or lower bound
    char *bound = score >= beta  ? " lowerbound"
                : score <= alpha ? " upperbound"
                                 : "";

    // Translate internal score into printed score
    score = abs(score) >=  MATE_IN_MAX ? MateScore(score)
          : abs(score) >= TBWIN_IN_MAX ? score
          :    abs(score) <= 8
            && pv->length <= 2         ? 0
                                       : score * 100 / P_MG;

    TimePoint elapsed = TimeSince(Limits.start);
    uint64_t nodes    = TotalNodes(thread);
    uint64_t tbhits   = TotalTBHits(thread);
    int hashFull      = HashFull();
    int nps           = (int)(1000 * nodes / (elapsed + 1));

    Depth seldepth = 128;
    for (; seldepth > 0; --seldepth)
        if (history(seldepth-1).key != 0) break;

    // Basic info
    printf("info depth %d seldepth %d score %s %d%s time %" PRId64
           " nodes %" PRIu64 " nps %d tbhits %" PRIu64 " hashfull %d pv",
            thread->depth, seldepth, type, score, bound, elapsed,
            nodes, nps, tbhits, hashFull);

    // Principal variation
    for (int i = 0; i < pv->length; i++)
        printf(" %s", MoveToStr(pv->line[i]));

    printf("\n");
    fflush(stdout);
}

// Print conclusion of search - best move and ponder move
void PrintConclusion(const Thread *thread) {
    printf("bestmove %s\n\n", MoveToStr(thread->bestMove));
    fflush(stdout);
}
