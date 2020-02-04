// uci.c

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "board.h"
#include "makemove.h"
#include "move.h"
#include "search.h"
#include "tests.h"
#include "time.h"
#include "transposition.h"


#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define INPUT_SIZE 4096


bool ABORT_SIGNAL = false;


// Checks if a string begins with another string
INLINE bool BeginsWith(const char *string, const char *token) {
    return strstr(string, token) == string;
}

// Parses the time controls
INLINE void TimeControl(int side, char *line) {

    memset(&Limits, 0, sizeof(SearchLimits));

    Limits.start = Now();

    // Read in relevant search constraints
    char *ptr = NULL;
    // if ((ptr = strstr(line, "infinite")))
    if ((ptr = strstr(line, "wtime")) && side == WHITE) Limits.time = atoi(ptr + 6);
    if ((ptr = strstr(line, "btime")) && side == BLACK) Limits.time = atoi(ptr + 6);
    if ((ptr = strstr(line, "winc"))  && side == WHITE) Limits.inc  = atoi(ptr + 5);
    if ((ptr = strstr(line, "binc"))  && side == BLACK) Limits.inc  = atoi(ptr + 5);
    if ((ptr = strstr(line, "movestogo"))) Limits.movestogo = atoi(ptr + 10);
    if ((ptr = strstr(line, "movetime")))  Limits.movetime  = atoi(ptr +  9);
    if ((ptr = strstr(line, "depth")))     Limits.depth     = atoi(ptr +  6);

    // If no depth limit is given, use MAXDEPTH
    Limits.depth = Limits.depth == 0 ? MAXDEPTH : Limits.depth;
}

// Parses a 'go' and starts a search
static void *ParseGo(void *searchThreadInfo) {

    ThreadInfo *sst  = (ThreadInfo*)searchThreadInfo;
    Position *pos    = sst->pos;
    SearchInfo *info = sst->info;

    TimeControl(sideToMove(), sst->line);

    SearchPosition(pos, info);

    return NULL;
}

// Parses a 'position' and sets up the board
static void UCIPosition(const char *line, Position *pos) {

    // Set up original position. This will either be a
    // position given as FEN, or the normal start position
    BeginsWith(line, "position fen") ? ParseFen(line + 13, pos)
                                     : ParseFen(START_FEN, pos);

    // Skip to "moves" and make them to get to correct position
    if ((line = strstr(line, "moves")) == NULL)
        return;

    line += 6;
    while (*line) {

        // Parse a move
        int move = ParseMove(line, pos);

        // Make the move
        if (!MakeMove(pos, move)) {
            printf("Weiss thinks this move is illegal: %s\n", MoveToStr(move));
            fflush(stdout);
            exit(EXIT_SUCCESS);
        }

        // Skip to the next move if any
        if ((line = strstr(line, " ")) == NULL)
            return;
        line += 1;
    }
}

// Returns the name of a setoption string
INLINE bool OptionName(const char *name, const char *line) {
    return BeginsWith(strstr(line, "name") + 5, name);
}

// Returns the value of a setoption string
INLINE char *OptionValue(const char *line) {
    return strstr(line, "value") + 6;
}

// Parses a 'setoption' and updates settings
static void UCISetoption(char *line) {

    // Sets the size of the transposition table
    if (OptionName("Hash", line)) {

        TT.requestedMB = atoi(OptionValue(line));

        printf("Hash will use %" PRI_SIZET "MB after next 'isready'.\n", TT.requestedMB);

    // Sets the syzygy tablebase path
    } else if (OptionName("SyzygyPath", line)) {

        tb_init(OptionValue(line));

        TB_LARGEST > 0 ? printf("TableBase init success - largest found: %d.\n", TB_LARGEST)
                       : printf("TableBase init failure - not found.\n");
    }
    fflush(stdout);
}

// Prints UCI info
static void UCIInfo() {
    printf("id name %s\n", NAME);
    printf("id author Terje Kirstihagen\n");
    printf("option name Hash type spin default %d min %d max %d\n", DEFAULTHASH, MINHASH, MAXHASH);
    printf("option name SyzygyPath type string default <empty>\n");
    printf("option name Ponder type check default false\n"); // Turn on ponder stats in cutechess gui
    printf("uciok\n"); fflush(stdout);
}

INLINE void UCIGo(pthread_t *st, ThreadInfo *ti, char *line) {

    ABORT_SIGNAL = false,
    strncpy(ti->line, line, INPUT_SIZE),
    pthread_create(st, NULL, &ParseGo, ti);
}

// Reads a line from stdin
INLINE bool GetInput(char *line) {

    memset(line, 0, INPUT_SIZE);

    if (fgets(line, INPUT_SIZE, stdin) == NULL)
        return false;

    // Strip newline
    line[strcspn(line, "\r\n")] = '\0';

    return true;
}

// Sets up the engine and follows UCI protocol commands
int main(int argc, char **argv) {

    // Init engine
    Position pos[1];
    SearchInfo info[1];
    TT.currentMB = 0;
    TT.requestedMB = DEFAULTHASH;

    // Benchmark
    if (argc > 1 && strstr(argv[1], "bench")) {
        InitTT();
        Benchmark(argc > 2 ? atoi(argv[2]) : 13, pos, info);
        free(TT.mem);
        return EXIT_SUCCESS;
    }

    // Setup the default position
    ParseFen(START_FEN, pos);

    // Search thread setup
    pthread_t searchThread;
    ThreadInfo threadInfo = { .pos = pos, .info = info };

    // UCI loop
    char line[INPUT_SIZE];
    while (GetInput(line)) {

        if      (BeginsWith(line, "go"))         UCIGo(&searchThread, &threadInfo, line);
        else if (BeginsWith(line, "isready"))    InitTT(), printf("readyok\n"), fflush(stdout);
        else if (BeginsWith(line, "position"))   UCIPosition(line, pos);
        else if (BeginsWith(line, "ucinewgame")) ClearTT();
        else if (BeginsWith(line, "stop"))       ABORT_SIGNAL = true, pthread_join(searchThread, NULL);
        else if (BeginsWith(line, "quit"))       break;
        else if (BeginsWith(line, "uci"))        UCIInfo();
        else if (BeginsWith(line, "setoption"))  UCISetoption(line);

#ifdef DEV
        // Non UCI commands
        else if (BeginsWith(line, "printboard")) PrintBoard(pos);
        else if (BeginsWith(line, "perft"))      Perft(line);
        else if (BeginsWith(line, "eval"))       PrintEval(pos);
        else if (BeginsWith(line, "mirrortest")) MirrorEvalTest(pos);
        else if (BeginsWith(line, "matetest"))   MateInXTest(pos);
#endif
    }
    free(TT.mem);
    return EXIT_SUCCESS;
}