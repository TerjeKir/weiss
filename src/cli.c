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
#include "misc.h"
#include "transposition.h"
#include "search.h"
#include "tests.h"


#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define PERFT_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"


static int ThreeFoldRep(const Position *pos) {

    assert(CheckBoard(pos));

    int repeats = 0;

    for (int i = 0; i < pos->hisPly; ++i)
        if (pos->history[i].posKey == pos->posKey)
            repeats++;

    return repeats;
}

static bool DrawMaterial(const Position *pos) {
    assert(CheckBoard(pos));

    // Pawns can promote to pieces that can mate
    if (pos->pieceBBs[PAWN])
        return false;
    // Rooks and queens can mate
    if (pos->pieceBBs[ROOK] || pos->pieceBBs[QUEEN])
        return false;
    // 3 knights can mate
    if (PopCount(pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT]) > 2 || PopCount(pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT]) > 2)
        return false;
    // 2 bishops can mate
    if (PopCount(pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP]) > 1 || PopCount(pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP]) > 1)
        return false;
    // Bishop + Knight can mate
    if ((pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT]) && (pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP]))
        return false;
    if ((pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT]) && (pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP]))
        return false;

    return true;
}

static bool checkresult(Position *pos) {
    assert(CheckBoard(pos));

    if (pos->fiftyMove > 100) {
        printf("1/2-1/2 {fifty move rule (claimed by weiss)}\n");
        return true;
    }

    if (ThreeFoldRep(pos) >= 2) {
        printf("1/2-1/2 {3-fold repetition (claimed by weiss)}\n");
        return true;
    }

    if (DrawMaterial(pos)) {
        printf("1/2-1/2 {insufficient material (claimed by weiss)}\n");
        return true;
    }

    MoveList list[1];
    GenAllMoves(pos, list);

    int found = 0;
    for (unsigned int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

        if (!MakeMove(pos, list->moves[MoveNum].move))
            continue;

        found++;
        TakeMove(pos);
        break;
    }

    if (found != 0)
        return false;

    int InCheck = SqAttacked(pos->kingSq[pos->side], !pos->side, pos);

    if (InCheck) {
        if (pos->side == WHITE) {
            printf("0-1 {black mates (claimed by weiss)}\n");
            return true;
        } else {
            printf("0-1 {white mates (claimed by weiss)}\n");
            return true;
        }
    } else {
        printf("\n1/2-1/2 {stalemate (claimed by weiss)}\n");
        return true;
    }
    return false;
}

void Console_Loop(Position *pos, SearchInfo *info) {

    printf("\nweiss started in console mode\n");
    printf("Type help for commands\n\n");

    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    int depth = MAXDEPTH, movetime = 5000;
    int engineSide = BLACK;
    int move = NOMOVE;
    char inBuf[80], command[80];

    ParseFen(START_FEN, pos);

    InitHashTable(pos->hashTable, 256);

    tb_init("F:\\Syzygy");

    while (true) {

        // If engine's turn to play, search and play before continuing
        if (pos->side == engineSide && checkresult(pos) == false) {
            info->starttime = GetTimeMs();
            info->depth = depth;

            if (movetime != 0) {
                info->timeset = true;
                info->stoptime = info->starttime + movetime;
            }

            SearchPosition(pos, info);
            PrintBoard(pos);
        }

        printf("\nweiss > ");

        memset(&inBuf[0], 0, sizeof(inBuf));

        if (!fgets(inBuf, 80, stdin))
            continue;

        sscanf(inBuf, "%s", command);

        // Basic commands

        if (!strcmp(command, "help")) {
            printf("Commands:\n");
            printf("quit   - quit\n");
            printf("new    - new game, engine plays black unless given a go on white's turn\n");
            printf("go     - engine will play a move, and play this color from now on\n");
            printf("force  - engine will not play either color until given a go or new\n");
            printf("print  - print the current board state\n");
            printf("limits - print the current depth and movetime limits\n");
            printf("depth x - set search depth limit to x half-moves\n");
            printf("time  x - set search time limit to x ms (depth still applies if set)\n");
            printf("position x - set position to fen x\n");
            printf("enter moves using b7b8q notation\n");
            continue;
        }

        if (!strcmp(command, "quit")) {
            return;
        }

        // Search and make a move, automatically plays for current side after

        if (!strcmp(command, "go")) {
            engineSide = pos->side;
            continue;
        }

        // Engine settings

        if (!strcmp(command, "force")) {
            engineSide = BOTH;
            continue;
        }

        if (!strcmp(command, "depth")) {
            sscanf(inBuf, "depth %d", &depth);
            if (depth == 0)
                depth = MAXDEPTH;
            printf("Depth limit: %d\n", depth);
            continue;
        }

        if (!strcmp(command, "time")) {
            sscanf(inBuf, "time %d", &movetime);
            printf("Movetime limit: %dms\n", movetime);
            continue;
        }

        // Set gamestate

        if (!strcmp(command, "position")) {
            engineSide = BOTH;
            ParseFen(inBuf + 9, pos);
            continue;
        }

        if (!strcmp(command, "new")) {
            ClearHashTable(pos->hashTable);
            engineSide = BLACK;
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

        if (!strcmp(command, "limits")) {
            if (depth == MAXDEPTH)
                printf("Depth limit    : none\n");
            else
                printf("Depth limit    : %d\n", depth);

            if (movetime == 0)
                printf("Movetime limit : none\n");
            else
                printf("Movetime limit : %dms\n", movetime);

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
            engineSide = BOTH;
            MirrorEvalTest(pos);
            continue;
        }

        if (!strcmp(command, "matetest")) {
            engineSide = BOTH;
            MateInXTest(pos);
            continue;
        }

        // Parse user move
        move = ParseMove(inBuf, pos);

        // If we got here and ParseMove failed, the input was wrong
        if (move == NOMOVE) {
            printf("Unknown command: %s\n", inBuf);
            continue;
        }

        // Make the move
        MakeMove(pos, move);
        pos->ply = 0;
    }
}
#endif