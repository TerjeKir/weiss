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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "fathom/tbprobe.h"
#include "board.h"
#include "makemove.h"
#include "move.h"
#include "search.h"
#include "tests.h"
#include "threads.h"
#include "time.h"
#include "transposition.h"
#include "tune.h"
#include "uci.h"


volatile bool ABORT_SIGNAL = false;


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

    // If no depth limit is given, use MAXDEPTH - 1
    Limits.depth = Limits.depth == 0 ? MAXDEPTH - 1 : Limits.depth;
}

// Parses a 'go' and starts a search
static void *ParseGo(void *voidEngine) {

    Engine *engine  = voidEngine;
    Position *pos   = &engine->pos;
    Thread *threads = engine->threads;

    ParseTimeControl(engine->str, sideToMove);

    SearchPosition(pos, threads);

    return NULL;
}

// Starts a new thread to handle the go command and search
INLINE void UCIGo(Engine *engine, char *str) {
    ABORT_SIGNAL = false;
    strncpy(engine->str, str, INPUT_SIZE);
    pthread_create(&engine->threads->pthreads[0], NULL, &ParseGo, engine);
}

// Parses a 'position' and sets up the board
static void UCIPosition(Position *pos, char *str) {

    // Set up original position. This will either be a
    // position given as FEN, or the normal start position
    BeginsWith(str, "position fen") ? ParseFen(str + 13, pos)
                                    : ParseFen(START_FEN, pos);

    // Check if there are moves to be made from the initial position
    if ((str = strstr(str, "moves")) == NULL)
        return;

    // Loop over the moves and make them in succession
    char *move = strtok(str, " ");
    while ((move = strtok(NULL, " "))) {

        // Parse and make move
        MakeMove(pos, ParseMove(move, pos));

        // Reset ply to avoid triggering asserts in debug mode in long games
        pos->ply = 0;

        // Reset gamePly so long games don't go out of bounds of arrays
        if (pos->rule50 == 0)
            pos->gamePly = 0;
    }
}

// Parses a 'setoption' and updates settings
static void UCISetOption(Engine *engine, char *str) {

    // Sets the size of the transposition table
    if (OptionName(str, "Hash")) {

        TT.requestedMB = atoi(OptionValue(str));

        printf("Hash will use %" PRI_SIZET "MB after next 'isready'.\n", TT.requestedMB);

    // Sets number of threads to use for searching
    } else if (OptionName(str, "Threads")) {

        free(engine->threads->pthreads);
        free(engine->threads);
        engine->threads = InitThreads(atoi(OptionValue(str)));

        printf("Search will use %d threads.\n", engine->threads->count);

    // Sets the syzygy tablebase path
    } else if (OptionName(str, "SyzygyPath")) {

        tb_init(OptionValue(str));

        TB_LARGEST ? printf("TableBase init success - largest found: %d.\n", TB_LARGEST)
                   : printf("TableBase init failure - not found.\n");

    // Sets evaluation parameters (dev mode)
    } else
        TuneParseAll(strstr(str, "name") + 5, atoi(OptionValue(str)));
    fflush(stdout);
}

// Prints UCI info
static void UCIInfo() {
    printf("id name %s\n", NAME);
    printf("id author Terje Kirstihagen\n");
    printf("option name Hash type spin default %d min %d max %d\n", DEFAULTHASH, MINHASH, MAXHASH);
    printf("option name Threads type spin default %d min %d max %d\n", 1, 1, 256);
    printf("option name SyzygyPath type string default <empty>\n");
    printf("option name Ponder type check default false\n"); // Turn on ponder stats in cutechess gui
    TuneDeclareAll(); // Declares all evaluation parameters as options (dev mode)
    printf("uciok\n"); fflush(stdout);
}

// Stops searching
static void UCIStop(Engine *engine) {
    ABORT_SIGNAL = true;
    Wake(engine->threads);
    pthread_join(engine->threads->pthreads[0], NULL);
}

// Signals the engine is ready
static void UCIIsReady() {
    InitTT();
    printf("readyok\n");
    fflush(stdout);
}

// Sets up the engine and follows UCI protocol commands
int main(int argc, char **argv) {

    // Benchmark
    if (argc > 1 && strstr(argv[1], "bench"))
        return Benchmark(argc, argv), 0;

    // Init engine
    Engine engine = { .threads = InitThreads(1) };
    Position *pos = &engine.pos;

    // Setup the default position
    ParseFen(START_FEN, pos);

    // Input loop
    char str[INPUT_SIZE];
    while (GetInput(str)) {
        switch (HashInput(str)) {
            case GO         : UCIGo(&engine, str);        break;
            case UCI        : UCIInfo();                  break;
            case STOP       : UCIStop(&engine);           break;
            case ISREADY    : UCIIsReady();               break;
            case POSITION   : UCIPosition(pos, str);      break;
            case SETOPTION  : UCISetOption(&engine, str); break;
            case UCINEWGAME : ClearTT();                  break;
            case QUIT       : return 0;
#ifdef DEV
            // Non-UCI commands
            case EVAL       : PrintEval(pos);      break;
            case PRINT      : PrintBoard(pos);     break;
            case PERFT      : Perft(str);          break;
            case MIRRORTEST : MirrorEvalTest(pos); break;
#endif
        }
    }
}