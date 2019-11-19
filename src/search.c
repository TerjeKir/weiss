// search.c

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "attack.h"
#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "misc.h"
#include "move.h"
#include "movegen.h"
#include "movepicker.h"
#include "transposition.h"
#include "syzygy.h"


int Reductions[32][32];

extern bool ABORT_SIGNAL;


// Initializes the late move reduction array
static void InitReductions() __attribute__((constructor));
static void InitReductions() {

    for (int depth = 0; depth < 32; ++depth)
        for (int moves = 0; moves < 32; ++moves)
            Reductions[depth][moves] = 0.75 + log(depth) * log(moves) / 2.25;
}

// Check time situation
static bool OutOfTime(SearchInfo *info) {

    if (  (info->nodes & 8192) == 0
        && info->timeset
        && GetTimeMs() >= info->stoptime)

        return true;

    return false;
}

// Check if current position is a repetition
static bool IsRepetition(const Position *pos) {

    for (int i = pos->hisPly - 2; i >= pos->hisPly - pos->fiftyMove; i -= 2)
        if (pos->posKey == pos->history[i].posKey)
            return true;

    return false;
}

// Get ready to start a search
static void ClearForSearch(Position *pos, SearchInfo *info) {

    memset(pos->searchHistory, 0, sizeof(pos->searchHistory));
    memset(pos->searchKillers, 0, sizeof(pos->searchKillers));

    pos->ply       = 0;
    info->nodes    = 0;
    info->tbhits   = 0;
    info->seldepth = 0;
#ifdef SEARCH_STATS
    pos->hashTable->hit = 0;
    pos->hashTable->cut = 0;
    pos->hashTable->overWrite = 0;
    info->fh  = 0;
    info->fhf = 0;
#endif
}

// Print thinking
static void PrintThinking(const SearchInfo *info, Position *pos, int score, const int depth) {

    // Determine whether we have a centipawn or mate score
    char *scoreType = abs(score) > ISMATE ? "mate" : "cp";

    // Convert score to mate score when applicable
    score = score >  ISMATE ?  ((INFINITE - score) / 2) + 1
          : score < -ISMATE ? -((INFINITE + score) / 2)
          : score;

    int elapsed  = GetTimeMs() - info->starttime;
    int hashFull = HashFull(pos);
    int nps      = (int)(1000 * (info->nodes / (elapsed + 1)));

    // Basic info
    printf("info score %s %d depth %d seldepth %d nodes %" PRId64 " nps %d tbhits %" PRId64 " time %d hashfull %d ",
            scoreType, score, depth, info->seldepth, info->nodes, nps, info->tbhits, elapsed, hashFull);

    // Principal variation
    printf("pv");
    for (int i = 0; i < info->pv.length; i++)
        printf(" %s", MoveToStr(info->pv.line[i]));

    printf("\n");

#ifdef SEARCH_STATS
    if (info->nodes > (uint64_t)depth)
        printf("Stats: Hits: %d Overwrite: %d NewWrite: %d Cut: %d\nOrdering %.2f NullCut: %d\n", pos->hashTable->hit,
            pos->hashTable->overWrite, pos->hashTable->newWrite, pos->hashTable->cut, (info->fhf/info->fh)*100, info->nullCut);
#endif
    fflush(stdout);
}

// Print conclusion of search - best move and ponder move
static void PrintConclusion(const PV pv) {

    printf("bestmove %s", MoveToStr(pv.line[0]));
    if (pv.length > 1)
        printf(" ponder %s", MoveToStr(pv.line[1]));
    printf("\n\n");
    fflush(stdout);
}

static uint64_t relativeRank7(const int side) {
    return side ? rank7BB : rank2BB;
}

static bool pawnOn7th(const Position *pos) {
    return pos->colorBBs[pos->side] & pos->pieceBBs[PAWN] & relativeRank7(pos->side);
}

// Dynamic delta pruning margin
static int QuiescenceDeltaMargin(const Position *pos) {

    // Optimistic to improve our position by a pawn, or if we have
    // a pawn on the 7th we can hope to improve by a queen instead
    const int DeltaBase = pawnOn7th(pos) ? Q_MG : P_MG;

    // Look for possible captures on the board
    const bitboard enemy = pos->colorBBs[!pos->side];

    // Find the most valuable piece we could take and add to our base
    // TODO: Faster with pos->pieceCounts?
    return (enemy & pos->pieceBBs[QUEEN ]) ? DeltaBase + Q_MG
         : (enemy & pos->pieceBBs[ROOK  ]) ? DeltaBase + R_MG
         : (enemy & pos->pieceBBs[BISHOP]) ? DeltaBase + B_MG
         : (enemy & pos->pieceBBs[KNIGHT]) ? DeltaBase + N_MG
                                           : DeltaBase + P_MG;
}

// Quiescence
static int Quiescence(int alpha, const int beta, Position *pos, SearchInfo *info, PV *pv) {

    assert(CheckBoard(pos));

    PV pvFromHere;
    pv->length = 0;

    MovePicker mp;
    MoveList list;

    // Check time situation
    if (OutOfTime(info) || ABORT_SIGNAL)
        longjmp(info->jumpBuffer, true);

    // Update node count and selective depth
    info->nodes++;
    if (pos->ply > info->seldepth)
        info->seldepth = pos->ply;

    // Check for draw
    if (IsRepetition(pos) || pos->fiftyMove >= 100)
        return 0;

    // If we are at max depth, return static eval
    if (pos->ply >= MAXDEPTH)
        return EvalPosition(pos);

    // Standing Pat -- If the stand-pat beats beta there is most likely also a move that beats beta
    // so we assume we have a beta cutoff. If the stand-pat beats alpha we use it as alpha.
    int score = EvalPosition(pos);
    if (score >= beta)
        return score;
    if (score + QuiescenceDeltaMargin(pos) < alpha)
        return alpha;
    if (score > alpha)
        alpha = score;

    InitNoisyMP(&mp, &list, pos);

    int movesTried = 0;
    int bestScore = score;

    // Move loop
    int move;
    while ((move = NextMove(&mp))) {

        // Recursively search the positions after making the moves, skipping illegal ones
        if (!MakeMove(pos, move)) continue;
        score = -Quiescence(-beta, -alpha, pos, info, &pvFromHere);
        TakeMove(pos);

        movesTried++;

        if (score > bestScore) {

            bestScore = score;

            // If score beats alpha we update alpha
            if (score > alpha) {
                alpha = score;

                // Update the Principle Variation
                pv->length = 1 + pvFromHere.length;
                pv->line[0] = move;
                memcpy(pv->line + 1, pvFromHere.line, sizeof(int) * pvFromHere.length);

                // If score beats beta we have a cutoff
                if (score >= beta) {

        #ifdef SEARCH_STATS
                    if (movesTried == 1) info->fhf++;
                    info->fh++;
        #endif
                    return score;
                }
            }
        }
    }

    return bestScore;
}

// Alpha Beta
static int AlphaBeta(int alpha, int beta, int depth, Position *pos, SearchInfo *info, PV *pv, const int doNull) {

    assert(CheckBoard(pos));

    const bool pvNode = alpha != beta - 1;

    PV pv_from_here;
    pv->length = 0;

    MovePicker mp;
    MoveList list;

    int R;

    // Extend search if in check
    const bool inCheck = SqAttacked(pos->kingSq[pos->side], !pos->side, pos);
    if (inCheck) depth++;

    // Quiescence at the end of search
    if (depth <= 0)
        return Quiescence(alpha, beta, pos, info, &pv_from_here);

    // Check time situation
    if (OutOfTime(info) || ABORT_SIGNAL)
        longjmp(info->jumpBuffer, true);

    // Update node count and selective depth
    info->nodes++;
    if (pos->ply > info->seldepth)
        info->seldepth = pos->ply;

    // Early exits (not in root node)
    if (pos->ply) {

        // Position is drawn
        if (IsRepetition(pos) || pos->fiftyMove >= 100)
            return 0;

        // Max depth reached
        if (pos->ply >= MAXDEPTH)
            return EvalPosition(pos);

        // Mate distance pruning
        alpha = MAX(alpha, -INFINITE + pos->ply);
        beta  = MIN(beta,   INFINITE - pos->ply - 1);
        if (alpha >= beta)
            return alpha;
    }

    int score = -INFINITE;
    int ttMove = NOMOVE;

    // Probe transposition table
    if (ProbeHashEntry(pos, &ttMove, &score, alpha, beta, depth) && !pvNode) {
#ifdef SEARCH_STATS
        pos->hashTable->cut++;
#endif
        return score;
    }

    // Probe syzygy TBs
    unsigned tbresult;
    if ((tbresult = probeWDL(pos)) != TB_RESULT_FAILED) {

        info->tbhits++;

        int val = tbresult == TB_LOSS ? -INFINITE + MAXDEPTH + pos->ply + 1
                : tbresult == TB_WIN  ?  INFINITE - MAXDEPTH - pos->ply - 1
                                      :  0;

        int flag = tbresult == TB_LOSS ? BOUND_UPPER
                 : tbresult == TB_WIN  ? BOUND_LOWER
                                       : BOUND_EXACT;

        if (    flag == BOUND_EXACT
            || (flag == BOUND_LOWER && val >= beta)
            || (flag == BOUND_UPPER && val <= alpha)) {

            StoreHashEntry(pos, NOMOVE, val, flag, MAXDEPTH-1);
            return val;
        }
    }

    // Skip pruning while in check
    if (!inCheck) {

        // Do a static evaluation for pruning consideration
        score = EvalPosition(pos);

        // Null Move Pruning
        if (doNull && score >= beta && pos->ply && (pos->bigPieces[pos->side] > 0) && depth >= 4) {

            MakeNullMove(pos);
            score = -AlphaBeta(-beta, -beta + 1, depth - 4, pos, info, &pv_from_here, false);
            TakeNullMove(pos);

            // Cutoff
            if (score >= beta) {
#ifdef SEARCH_STATS
                info->nullCut++;
#endif
                // Don't return unproven mate scores
                if (score >= ISMATE)
                    score = beta;
                return score;
            }
        }
    }

    InitNormalMP(&mp, &list, pos, ttMove);

    int movesTried = 0;
    const int oldAlpha = alpha;
    int bestMove = NOMOVE;
    int bestScore = -INFINITE;
    score = -INFINITE;

    // Move loop
    int move;
    while ((move = NextMove(&mp))) {

        // Make the next predicted best move, skipping illegal ones
        if (!MakeMove(pos, move)) continue;

        movesTried++;

        bool moveIsNoisy = move & MOVE_IS_NOISY;
        bool doLMR = depth > 2 && movesTried > (2 + pvNode) && pos->ply && !moveIsNoisy;

        // Reduced depth zero-window search (-1 depth)
        if (doLMR) {
            // Base reduction
            R  = Reductions[MIN(31, depth)][MIN(31, movesTried)];
            // Reduce more in non-pv nodes
            R += !pvNode;

            // Depth after reductions, avoiding going straight to quiescence
            int RDepth = MAX(1, depth - 1 - MAX(R, 1));

            score = -AlphaBeta(-alpha - 1, -alpha, RDepth, pos, info, &pv_from_here, true);
        }
        // Full depth zero-window search
        if ((doLMR && score > alpha) || (!doLMR && (!pvNode || movesTried > 1)))
            score = -AlphaBeta(-alpha - 1, -alpha, depth - 1, pos, info, &pv_from_here, true);

        // Full depth alpha-beta window search
        if (pvNode && ((score > alpha && score < beta) || movesTried == 1))
            score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, &pv_from_here, true);

        // Undo the move
        TakeMove(pos);

        // Found a new best move in this position
        if (score > bestScore) {

            bestScore = score;
            bestMove  = move;

            // If score beats alpha we update alpha
            if (score > alpha) {

                alpha = score;

                // Update the Principle Variation
                pv->length = 1 + pv_from_here.length;
                pv->line[0] = move;
                memcpy(pv->line + 1, pv_from_here.line, sizeof(int) * pv_from_here.length);

                // If score beats beta we have a cutoff
                if (score >= beta) {

                    // Update killers if quiet move
                    if (!(move & MOVE_IS_CAPTURE)) {
                        pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
                        pos->searchKillers[0][pos->ply] = move;
                    }

    #ifdef SEARCH_STATS
                    if (movesTried == 1) info->fhf++;
                    info->fh++;
    #endif

                    StoreHashEntry(pos, move, score, BOUND_LOWER, depth);

                    return score;
                }

                // Update searchHistory if quiet move and not beta cutoff
                if (!(move & MOVE_IS_CAPTURE))
                    pos->searchHistory[pos->board[FROMSQ(bestMove)]][TOSQ(bestMove)] += depth;
            }
        }
    }

    // Checkmate or stalemate
    if (!movesTried)
        return inCheck ? -INFINITE + pos->ply : 0;

    assert(alpha >= oldAlpha);
    assert(alpha <=  INFINITE);
    assert(alpha >= -INFINITE);
    assert(bestScore <=  INFINITE);
    assert(bestScore >= -INFINITE);

    const int flag = alpha != oldAlpha ? BOUND_EXACT : BOUND_UPPER;
    StoreHashEntry(pos, bestMove, bestScore, flag, depth);

    return bestScore;
}

// Aspiration window
int AspirationWindow(Position *pos, SearchInfo *info, const int depth, int previousScore, PV *pv) {

    // Dynamic bonus increasing initial window and delta
    const int bonus = (previousScore * previousScore) / 8;
    // Delta used for initial window and widening
    const int delta = (P_MG / 2) + bonus;
    // Initial window
    int alpha = previousScore - delta / 4;
    int beta  = previousScore + delta / 4;
    // Counter for failed searches, bounds are relaxed more for each successive fail
    unsigned fails = 0;

    while (true) {
        int result = AlphaBeta(alpha, beta, depth, pos, info, pv, true);
        // Result within the bounds is accepted as correct
        if (result >= alpha && result <= beta)
            return result;
        // Failed low, relax lower bound and search again
        else if (result < alpha) {
            alpha -= delta << fails++;
            alpha  = MAX(alpha, -INFINITE);
        // Failed high, relax upper bound and search again
        } else if (result > beta) {
            beta += delta << fails++;
            beta  = MIN(beta, INFINITE);
        }
    }
}

// Root of search
void SearchPosition(Position *pos, SearchInfo *info) {

    int score;
    unsigned depth;

    ClearForSearch(pos, info);

    // Iterative deepening
    for (depth = 1; depth <= info->depth; ++depth) {

        if (setjmp(info->jumpBuffer)) break;

        // Search position, using aspiration windows for higher depths
        if (depth > 6)
            score = AspirationWindow(pos, info, depth, score, &info->pv);
        else
            score = AlphaBeta(-INFINITE, INFINITE, depth, pos, info, &info->pv, true);

        // Print thinking
        PrintThinking(info, pos, score, depth);
    }

    // Print conclusion
    PrintConclusion(info->pv);
}