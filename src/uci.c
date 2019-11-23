// uci.c

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "board.h"
#include "cli.h"
#include "makemove.h"
#include "misc.h"
#include "move.h"
#include "tests.h"
#include "transposition.h"
#include "search.h"
#include "validate.h"


#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define INPUT_SIZE 4096


bool ABORT_SIGNAL = false;


// Checks if a string begins with another string
static inline bool BeginsWith(const char *string, const char *token) {
    return strstr(string, token) == string;
}

// Parses a go command
static void *ParseGo(void *searchThreadInfo) {

    SearchThread *sst = (SearchThread*)searchThreadInfo;
    char *line        = sst->line;
    Position *pos     = sst->pos;
    SearchInfo *info  = sst->info;

    info->starttime = GetTimeMs();
    info->timeset = false;

    const int moveOverhead = 50;
    int depth = -1, movestogo = 30, movetime = -1;
    int time = -1, inc = 0;
    char *ptr = NULL;

    // Read in relevant time constraints if any
    if ((ptr = strstr(line, "infinite"))) {
        ;
    }
    // Increment
    if ((ptr = strstr(line, "binc")) && pos->side == BLACK)
        inc = atoi(ptr + 5);
    if ((ptr = strstr(line, "winc")) && pos->side == WHITE)
        inc = atoi(ptr + 5);
    // Total remaining time
    if ((ptr = strstr(line, "wtime")) && pos->side == WHITE)
        time = atoi(ptr + 6);
    if ((ptr = strstr(line, "btime")) && pos->side == BLACK)
        time = atoi(ptr + 6);
    // Moves left until next time control
    if ((ptr = strstr(line, "movestogo")))
        movestogo = atoi(ptr + 10);
    // Time per move
    if ((ptr = strstr(line, "movetime")))
        movetime = atoi(ptr + 9);
    // Max depth to search to
    if ((ptr = strstr(line, "depth")))
        depth = atoi(ptr + 6);

    // In movetime mode we use all the time given each turn and expect more next move
    if (movetime != -1) {
        time = movetime;
        movestogo = 1;
    }

    // Update search depth limit if we were given one
    info->depth = depth == -1 ? MAXDEPTH : depth;

    // Calculate how much time to use if given time constraints
    if (time != -1) {
        info->timeset = true;
        int timeThisMove = (time / movestogo) + inc;    // Try to use 1/30 of remaining time + increment
        int maxTime = time;                             // Most time we can use
        info->stoptime  = info->starttime;
        info->stoptime += timeThisMove > maxTime ? maxTime : timeThisMove;
        info->stoptime -= moveOverhead;
    }

    SearchPosition(pos, info);

    return NULL;
}

// Parses a position command
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

        // Ply represents how deep in a search we are, it should be 0 before searching starts
        pos->ply = 0;

        // Skip to the next move if any
        line = strstr(line, " ");
        if (line == NULL)
            return;
        line += 1;
    }
}

// Prints UCI info
static void PrintUCI() {
    printf("id name %s\n", NAME);
    printf("id author LoliSquad\n");
    printf("option name Hash type spin default %d min 4 max %d\n", DEFAULTHASH, MAXHASH);
    printf("option name Ponder type check default false\n"); // Turn on ponder stats in cutechess gui
    printf("option name SyzygyPath type string default <empty>\n");
    printf("uciok\n"); fflush(stdout);
}

// Reads a line from stdin
static inline bool GetInput(char *line) {

    memset(line, 0, INPUT_SIZE);

    if (fgets(line, INPUT_SIZE, stdin) == NULL)
        return false;

    line[strcspn(line, "\r\n")] = '\0';

    return true;
}

// Sets up the engine and follows UCI protocol commands
int main(int argc, char **argv) {

    // Init engine
    Position pos[1];
    SearchInfo info[1];
    pos->hashTable->TT = NULL;
    InitHashTable(pos->hashTable, DEFAULTHASH);

    // Benchmark
    if (argc > 1 && strstr(argv[1], "bench")) {
        if (argc > 2)
            benchmark(atoi(argv[2]), pos, info);
        else
            benchmark(13, pos, info);
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

        if (BeginsWith(line, "go")) {
            ABORT_SIGNAL = false;
            strncpy(searchThreadInfo.line, line, INPUT_SIZE);
            pthread_create(&searchThread, NULL, &ParseGo, &searchThreadInfo);

        } else if (BeginsWith(line, "isready")) {
            printf("readyok\n"); fflush(stdout);

        } else if (BeginsWith(line, "position"))
            ParsePosition(line, pos);

        else if (BeginsWith(line, "ucinewgame"))
            ClearHashTable(pos->hashTable);

        else if (BeginsWith(line, "stop")) {
            ABORT_SIGNAL = true;
            pthread_join(searchThread, NULL);

        } else if (BeginsWith(line, "quit")) {
            break;

        } else if (BeginsWith(line, "uci"))
            PrintUCI();

        else if (BeginsWith(line, "setoption name Hash value ")) {
            int MB;
            sscanf(line, "%*s %*s %*s %*s %d", &MB);
            InitHashTable(pos->hashTable, MB);

        } else if (BeginsWith(line, "setoption name SyzygyPath value ")) {

            char *path = line + strlen("setoption name SyzygyPath value ");

            // Replace newline with null
            char *newline;
            if ((newline = strchr(path, '\n')))
                path[newline-path] = '\0';

            strcpy(info->syzygyPath, path);
            tb_init(info->syzygyPath);

            if (TB_LARGEST > 0)
                printf("TableBase init complete - largest found: %d\n", TB_LARGEST);
        }
        // Non UCI commands
#ifdef DEV
        else if (!strncmp(line, "weiss", 5)) {
            Console_Loop(pos, info);
            break;
        }
#endif
    }
    free(pos->hashTable->TT);
    return 0;
}