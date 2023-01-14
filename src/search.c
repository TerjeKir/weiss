/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2023 Terje Kirstihagen

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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "noobprobe/noobprobe.h"
#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "history.h"
#include "makemove.h"
#include "move.h"
#include "movepicker.h"
#include "search.h"
#include "syzygy.h"
#include "time.h"
#include "threads.h"
#include "transposition.h"
#include "uci.h"


SearchLimits Limits = { .multiPV = 1 };
volatile bool ABORT_SIGNAL;
volatile bool SEARCH_STOPPED = true;

static int Reductions[2][32][32];


// Initializes the late move reduction array
CONSTR(1) InitReductions() {
    for (int depth = 1; depth < 32; ++depth)
        for (int moves = 1; moves < 32; ++moves)
            Reductions[0][depth][moves] = 0.00 + log(depth) * log(moves) / 3.25, // capture
            Reductions[1][depth][moves] = 1.50 + log(depth) * log(moves) / 1.75; // quiet
}

// Checks if the move is in the list of searchmoves if any were given
static bool NotInSearchMoves(Move move) {
    if (Limits.searchmoves[0] == NOMOVE) return false;
    for (Move *m = Limits.searchmoves; *m != NOMOVE; ++m)
        if (*m == move)
            return false;
    return true;
}

// Quiescence
static int Quiescence(Thread *thread, Stack *ss, int alpha, const int beta) {

    Position *pos = &thread->pos;
    MovePicker mp;

    const bool pvNode = alpha != beta - 1;

    // Check time situation
    if (OutOfTime(thread) || ABORT_SIGNAL)
        longjmp(thread->jumpBuffer, true);

    bool inCheck = pos->checkers;

    int eval = NOSCORE;
    int futility = -INFINITE;
    int bestScore = -INFINITE;

    // Position is drawn
    if (IsRepetition(pos) || pos->rule50 >= 100)
        return 8 - (pos->nodes & 0x7);

    // Probe transposition table
    bool ttHit;
    Key key = pos->key;
    TTEntry *tte = ProbeTT(key, &ttHit);

    Move ttMove = ttHit ? tte->move : NOMOVE;
    int ttScore = ttHit ? ScoreFromTT(tte->score, ss->ply) : NOSCORE;
    // Depth ttDepth = tte->depth;
    int ttBound = Bound(tte);

    if (ttMove && !MoveIsPseudoLegal(pos, ttMove))
        ttHit = false, ttMove = NOMOVE, ttScore = NOSCORE;

    // Trust TT if not a pvnode and the entry depth is sufficiently high
    if (   !pvNode
        && ttHit
        && (ttBound & (ttScore >= beta ? BOUND_LOWER : BOUND_UPPER)))
        return ttScore;

    // Do a static evaluation for pruning considerations
    eval = history(-1).move == NOMOVE ? -(ss-1)->eval + 2 * Tempo
                                      : EvalPosition(pos, thread->pawnCache);

    // If we are at max depth, return static eval
    if (ss->ply >= MAX_PLY)
        return eval;

    if (inCheck) goto moveloop;

    // If eval beats beta we assume some move will also beat it
    if (eval >= beta)
        return eval;

    // Use eval as a lowerbound if it's above alpha
    if (eval > alpha)
        alpha = eval;

    futility = eval + 60;
    bestScore = eval;

moveloop:

    if (!inCheck) InitNoisyMP(&mp, thread, ss, ttMove); else InitNormalMP(&mp, thread, ss, 0, ttMove, NOMOVE, NOMOVE);

    // Move loop
    Move bestMove = NOMOVE;
    Move move;
    while ((move = NextMove(&mp))) {

        if (bestScore <= -TBWIN_IN_MAX) goto search;

        // Skip moves SEE deem bad
        if (mp.stage > NOISY_GOOD) break;

        // Futility pruning
        if (    futility + PieceValue[EG][capturing(move)] <= alpha
            && !(   PieceTypeOf(piece(move)) == PAWN
                 && RelativeRank(sideToMove, RankOf(toSq(move))) > 5)) {
            bestScore = MAX(bestScore, futility + PieceValue[EG][capturing(move)]);
            continue;
        }

        // SEE pruning
        if (    futility <= alpha
            && !SEE(pos, move, 1)) {
            bestScore = MAX(bestScore, futility);
            continue;
        }

search:

        ss->continuation = &thread->continuation[moveIsCapture(move)][piece(move)][toSq(move)];

        // Recursively search the positions after making the moves, skipping illegal ones
        if (!MakeMove(pos, move)) continue;
        int score = -Quiescence(thread, ss+1, -beta, -alpha);
        TakeMove(pos);

        // Found a new best move in this position
        if (score > bestScore) {
            bestScore = score;

            // If score beats alpha we update alpha
            if (score > alpha) {
                alpha = score;
                bestMove = move;

                // If score beats beta we have a cutoff
                if (score >= beta)
                    break;
            }
        }
    }

    // Checkmate
    if (inCheck && bestScore == -INFINITE)
        return -MATE + ss->ply;

    StoreTTEntry(tte, key, bestMove, ScoreToTT(bestScore, ss->ply), 0,
                 bestScore >= beta ? BOUND_LOWER : BOUND_UPPER);

    return bestScore;
}

// Alpha Beta
static int AlphaBeta(Thread *thread, Stack *ss, int alpha, int beta, Depth depth, bool cutnode) {

    Position *pos = &thread->pos;
    MovePicker mp;
    ss->pv.length = 0;

    const bool pvNode = alpha != beta - 1;
    const bool root   = ss->ply == 0;

    // Check time situation
    if (OutOfTime(thread) || ABORT_SIGNAL)
        longjmp(thread->jumpBuffer, true);

    // Early exits
    if (!root) {

        // Position is drawn
        if (IsRepetition(pos) || pos->rule50 >= 100)
            return 8 - (pos->nodes & 0x7);

        // Max depth reached
        if (ss->ply >= MAX_PLY)
            return EvalPosition(pos, thread->pawnCache);

        // Mate distance pruning
        alpha = MAX(alpha, -MATE + ss->ply);
        beta  = MIN(beta,   MATE - ss->ply - 1);
        if (alpha >= beta)
            return alpha;
    }

    // Quiescence at the end of search
    if (depth <= 0)
        return Quiescence(thread, ss, alpha, beta);

    // Probe transposition table
    bool ttHit;
    Key key = pos->key;
    TTEntry *tte = ProbeTT(key, &ttHit);

    Move ttMove = ttHit ? tte->move : NOMOVE;
    int ttScore = ttHit ? ScoreFromTT(tte->score, ss->ply) : NOSCORE;
    Depth ttDepth = tte->depth;
    int ttBound = Bound(tte);

    if (ttMove && (!MoveIsPseudoLegal(pos, ttMove) || ttMove == ss->excluded))
        ttHit = false, ttMove = NOMOVE, ttScore = NOSCORE;

    // Trust TT if not a pvnode and the entry depth is sufficiently high
    if (   !pvNode
        && ttHit
        && ttDepth >= depth
        && (ttBound & (ttScore >= beta ? BOUND_LOWER : BOUND_UPPER))) {

        // Give a history bonus to quiet tt moves that causes a cutoff
        if (ttScore >= beta && moveIsQuiet(ttMove))
            QuietHistoryUpdate(ttMove, Bonus(depth));

        return ttScore;
    }

    int bestScore = -INFINITE;
    int maxScore  =  INFINITE;

    // Probe syzygy TBs
    int tbScore, bound;
    if (ProbeWDL(pos, &tbScore, &bound, ss->ply)) {

        thread->tbhits++;

        // Draw scores are exact, while wins are lower bounds and losses upper bounds (mate scores are better/worse)
        if (bound == BOUND_EXACT || (bound == BOUND_LOWER ? tbScore >= beta : tbScore <= alpha)) {
            StoreTTEntry(tte, key, NOMOVE, ScoreToTT(tbScore, ss->ply), MAX_PLY, bound);
            return tbScore;
        }

        // Limit the score of this node based on the tb result
        if (pvNode) {
            if (bound == BOUND_LOWER)
                bestScore = tbScore,
                alpha = MAX(alpha, tbScore);
            else
                maxScore = tbScore;
        }
    }

    const bool inCheck = pos->checkers;

    // Do a static evaluation for pruning considerations
    int eval = ss->eval = inCheck          ? NOSCORE
                        : lastMoveNullMove ? -(ss-1)->eval + 2 * Tempo
                                           : EvalPosition(pos, thread->pawnCache);

    // Use ttScore as eval if it is more informative
    if (   ttScore != NOSCORE
        && ttBound & (ttScore > eval ? BOUND_LOWER : BOUND_UPPER))
        eval = ttScore;

    // Improving if not in check, and current eval is higher than 2 plies ago
    bool improving = !inCheck && eval > (ss-2)->eval;

    // Internal iterative reduction based on Rebel's idea
    if (pvNode && depth >= 4 && !ttMove)
        depth--;

    // Skip pruning in check, in pv nodes, during early iterations, when proving singularity, or when looking for terminal scores
    if (inCheck || pvNode || !thread->doPruning || ss->excluded || abs(beta) >= TBWIN_IN_MAX)
        goto move_loop;

    // Reverse Futility Pruning
    if (   depth < 7
        && eval >= beta
        && eval - 175 * depth / (1 + improving) - (ss-1)->histScore / 400 >= beta
        && (!ttMove || GetHistory(thread, ss, ttMove) > 10000))
        return eval;

    // Null Move Pruning
    if (   eval >= beta
        && ss->eval >= beta + 120 - 15 * depth
        && (ss-1)->histScore < 35000
        && history(-1).move != NOMOVE
        && pos->nonPawnCount[sideToMove] > (depth > 8)) {

        Depth nullDepth = depth - (3 + depth / 5 + MIN(3, (eval - beta) / 256));

        // Remember who last null-moved
        Color nullMoverTemp = thread->nullMover;
        thread->nullMover = sideToMove;

        ss->continuation = &thread->continuation[0][EMPTY][0];

        MakeNullMove(pos);
        int score = -AlphaBeta(thread, ss+1, -beta, -alpha, nullDepth, !cutnode);
        TakeNullMove(pos);

        thread->nullMover = nullMoverTemp;

        // Cutoff
        if (score >= beta)
            // Don't return unproven terminal win scores
            return score >= TBWIN_IN_MAX ? beta : score;
    }

    int probCutBeta = beta + 200;

    // ProbCut
    if (   depth >= 5
        && !(   ttHit
             && ttBound & BOUND_UPPER
             && ttScore < probCutBeta)) {

        InitNoisyMP(&mp, thread, ss, NOMOVE);

        Move move;
        while ((move = NextMove(&mp))) {

            if (mp.stage > NOISY_GOOD) break;

            if (!MakeMove(pos, move)) continue;

            ss->continuation = &thread->continuation[moveIsCapture(move)][piece(move)][toSq(move)];

            // See if a quiescence search beats the threshold
            int score = -Quiescence(thread, ss+1, -probCutBeta, -probCutBeta+1);

            // If it did, do a proper search with reduced depth
            if (score >= probCutBeta)
                score = -AlphaBeta(thread, ss+1, -probCutBeta, -probCutBeta+1, depth-4, !cutnode);

            TakeMove(pos);

            // Cut if the reduced depth search beats the threshold
            if (score >= probCutBeta)
                return score;
        }
    }

move_loop:

    InitNormalMP(&mp, thread, ss, depth, ttMove, ss->killers[0], ss->killers[1]);

    Move quiets[32] = { 0 };
    Move noisys[32] = { 0 };
    Move bestMove = NOMOVE;
    int moveCount = 0, quietCount = 0, noisyCount = 0;
    int score = -INFINITE;

    Color opponent = !sideToMove;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        if (move == ss->excluded) continue;
        if (root && AlreadySearchedMultiPV(thread, move)) continue;
        if (root && NotInSearchMoves(move)) continue;

        bool quiet = moveIsQuiet(move);

        ss->histScore = GetHistory(thread, ss, move);

        // Misc pruning
        if (  !root
            && thread->doPruning
            && bestScore > -TBWIN_IN_MAX) {

            Depth lmrDepth = depth - Reductions[quiet][MIN(31, depth)][MIN(31, moveCount)];

            // Late move pruning
            if (moveCount > (3 + 2 * depth * depth) / (2 - improving))
                break;

            // Quiet late move pruning
            if (quiet && moveCount > (1 + depth * depth) / (2 - improving)) {
                mp.onlyNoisy = true;
                continue;
            }

            // History pruning
            if (lmrDepth < 3 && ss->histScore < -1024 * depth)
                continue;

            // SEE pruning
            if (lmrDepth < 7 && !SEE(pos, move, quiet ? -50 * depth : -90 * depth))
                continue;
        }

        TTPrefetch(KeyAfter(pos, move));

        // Make the move, skipping to the next if illegal
        if (!MakeMove(pos, move)) continue;

        moveCount++;

        Depth extension = 0;

        // Singular extension
        if (   depth > 8
            && move == ttMove
            && !ss->excluded
            && ttDepth > depth - 3
            && ttBound != BOUND_UPPER
            && abs(ttScore) < TBWIN_IN_MAX / 4
            && !root) {

            // ttMove has been made to check legality
            TakeMove(pos);

            // Search to reduced depth with a zero window a bit lower than ttScore
            int singularBeta = ttScore - depth * 2;
            ss->excluded = move;
            score = AlphaBeta(thread, ss, singularBeta-1, singularBeta, depth/2, cutnode);
            ss->excluded = NOMOVE;

            // Extend as this move seems forced
            if (score < singularBeta)
                extension = 1;
            else if (singularBeta >= beta)
                return singularBeta;

            // Replay ttMove
            MakeMove(pos, move);
        }

        // Extend when in check
        if (inCheck && !root)
            extension = 1;

        // If alpha > 0 and we take back our last move, opponent can do the same
        // and get a fail high by repetition
        if (   pos->rule50 >= 3
            && pos->histPly >= 3
            && alpha > 0
            // The current move has been made and is -1, 2 back is then -3
            && fromSq(move) == toSq(history(-3).move)
            && toSq(move) == fromSq(history(-3).move)) {

            score = 0;
            (ss+1)->pv.length = 0;
            goto skip_search;
        }

        ss->continuation = &thread->continuation[moveIsCapture(move)][piece(move)][toSq(move)];

        const Depth newDepth = depth - 1 + extension;

        bool doFullDepthSearch;

        // Reduced depth zero-window search
        if (   depth > 2
            && moveCount > 1 + pvNode + !ttMove + root
            && thread->doPruning) {

            // Base reduction
            int r = Reductions[quiet][MIN(31, depth)][MIN(31, moveCount)];
            // Adjust reduction by move history
            r -= ss->histScore / 8192;
            // Reduce less in pv nodes
            r -= pvNode;
            // Reduce less when improving
            r -= improving;
            // Reduce less for killers
            r -= move == mp.kill1 || move == mp.kill2;
            // Reduce more for the side that was last null moved against
            r += opponent == thread->nullMover;
            // Reduce quiets more if ttMove is a capture
            r += quiet && moveIsCapture(ttMove);
            // Reduce more when opponent has few pieces
            r += pos->nonPawnCount[opponent] < 2;
            // Reduce more in cut nodes
            r += cutnode;

            // Depth after reductions, avoiding going straight to quiescence as well as extending
            Depth lmrDepth = CLAMP(newDepth - r, 1, newDepth);

            score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, lmrDepth, true);

            doFullDepthSearch = score > alpha && lmrDepth < newDepth;
        } else
            doFullDepthSearch = !pvNode || moveCount > 1;

        // Full depth zero-window search
        if (doFullDepthSearch)
            score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, newDepth, !cutnode);

        // Full depth alpha-beta window search
        if (pvNode && ((score > alpha && score < beta) || moveCount == 1))
            score = -AlphaBeta(thread, ss+1, -beta, -alpha, newDepth, false);

skip_search:

        // Undo the move
        TakeMove(pos);

        // New best move
        if (score > bestScore) {
            bestScore = score;

            // Update PV
            if ((score > alpha && pvNode) || (root && moveCount == 1)) {
                ss->pv.length = 1 + (ss+1)->pv.length;
                ss->pv.line[0] = move;
                memcpy(ss->pv.line+1, (ss+1)->pv.line, sizeof(int) * (ss+1)->pv.length);
            }

            // If score beats alpha we update alpha
            if (score > alpha) {
                alpha = score;
                bestMove = move;

                // If score beats beta we have a cutoff
                if (score >= beta) {
                    UpdateHistory(thread, ss, bestMove, depth, quiets, quietCount, noisys, noisyCount);
                    break;
                }
            }
        }

        // Remember attempted moves to adjust their history scores
        if (quiet && quietCount < 32)
            quiets[quietCount++] = move;
        else if (!quiet && noisyCount < 32)
            noisys[noisyCount++] = move;
    }

    // Checkmate or stalemate
    if (!moveCount)
        bestScore =  ss->excluded ? alpha
                   : inCheck      ? -MATE + ss->ply
                                  : 0;

    // Make sure score isn't above the max score given by TBs
    bestScore = MIN(bestScore, maxScore);

    // Store in TT
    if (!ss->excluded && (!root || !thread->multiPV))
        StoreTTEntry(tte, key, bestMove, ScoreToTT(bestScore, ss->ply), depth,
                       bestScore >= beta  ? BOUND_LOWER
                     : pvNode && bestMove ? BOUND_EXACT
                                          : BOUND_UPPER);

    return bestScore;
}

// Aspiration window
static void AspirationWindow(Thread *thread, Stack *ss) {

    Position *pos = &thread->pos;

    const bool mainThread = thread->index == 0;
    const int multiPV = thread->multiPV;
    int depth = thread->depth;

    const int initialWindow = 12;
    int delta = 16;

    int alpha = -INFINITE;
    int beta  =  INFINITE;

    // Shrink the window after the first few iterations
    if (depth > 6) {
        int prevScore = thread->rootMoves[multiPV].score;

        alpha = MAX(prevScore - initialWindow, -INFINITE);
        beta  = MIN(prevScore + initialWindow,  INFINITE);

        int x = CLAMP(prevScore / 2, -35, 35);
        pos->trend = sideToMove == WHITE ? S(x, x/2) : -S(x, x/2);
    }

    // Repeatedly search and adjust the window until the score is inside the window
    while (true) {

        if (alpha < -3500) alpha = -INFINITE;
        if (beta  >  3500) beta  =  INFINITE;

        thread->doPruning =
            Limits.infinite ? TimeSince(Limits.start) > 1000
                            :   TimeSince(Limits.start) >= Limits.optimalUsage / 64
                             || depth > 2 + Limits.optimalUsage / 256;

        int score = AlphaBeta(thread, ss, alpha, beta, depth, false);

        thread->rootMoves[multiPV].score = score;
        memcpy(&thread->rootMoves[multiPV].pv, &ss->pv, sizeof(PV));

        // Give an update when failing high/low in longer searches
        if (   mainThread
            && Limits.multiPV == 1
            && (score <= alpha || score >= beta)
            && TimeSince(Limits.start) > 3000)
            PrintThinking(thread, alpha, beta);

        // Failed low, relax lower bound and search again
        if (score <= alpha) {
            alpha = MAX(alpha - delta, -INFINITE);
            beta  = (alpha + 3 * beta) / 4;
            depth = thread->depth;

        // Failed high, relax upper bound and search again
        } else if (score >= beta) {
            beta = MIN(beta + delta, INFINITE);
            depth = MAX(1, depth - (abs(score) < TBWIN_IN_MAX));

        // Score inside the window
        } else {
            if (multiPV == 0)
                thread->uncertain = ss->pv.line[0] != thread->rootMoves[0].move;
            thread->rootMoves[multiPV].move = ss->pv.line[0];
            return;
        }

        delta += delta * 2 / 3;
    }
}

// Iterative deepening
static void *IterativeDeepening(void *voidThread) {

    Thread *thread = voidThread;
    Position *pos = &thread->pos;
    Stack *ss = thread->ss+SS_OFFSET;
    bool mainThread = thread->index == 0;
    int multiPV = MIN(Limits.multiPV, thread->rootMoveCount);

    // Iterative deepening
    while (++thread->depth <= (mainThread ? Limits.depth : MAX_PLY)) {

        // Jump here and return if we run out of allocated time mid-search
        if (setjmp(thread->jumpBuffer)) break;

        // Search position, using aspiration windows for higher depths
        for (thread->multiPV = 0; thread->multiPV < multiPV; ++thread->multiPV)
            AspirationWindow(thread, ss);

        // Sort root moves so they are printed in the right order in multi-pv mode
        SortRootMoves(thread, multiPV);

        // Only the main thread concerns itself with the rest
        if (!mainThread) continue;

        // Print thinking info
        PrintThinking(thread, -INFINITE, INFINITE);

        // Stop searching after finding a short enough mate
        if (MATE - abs(thread->rootMoves[0].score) <= 2 * abs(Limits.mate)) break;

        // Limit time spent on forced moves
        if (thread->rootMoveCount == 1 && Limits.timelimit && !Limits.movetime)
            Limits.optimalUsage = MIN(500, Limits.optimalUsage);

        // If an iteration finishes after optimal time usage, stop the search
        if (   Limits.timelimit
            && !thread->uncertain
            && TimeSince(Limits.start) > Limits.optimalUsage)
            break;

        // Clear key history for seldepth calculation
        for (int i = 1; i < MAX_PLY; ++i)
            history(i).key = 0;
    }

    return NULL;
}

// Root of search
void *SearchPosition(void *pos) {

    SEARCH_STOPPED = false;

    InitTimeManagement();
    TTNewSearch();
    PrepareSearch(pos, Limits.searchmoves);
    bool threadsSpawned = false;

    // Probe TBs for a move if already in a TB position
    if (SyzygyMove(pos)) goto conclusion;

    // Probe noobpwnftw's Chess Cloud Database
    if (   (!Limits.timelimit || Limits.maxUsage > 2000)
        && ProbeNoob(pos)) goto conclusion;

    // Start helper threads and begin searching
    threadsSpawned = true;
    StartHelpers(IterativeDeepening);
    IterativeDeepening(&threads[0]);

conclusion:

    // Wait for 'stop' in infinite search
    if (Limits.infinite) Wait(&ABORT_SIGNAL);

    // Signal helper threads to stop and wait for them to finish
    ABORT_SIGNAL = true;
    if (threadsSpawned)
        WaitForHelpers();

    // Print conclusion
    PrintConclusion(threads);

    SEARCH_STOPPED = true;
    Wake();

    return NULL;
}
