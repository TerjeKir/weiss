// uci.c

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "board.h"
#include "cli.h"
#include "makemove.h"
#include "time.h"
#include "move.h"
#include "tests.h"
#include "transposition.h"
#include "search.h"
#include "validate.h"


#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
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

    SearchThread *sst = (SearchThread*)searchThreadInfo;
    Position *pos     = sst->pos;
    SearchInfo *info  = sst->info;

    TimeControl(sideToMove(), sst->line);

    SearchPosition(pos, info);

    return NULL;
}

// Parses a 'position' and sets up the board
static void ParsePosition(const char *line, Position *pos) {

    // Skip past "position "
    line += 9;

    // Set up original position, either normal start position,
    if (BeginsWith(line, "startpos"))
        ParseFen(START_FEN, pos);
    // Or position given as FEN
    else if (BeginsWith(line, "fen"))
        ParseFen(line + 4, pos);

    // Skip to "moves" and make them to get to current position
    line = strstr(line, "moves");
    if (line == NULL)
        return;

    // Skip to the first move and loop until all moves are parsed
    line += 6;
    while (*line) {

        // Parse a move
        int move = ParseMove(line, pos);
        if (move == NOMOVE) {
            printf("Weiss failed to parse a move: %s\n", line);
            fflush(stdout);
            exit(EXIT_SUCCESS);
        }

        // Make the move
        if (!MakeMove(pos, move)) {
            printf("Weiss thinks this move is illegal: %s\n", MoveToStr(move));
            fflush(stdout);
            exit(EXIT_SUCCESS);
        }

        // Skip to the next move if any
        line = strstr(line, " ");
        if (line == NULL)
            return;
        line += 1;
    }
}

// Parses a 'setoption' and updates settings
static void SetOption(char *line) {

    if (BeginsWith(line, "setoption name Hash value ")) {
        sscanf(line, "%*s %*s %*s %*s %" PRIu64 "", &TT.requestedMB);

    } else if (BeginsWith(line, "setoption name SyzygyPath value ")) {

        char *path = line + strlen("setoption name SyzygyPath value ");

        tb_init(path);

        if (TB_LARGEST > 0)
            printf("TableBase init complete - largest found: %d.\n", TB_LARGEST);
        else
            printf("TableBase init failed - not found.\n");
    }
}

// Prints UCI info
static void PrintUCI() {
    printf("id name %s\n", NAME);
    printf("id author Terje Kirstihagen\n");
    printf("option name Hash type spin default %d min %d max %d\n", DEFAULTHASH, MINHASH, MAXHASH);
    printf("option name SyzygyPath type string default <empty>\n");
    printf("option name Ponder type check default false\n"); // Turn on ponder stats in cutechess gui
    printf("uciok\n"); fflush(stdout);
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
        if (argc > 2)
            Benchmark(atoi(argv[2]), pos, info);
        else
            Benchmark(13, pos, info);
        return 0;
    }

    // Search thread setup
    pthread_t searchThread;
    SearchThread searchThreadInfo;
    searchThreadInfo.info = info;
    searchThreadInfo.pos  = pos;

    // UCI loop
    char line[INPUT_SIZE];
    while (GetInput(line)) {

        if (BeginsWith(line, "go"))
            ABORT_SIGNAL = false,
            strncpy(searchThreadInfo.line, line, INPUT_SIZE),
            pthread_create(&searchThread, NULL, &ParseGo, &searchThreadInfo);

        else if (BeginsWith(line, "isready"))
            InitTT(),
            printf("readyok\n"), fflush(stdout);

        else if (BeginsWith(line, "position"))
            ParsePosition(line, pos);

        else if (BeginsWith(line, "ucinewgame"))
            ClearTT();

        else if (BeginsWith(line, "stop"))
            ABORT_SIGNAL = true,
            pthread_join(searchThread, NULL);

        else if (BeginsWith(line, "quit"))
            break;

        else if (BeginsWith(line, "uci"))
            PrintUCI();

        else if (BeginsWith(line, "setoption"))
            SetOption(line);

        // Non UCI commands
#ifdef DEV
        else if (!strncmp(line, "weiss", 5)) {
            ConsoleLoop(pos);
            break;
        }
#endif
    }
    free(TT.table);
    return 0;
}