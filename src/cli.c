// cli.c

#ifdef DEV

#include <stdio.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"
#include "search.h"
#include "tests.h"


#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define PERFT_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"


void Console_Loop(Position *pos) {

    printf("\nweiss started in console mode\n");
    printf("Type help for commands\n\n");

    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char inBuf[80], command[80];

    ParseFen(START_FEN, pos);

    InitTT(pos->hashTable, 256);

    tb_init("F:\\Syzygy");

    while (true) {

        printf("\nweiss > ");

        memset(&inBuf[0], 0, sizeof(inBuf));

        if (!fgets(inBuf, 80, stdin))
            continue;

        sscanf(inBuf, "%s", command);

        // Basic commands

        if (!strcmp(command, "help")) {
            printf("Commands:\n");
            printf("new         - clear hash, setup start position\n");
            printf("pos x       - set position to fen x\n");
            printf("print       - print the current board state\n");
            printf("eval        - print evaluation of current board state\n");
            printf("perft       - run a perft\n");
            printf("mirrortest  - run a test verifying eval is symmetrical\n");
            printf("matetest    - run a test looking for mate in various position\n");
            printf("quit        - quit\n");
            continue;
        }

        if (!strcmp(command, "quit")) {
            return;
        }

        // Set gamestate

        if (!strcmp(command, "pos")) {
            ParseFen(inBuf + 4, pos);
            continue;
        }

        if (!strcmp(command, "new")) {
            ClearTT(pos->hashTable);
            ParseFen(START_FEN, pos);
            continue;
        }

        // Show info about current state

        if (!strcmp(command, "eval")) {
            printf("Eval     : %d\n", EvalPosition(pos));
            MirrorBoard(pos);
            printf("Mirrored : %d\n", EvalPosition(pos));
            MirrorBoard(pos);
            continue;
        }

        if (!strcmp(command, "print")) {
            PrintBoard(pos);
            continue;
        }

        // Tests

        if (!strcmp(command, "perft")) {

            Position perftBoard[1];
            int perftDepth = 5;
            sscanf(inBuf, "perft %d", &perftDepth);
            perftDepth = perftDepth > 6 ? 6 : perftDepth;
            char *perftFen = inBuf + 8;

            if (!*perftFen)
                ParseFen(PERFT_FEN, perftBoard);
            else
                ParseFen(perftFen, perftBoard);

            Perft(perftDepth, perftBoard);
            continue;
        }

        if (!strcmp(command, "mirrortest")) {
            MirrorEvalTest(pos);
            continue;
        }

        if (!strcmp(command, "matetest")) {
            MateInXTest(pos);
            continue;
        }

        // If we got here the input was wrong
        printf("Unknown command: %s\n", inBuf);
        continue;
    }
}
#endif