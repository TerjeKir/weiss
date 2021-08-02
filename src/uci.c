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

#include <stddef.h>
#include <stdlib.h>

#include "pyrrhic/tbprobe.h"
#include "noobprobe/noobprobe.h"
#include "onlinesyzygy/onlinesyzygy.h"
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
static void ParseTimeControl(const char *str, const Position *pos) {

    memset(&Limits, 0, offsetof(SearchLimits, multiPV));
    Limits.start = Now();

    // Parse relevant search constraints
    Limits.infinite = strstr(str, "infinite");
    SetLimit(str, sideToMove == WHITE ? "wtime" : "btime", &Limits.time);
    SetLimit(str, sideToMove == WHITE ? "winc"  : "binc" , &Limits.inc);
    SetLimit(str, "movestogo", &Limits.movestogo);
    SetLimit(str, "movetime",  &Limits.movetime);
    SetLimit(str, "depth",     &Limits.depth);
    SetLimit(str, "mate",      &Limits.mate);

    // Parse searchmoves, assumes they are at the end of the string
    char *searchmoves = strstr(str, "searchmoves ");
    if (searchmoves) {
        char *move = strtok(searchmoves, " ");
        int i = 0;
        while ((move = strtok(NULL, " ")))
            Limits.searchmoves[i++] = ParseMove(move, pos);
    }

    Limits.timelimit = Limits.time || Limits.movetime;
    Limits.depth = Limits.depth ?: 100;
}

// Parses the given limits and creates a new thread to start the search
INLINE void Go(Position *pos, char *str) {
    ABORT_SIGNAL = false;
    InitTT();
    TT.dirty = true;
    ParseTimeControl(str, pos);
    StartMainThread(SearchPosition, pos);
}

// Parses a 'position' and sets up the board
static void Pos(Position *pos, char *str) {

    // Set up original position. This will either be a
    // position given as FEN, or the normal start position
    ParseFen(BeginsWith(str, "position fen") ? str + 13 : START_FEN, pos);

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
    if (OptionName(str, "Hash"))
        TT.requestedMB = atoi(OptionValue(str)),
        puts("info string Hash will resize after next 'isready'.");

    // Sets number of threads to use for searching
    else if (OptionName(str, "Threads"))
        InitThreads(atoi(OptionValue(str)));

    // Sets the syzygy tablebase path
    else if (OptionName(str, "SyzygyPath"))
        tb_init(OptionValue(str));

    // Sets multi-pv
    else if (OptionName(str, "MultiPV"))
        Limits.multiPV = atoi(OptionValue(str));

    // Toggles probing of Chess Cloud Database
    else if (OptionName(str, "UCI_Chess960"))
        chess960 = !strncmp(OptionValue(str), "true", 4);

    // Toggles probing of Chess Cloud Database
    else if (OptionName(str, "NoobBook"))
        noobbook = !strncmp(OptionValue(str), "true", 4);

    // Sets max depth for using NoobBook
    else if (OptionName(str, "NoobBookLimit"))
        noobLimit = atoi(OptionValue(str));

    // Toggles probing of Chess Cloud Database
    else if (OptionName(str, "OnlineSyzygy"))
        onlineSyzygy = !strncmp(OptionValue(str), "true", 4);

    else
        puts("info No such option.");

    fflush(stdout);
}

// Prints UCI info
static void Info() {
    printf("id name %s\n", NAME);
    printf("id author Terje Kirstihagen\n");
    printf("option name Hash type spin default %d min %d max %d\n", DEFAULTHASH, MINHASH, MAXHASH);
    printf("option name Threads type spin default %d min %d max %d\n", 1, 1, 2048);
    printf("option name SyzygyPath type string default <empty>\n");
    printf("option name MultiPV type spin default 1 min 1 max %d\n", MULTI_PV_MAX);
    printf("option name UCI_Chess960 type check default false\n");
    printf("option name NoobBook type check default false\n");
    printf("option name NoobBookLimit type spin default 0 min 0 max 1000\n");
    printf("option name OnlineSyzygy type check default false\n");
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
    int d = (MATE - abs(score) + 1) / 2;
    return score > 0 ? d : -d;
}

// Print thinking
void PrintThinking(const Thread *thread, int alpha, int beta) {

    const Position *pos = &thread->pos;

    TimePoint elapsed = TimeSince(Limits.start);
    uint64_t nodes    = TotalNodes(thread);
    uint64_t tbhits   = TotalTBHits(thread);
    int hashFull      = HashFull();
    int nps           = (int)(1000 * nodes / (elapsed + 1));

    Depth seldepth = 128;
    for (; seldepth > 0; --seldepth)
        if (history(seldepth-1).key != 0) break;

    for (int i = 0; i < Limits.multiPV; ++i) {

        const PV *pv = &thread->rootMoves[i].pv;
        int score = thread->rootMoves[i].score;

        // Skip empty pvs that occur when MultiPV > legal moves in root
        if (pv->length == 0) break;

        // Determine whether we have a centipawn or mate score
        char *type = abs(score) >= MATE_IN_MAX ? "mate" : "cp";

        // Determine if score is an upper or lower bound
        char *bound = score >= beta  ? " lowerbound"
                    : score <= alpha ? " upperbound"
                                     : "";

        // Translate internal score into printed score
        score = abs(score) >=  MATE_IN_MAX ? MateScore(score)
              :    abs(score) <= 8
                && pv->length <= 2         ? 0
                                           : score;

        // Basic info
        printf("info depth %d seldepth %d multipv %d score %s %d%s time %" PRId64
               " nodes %" PRIu64 " nps %d tbhits %" PRIu64 " hashfull %d pv",
                thread->depth, seldepth, i+1, type, score, bound, elapsed,
                nodes, nps, tbhits, hashFull);

        // Principal variation
        for (int j = 0; j < pv->length; j++)
            printf(" %s", MoveToStr(pv->line[j]));

        printf("\n");
    }
    fflush(stdout);
}

// Print conclusion of search - best move and ponder move
void PrintConclusion(const Thread *thread) {
    printf("bestmove %s\n\n", MoveToStr(thread->rootMoves[0].move));
    fflush(stdout);
}
