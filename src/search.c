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
#include "time.h"
#include "move.h"
#include "movegen.h"
#include "movepicker.h"
#include "transposition.h"
#include "syzygy.h"


int Reductions[32][32];

SearchLimits Limits;
extern bool ABORT_SIGNAL;


// Initializes the late move reduction array
CONSTR InitReductions() {

    for (int depth = 0; depth < 32; ++depth)
        for (int moves = 0; moves < 32; ++moves)
            Reductions[depth][moves] = 0.75 + log(depth) * log(moves) / 2.25;
}

// Check time situation
static bool OutOfTime(SearchInfo *info) {

    if (  (info->nodes & 8191) == 8191
        && Limits.timelimit
        && TimeSince(Limits.start) >= Limits.maxUsage)

        return true;

    return false;
}

// Check if current position is a repetition
static bool IsRepetition(const Position *pos) {

    // Compare current posKey to posKeys in history, skipping
    // opponents turns as that wouldn't be a repetition
    for (int i = 2; i <= pos->fiftyMove; i += 2)
        if (pos->posKey == history(-i).posKey)
            return true;

    return false;
}

// Get ready to start a search
static void ClearForSearch(Position *pos, SearchInfo *info) {

    memset(info, 0, sizeof(SearchInfo));

    memset(pos->searchHistory, 0, sizeof(pos->searchHistory));
    memset(pos->searchKillers, 0, sizeof(pos->searchKillers));

    pos->ply = 0;
}

// Print thinking
static void PrintThinking(const SearchInfo *info, Position *pos) {

    int score = info->score;

    // Determine whether we have a centipawn or mate score
    char *type = abs(score) > ISMATE ? "mate" : "cp";

    // Convert score to mate score when applicable
    score = score >  ISMATE ?  ((INFINITE - score) / 2) + 1
          : score < -ISMATE ? -((INFINITE + score) / 2)
          : score * 100 / P_MG;

    TimePoint elapsed = Now() - Limits.start;
    int depth    = info->depth;
    int seldepth = info->seldepth;
    int hashFull = HashFull(pos);
    int nps      = (int)(1000 * (info->nodes / (elapsed + 1)));
    uint64_t nodes  = info->nodes;
    uint64_t tbhits = info->tbhits;

    // Basic info
    printf("info depth %d seldepth %d score %s %d time %" PRId64 " nodes %" PRIu64 " nps %d tbhits %" PRIu64 " hashfull %d ",
            depth, seldepth, type, score, elapsed, nodes, nps, tbhits, hashFull);

    // Principal variation
    printf("pv");
    for (int i = 0; i < info->pv.length; i++)
        printf(" %s", MoveToStr(info->pv.line[i]));

    printf("\n");
    fflush(stdout);
}

// Print conclusion of search - best move and ponder move
static void PrintConclusion(const SearchInfo *info) {

    printf("bestmove %s", MoveToStr(info->bestMove));
    if (info->ponderMove)
        printf(" ponder %s", MoveToStr(info->ponderMove));
    printf("\n\n");
    fflush(stdout);
}

INLINE bool pawnOn7th(const Position *pos) {
    return colorBB(sideToMove()) & pieceBB(PAWN) & RankBB[relativeRank(sideToMove(), RANK_7)];
}

// Dynamic delta pruning margin
static int QuiescenceDeltaMargin(const Position *pos) {

    // Optimistic we can improve our position by a pawn without capturing anything,
    // or if we have a pawn on the 7th we can hope to improve by a queen instead
    const int DeltaBase = pawnOn7th(pos) ? Q_MG : P_MG;

    // Look for possible captures on the board
    const Bitboard enemy = colorBB(!sideToMove());

    // Find the most valuable piece we could take and add to our base
    return (enemy & pieceBB(QUEEN )) ? DeltaBase + Q_MG
         : (enemy & pieceBB(ROOK  )) ? DeltaBase + R_MG
         : (enemy & pieceBB(BISHOP)) ? DeltaBase + B_MG
         : (enemy & pieceBB(KNIGHT)) ? DeltaBase + N_MG
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

    int bestScore = score;

    // Move loop
    int move;
    while ((move = NextMove(&mp))) {

        // Recursively search the positions after making the moves, skipping illegal ones
        if (!MakeMove(pos, move)) continue;
        score = -Quiescence(-beta, -alpha, pos, info, &pvFromHere);
        TakeMove(pos);

        // Found a new best move in this position
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
                if (score >= beta)
                    break;
            }
        }
    }

    return bestScore;
}

// Alpha Beta
static int AlphaBeta(int alpha, int beta, int depth, Position *pos, SearchInfo *info, PV *pv) {

    assert(CheckBoard(pos));

    const bool pvNode = alpha != beta - 1;
    const bool root   = pos->ply == 0;

    PV pvFromHere;
    pv->length = 0;

    MovePicker mp;
    MoveList list;

    // Extend search if in check
    const bool inCheck = SqAttacked(pos->pieceList[makePiece(sideToMove(), KING)][0], !sideToMove(), pos);
    if (inCheck) depth++;

    // Quiescence at the end of search
    if (depth <= 0)
        return Quiescence(alpha, beta, pos, info, pv);

    // Check time situation
    if (OutOfTime(info) || ABORT_SIGNAL)
        longjmp(info->jumpBuffer, true);

    // Update node count and selective depth
    info->nodes++;
    if (pos->ply > info->seldepth)
        info->seldepth = pos->ply;

    // Early exits
    if (!root) {

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

    // Probe transposition table
    bool ttHit;
    uint64_t posKey = pos->posKey;
    TTEntry *tte = ProbeTT(pos, posKey, &ttHit);

    int ttMove  = ttHit ? tte->move : NOMOVE;
    int ttScore = ttHit ? ScoreFromTT(tte->score, pos->ply) : NOSCORE;

    // Trust the ttScore in non-pvNodes as long as the entry depth is equal or higher
    if (!pvNode && ttHit && tte->depth >= depth) {

        assert(tte->depth >= 1 && tte->depth < MAXDEPTH);
        assert(tte->flag >= BOUND_UPPER && tte->flag <= BOUND_EXACT);
        assert(-INFINITE <= ttScore && ttScore <= INFINITE);

        // Check if ttScore causes a cutoff
        if (ttScore >= beta ? tte->flag & BOUND_LOWER
                            : tte->flag & BOUND_UPPER)

            return ttScore;
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

            StoreTTEntry(tte, posKey, NOMOVE, val, MAXDEPTH-1, flag);
            return val;
        }
    }

    int score = -INFINITE;
    int eval = NOSCORE;

    // Skip pruning while in check and at the root
    if (!inCheck && !root) {

        // Do a static evaluation for pruning consideration
        if (history(-1).move == NOMOVE)
            history(0).eval = eval = -history(-1).eval;
        else
            history(0).eval = eval = EvalPosition(pos);

        // Razoring
        if (!pvNode && depth < 2 && eval + 640 < alpha)
            return Quiescence(alpha, beta, pos, info, pv);

        // Reverse Futility Pruning
        if (!pvNode && depth < 7 && eval - 225 * depth >= beta)
            return eval;

        // Null Move Pruning
        if (   history(-1).move != NOMOVE
            && eval >= beta
            && pos->nonPawns[sideToMove()] > 0
            && depth >= 3) {

            int R = 3 + depth / 5 + MIN(3, (eval - beta) / 256);

            MakeNullMove(pos);
            score = -AlphaBeta(-beta, -beta + 1, depth - R, pos, info, &pvFromHere);
            TakeNullMove(pos);

            // Cutoff
            if (score >= beta) {
                // Don't return unproven mate scores
                if (score >= ISMATE)
                    score = beta;
                return score;
            }
        }

        // Internal iterative deepening
        if (depth >= 4 && !ttMove) {

            AlphaBeta(alpha, beta, MAX(1, MIN(depth / 2, depth - 4)), pos, info, pv);

            tte = ProbeTT(pos, posKey, &ttHit);

            ttMove  = ttHit ? tte->move : NOMOVE;
            // ttScore = ttHit ? ScoreFromTT(tte->score, pos->ply) : NOSCORE;
        }
    }

    InitNormalMP(&mp, &list, pos, ttMove);

    const int oldAlpha = alpha;
    int moveCount  = 0;
    int quietCount = 0;
    int bestMove   = NOMOVE;
    int bestScore  = score = -INFINITE;

    // Move loop
    int move;
    while ((move = NextMove(&mp))) {

        bool quiet = !moveIsNoisy(move);

        if (quiet)
            quietCount++;

        // Late move pruning
        if (!pvNode && !inCheck && quiet && depth <= 3 && quietCount > 4 * depth * depth)
            break;

        // Make the move, skipping to the next if illegal
        if (!MakeMove(pos, move)) continue;

        moveCount++;

        const int newDepth = depth - 1;

        bool doLMR = depth > 2 && moveCount > (2 + pvNode) && quiet;

        // Reduced depth zero-window search
        if (doLMR) {
            // Base reduction
            int R = Reductions[MIN(31, depth)][MIN(31, moveCount)];
            // Reduce more in non-pv nodes
            R += !pvNode;

            // Depth after reductions, avoiding going straight to quiescence
            int RDepth = MAX(1, newDepth - MAX(R, 1));

            score = -AlphaBeta(-alpha - 1, -alpha, RDepth, pos, info, &pvFromHere);
        }
        // Full depth zero-window search
        if ((doLMR && score > alpha) || (!doLMR && (!pvNode || moveCount > 1)))
            score = -AlphaBeta(-alpha - 1, -alpha, newDepth, pos, info, &pvFromHere);

        // Full depth alpha-beta window search
        if (pvNode && ((score > alpha && score < beta) || moveCount == 1))
            score = -AlphaBeta(-beta, -alpha, newDepth, pos, info, &pvFromHere);

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
                pv->length = 1 + pvFromHere.length;
                pv->line[0] = move;
                memcpy(pv->line + 1, pvFromHere.line, sizeof(int) * pvFromHere.length);

                // Update search history
                if (quiet)
                    pos->searchHistory[pieceOn(fromSq(bestMove))][toSq(bestMove)] += depth * depth;

                // If score beats beta we have a cutoff
                if (score >= beta) {

                    // Update killers if quiet move
                    if (quiet && killer1 != move) {
                        killer2 = killer1;
                        killer1 = move;
                    }

                    break;
                }
            }
        }
    }

    // Checkmate or stalemate
    if (!moveCount)
        return inCheck ? -INFINITE + pos->ply : 0;

    assert(alpha >= oldAlpha);
    assert(alpha <=  INFINITE);
    assert(alpha >= -INFINITE);
    assert(bestScore <=  INFINITE);
    assert(bestScore >= -INFINITE);

    const int flag = bestScore >= beta ? BOUND_LOWER
                   : alpha != oldAlpha ? BOUND_EXACT
                                       : BOUND_UPPER;

    StoreTTEntry(tte, posKey, bestMove, ScoreToTT(bestScore, pos->ply), depth, flag);

    return bestScore;
}

// Aspiration window
static int AspirationWindow(Position *pos, SearchInfo *info) {

    const int score = info->score;
    // Dynamic bonus increasing initial window and relaxation delta
    const int bonus = score * score;
    const int initialWindow = 12 + bonus / 2048;
    const int delta = 64 + bonus / 256;
    // Initial window
    int alpha = MAX(score - initialWindow, -INFINITE);
    int beta  = MIN(score + initialWindow,  INFINITE);
    // Counter for failed searches, bounds are relaxed more for each successive fail
    unsigned fails = 0;

    while (true) {
        int result = AlphaBeta(alpha, beta, info->depth, pos, info, &info->pv);
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

// Decides when to stop a search
static void InitTimeManagement() {

    const int overhead = 30;
    const int minTime = 10;

    // In movetime mode we use all the time given each turn
    if (Limits.movetime) {
        Limits.maxUsage = MAX(minTime, Limits.movetime - overhead);
        Limits.timelimit = true;
        return;
    }

    // No time and no movetime means there is no timelimit
    if (!Limits.time) {
        Limits.timelimit = false;
        return;
    }

    double ratio = Limits.movestogo ? MAX(1.0, Limits.movestogo * 0.75)
                                    : 30.0;

    int timeThisMove = Limits.time / ratio + 1.5 * Limits.inc;

    // Try to save at least 10ms for each move left to go
    // as well as a buffer of 30ms, while using at least 10ms
    Limits.maxUsage  = MAX(minTime, MIN(Limits.time - overhead - Limits.movestogo * minTime, timeThisMove));
    Limits.timelimit = true;
}

// Root of search
void SearchPosition(Position *pos, SearchInfo *info) {

    InitTimeManagement();

    ClearForSearch(pos, info);

    // Iterative deepening
    for (info->depth = 1; info->depth <= Limits.depth; ++info->depth) {

        if (setjmp(info->jumpBuffer)) break;

        // Search position, using aspiration windows for higher depths
        if (info->depth > 6)
            info->score = AspirationWindow(pos, info);
        else
            info->score = AlphaBeta(-INFINITE, INFINITE, info->depth, pos, info, &info->pv);

        // Print thinking
        PrintThinking(info, pos);

        // Save bestMove and ponderMove before overwriting the pv next iteration
        info->bestMove   = info->pv.line[0];
        info->ponderMove = info->pv.length > 1 ? info->pv.line[1] : NOMOVE;
    }

    // Print conclusion
    PrintConclusion(info);
}