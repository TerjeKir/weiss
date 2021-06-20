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


SearchLimits Limits;
volatile bool ABORT_SIGNAL;

static int Reductions[2][32][32];


// Initializes the late move reduction array
CONSTR InitReductions() {
    for (int depth = 1; depth < 32; ++depth)
        for (int moves = 1; moves < 32; ++moves)
            Reductions[0][depth][moves] = 0.00 + log(depth) * log(moves) / 3.25, // capture
            Reductions[1][depth][moves] = 1.75 + log(depth) * log(moves) / 2.25; // quiet
}

// Dynamic delta pruning margin
static int QuiescenceDeltaMargin(const Position *pos) {

    bool pawnOn7th =  colorPieceBB(sideToMove, PAWN)
                    & RankBB[RelativeRank(sideToMove, RANK_7)];

    // Optimistic we can improve our position by a pawn without capturing anything,
    // or if we have a pawn on the 7th we can hope to improve by a queen instead
    const int DeltaBase = pawnOn7th ? 1400 : 110;

    // Find the most valuable piece we could capture and add to our base
    const Bitboard enemy = colorBB(!sideToMove);

    return DeltaBase + (  (enemy & pieceBB(QUEEN )) ? 1400
                        : (enemy & pieceBB(ROOK  )) ?  670
                        : (enemy & pieceBB(BISHOP)) ?  460
                        : (enemy & pieceBB(KNIGHT)) ?  437
                                                    :  110);
}

// Quiescence
static int Quiescence(Thread *thread, Stack *ss, int alpha, const int beta) {

    Position *pos = &thread->pos;
    MovePicker mp;

    // Check time situation
    if (OutOfTime(thread) || ABORT_SIGNAL)
        longjmp(thread->jumpBuffer, true);

    // Do a static evaluation for pruning considerations
    int eval = EvalPosition(pos, thread->pawnCache);

    // If we are at max depth, return static eval
    if (ss->ply >= MAX_PLY)
        return eval;

    // If eval beats beta we assume some move will also beat it
    if (eval >= beta)
        return eval;

    // If eval plus a margin is still below alpha we assume no move will beat it
    if (eval + QuiescenceDeltaMargin(pos) < alpha)
        return alpha;

    // Use eval as a lowerbound if it's above alpha (but below beta)
    if (eval > alpha)
        alpha = eval;

    InitNoisyMP(&mp, thread);

    int futility = eval + 40;
    int bestScore = eval;
    int score;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        // Skip moves SEE deem bad
        if (mp.stage > NOISY_GOOD) break;

        // Futility pruning
        if (   futility + PieceValue[EG][pieceOn(toSq(move))] <= alpha
            && !(   pieceTypeOn(fromSq(move)) == PAWN
                 && RelativeRank(sideToMove, RankOf(toSq(move))) > 5))
            continue;

        if (   futility <= alpha
            && !SEE(pos, move, 1)) {
            bestScore = MAX(bestScore, futility);
            continue;
        }

        // Recursively search the positions after making the moves, skipping illegal ones
        if (!MakeMove(pos, move)) continue;
        score = -Quiescence(thread, ss+1, -beta, -alpha);
        TakeMove(pos);

        // Found a new best move in this position
        if (score > bestScore) {
            bestScore = score;

            // If score beats alpha we update alpha
            if (score > alpha) {
                alpha = score;

                // If score beats beta we have a cutoff
                if (score >= beta)
                    break;
            }
        }
    }

    return bestScore;
}

// Alpha Beta
static int AlphaBeta(Thread *thread, Stack *ss, int alpha, int beta, Depth depth) {

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

    // Extend search if in check
    const bool inCheck = pos->checkers;
    if (inCheck) depth++;

    // Quiescence at the end of search
    if (depth <= 0)
        return Quiescence(thread, ss, alpha, beta);

    // Probe transposition table
    bool ttHit;
    Key key = pos->key ^ (((Key)ss->excluded) << 32);
    TTEntry *tte = ProbeTT(key, &ttHit);

    Move ttMove = ttHit ? tte->move : NOMOVE;
    int ttScore = ttHit ? ScoreFromTT(tte->score, ss->ply) : NOSCORE;

    // Trust TT if not a pvnode and the entry depth is sufficiently high
    if (   !pvNode
        && ttHit
        && tte->depth >= depth
        && (tte->bound & (ttScore >= beta ? BOUND_LOWER : BOUND_UPPER))) {

        if (ttScore >= beta && moveIsQuiet(ttMove))
            HistoryBonus(QuietEntry(ttMove), depth*depth);

        return ttScore;
    }

    int bestScore = -INFINITE;
    int maxScore  =  INFINITE;

    // Probe syzygy TBs
    int tbScore, bound;
    if (ProbeWDL(pos, &tbScore, &bound, ss->ply)) {

        thread->tbhits++;

        if (bound == BOUND_EXACT || (bound == BOUND_LOWER ? tbScore >= beta : tbScore <= alpha)) {
            StoreTTEntry(tte, key, NOMOVE, ScoreToTT(tbScore, ss->ply), MAX_PLY, bound);
            return tbScore;
        }

        if (pvNode) {
            if (bound == BOUND_LOWER)
                bestScore = tbScore,
                alpha = MAX(alpha, tbScore);
            else
                maxScore = tbScore;
        }
    }

    // Do a static evaluation for pruning considerations
    int eval = ss->eval = inCheck          ? NOSCORE
                        : lastMoveNullMove ? -(ss-1)->eval + 2 * Tempo
                                           : EvalPosition(pos, thread->pawnCache);

    // Use ttScore as eval if it is more informative
    if (   ttScore != NOSCORE
        && tte->bound & (ttScore > eval ? BOUND_LOWER : BOUND_UPPER))
        eval = ttScore;

    // Improving if not in check, and current eval is higher than 2 plies ago
    bool improving = !inCheck && eval > (ss-2)->eval;

    // Internal iterative reduction based on Rebel's idea
    if (depth >= 4 && !ttMove)
        depth--;

    // Skip pruning in check, at root, during early iterations, and when proving singularity
    if (inCheck || pvNode || !thread->doPruning || ss->excluded)
        goto move_loop;

    // Razoring
    if (   depth < 2
        && eval + 640 < alpha)
        return Quiescence(thread, ss, alpha, beta);

    // Reverse Futility Pruning
    if (   depth < 7
        && eval - 225 * depth + 100 * improving >= beta)
        return eval;

    // Null Move Pruning
    if (   depth >= 3
        && eval >= beta
        && ss->eval >= beta
        && history(-1).move != NOMOVE
        && pos->nonPawnCount[sideToMove] > (depth > 8)) {

        int R = 3 + depth / 5 + MIN(3, (eval - beta) / 256);

        Color nullMoverTemp = thread->nullMover;
        thread->nullMover = sideToMove;

        MakeNullMove(pos);
        int score = -AlphaBeta(thread, ss+1, -beta, -beta+1, depth-R);
        TakeNullMove(pos);

        thread->nullMover = nullMoverTemp;

        // Cutoff
        if (score >= beta)
            // Don't return unproven terminal win scores
            return score >= TBWIN_IN_MAX ? beta : score;
    }

    // ProbCut
    if (   depth >= 5
        && abs(beta) < TBWIN_IN_MAX
        && !(   ttHit
             && tte->bound & BOUND_UPPER
             && ttScore < beta)) {

        int threshold = beta + 200;

        InitNoisyMP(&mp, thread);

        Move move;
        while ((move = NextMove(&mp))) {

            if (!MakeMove(pos, move)) continue;

            // See if a quiescence search beats pbBeta
            int pbScore = -Quiescence(thread, ss+1, -threshold, -threshold+1);

            // If it did, do a proper search with reduced depth
            if (pbScore >= threshold)
                pbScore = -AlphaBeta(thread, ss+1, -threshold, -threshold+1, depth-4);

            TakeMove(pos);

            // Cut if the reduced depth search beats pbBeta
            if (pbScore >= threshold)
                return pbScore;
        }
    }

move_loop:

    InitNormalMP(&mp, thread, ttMove, ss->killers[0], ss->killers[1]);

    Move quiets[32] = { 0 };
    Move noisys[32] = { 0 };
    Move bestMove = NOMOVE;
    const int oldAlpha = alpha;
    int moveCount = 0, quietCount = 0, noisyCount = 0;
    int score = -INFINITE;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        if (move == ss->excluded) continue;

        bool quiet = moveIsQuiet(move);

        int histScore = mp.list.moves[mp.list.next-1].score;

        // Misc pruning
        if (  !root
            && thread->doPruning
            && bestScore > -TBWIN_IN_MAX) {

            // Late move pruning
            if (moveCount > (3 + 2 * depth * depth) / (2 - improving))
                break;

            // Quiet late move pruning
            if (quiet && moveCount > (3 + depth * depth) / (2 - improving)) {
                mp.onlyNoisy = true;
                continue;
            }

            // History pruning
            if (depth < 3 && histScore < -2024 * depth)
                continue;

            // SEE pruning
            if (depth < 7 && !SEE(pos, move, -50 * depth))
                continue;
        }

        __builtin_prefetch(GetEntry(KeyAfter(pos, move)));

        // Make the move, skipping to the next if illegal
        if (!MakeMove(pos, move)) continue;

        Depth extension = 0;

        // Singular extension
        if (   depth > 8
            && move == ttMove
            && !ss->excluded
            && tte->depth > depth - 3
            && tte->bound != BOUND_UPPER
            && abs(ttScore) < TBWIN_IN_MAX / 4
            && !root) {

            // ttMove has been made to check legality
            TakeMove(pos);

            // Search to reduced depth with a zero window a bit lower than ttScore
            int threshold = ttScore - depth * 2;
            ss->excluded = move;
            score = AlphaBeta(thread, ss, threshold-1, threshold, depth/2);
            ss->excluded = NOMOVE;

            // Extend as this move seems forced
            if (score < threshold)
                extension = 1;

            // Replay ttMove
            MakeMove(pos, move);
        }

        // Increment counts
        moveCount++;
        if (quiet && quietCount < 32)
            quiets[quietCount++] = move;
        else if (noisyCount < 32)
            noisys[noisyCount++] = move;

        // If alpha > 0 and we take back our last move, opponent can do the same
        // and get a fail high by repetition
        if (   pos->rule50 >= 3
            && pos->histPly >= 2
            && alpha > 0
            // The current move has been made and is -1, 2 back is then -3
            && fromSq(move) == toSq(history(-3).move)
            && toSq(move) == fromSq(history(-3).move)) {

            score = 0;
            (ss+1)->pv.length = 0;
            goto skip_search;
        }

        const Depth newDepth = depth - 1 + extension;

        bool doFullDepthSearch;

        // Reduced depth zero-window search
        if (   depth > 2
            && moveCount > (2 + pvNode)
            && thread->doPruning) {

            // Base reduction
            int r = Reductions[quiet][MIN(31, depth)][MIN(31, moveCount)];
            // Reduce less in pv nodes
            r -= pvNode;
            // Reduce less when improving
            r -= improving;
            // Reduce less for killers
            r -= mp.stage == KILLER1 || mp.stage == KILLER2;
            // Reduce more for the side that last null moved
            r += sideToMove == thread->nullMover;

            r -= histScore / 6000;

            // Depth after reductions, avoiding going straight to quiescence
            Depth lmrDepth = CLAMP(newDepth - r, 1, newDepth);

            score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, lmrDepth);

            doFullDepthSearch = score > alpha && lmrDepth < newDepth;
        } else
            doFullDepthSearch = !pvNode || moveCount > 1;

        // Full depth zero-window search
        if (doFullDepthSearch)
            score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, newDepth);

        // Full depth alpha-beta window search
        if (pvNode && ((score > alpha && score < beta) || moveCount == 1))
            score = -AlphaBeta(thread, ss+1, -beta, -alpha, newDepth);

skip_search:

        // Undo the move
        TakeMove(pos);

        // New best move
        if (score > bestScore) {
            bestScore = score;
            bestMove  = move;

            // Update PV
            if ((score > alpha && pvNode) || (root && moveCount == 1)) {
                ss->pv.length = 1 + (ss+1)->pv.length;
                ss->pv.line[0] = move;
                memcpy(ss->pv.line+1, (ss+1)->pv.line, sizeof(int) * (ss+1)->pv.length);
            }

            // If score beats alpha we update alpha
            if (score > alpha) {
                alpha = score;

                // If score beats beta we have a cutoff
                if (score >= beta)
                    break;
            }
        }
    }

    // Update history if a quiet move causes a beta cutoff
    if (bestScore >= beta)
        UpdateHistory(thread, ss, bestMove, depth, quiets, quietCount, noisys, noisyCount);

    // Checkmate or stalemate
    if (!moveCount)
        return ss->excluded ? alpha
             : inCheck      ? -MATE + ss->ply
                            : 0;

    bestScore = MIN(bestScore, maxScore);

    // Store in TT
    const int flag = bestScore >= beta ? BOUND_LOWER
                   : alpha != oldAlpha ? BOUND_EXACT
                                       : BOUND_UPPER;
    if (!ss->excluded)
        StoreTTEntry(tte, key, bestMove, ScoreToTT(bestScore, ss->ply), depth, flag);

    return bestScore;
}

// Aspiration window
static int AspirationWindow(Thread *thread, Stack *ss) {

    bool mainThread = thread->index == 0;
    int score = thread->score;
    int depth = thread->depth;

    const int initialWindow = 12;
    int delta = 16;

    int alpha = -INFINITE;
    int beta  =  INFINITE;

    // Shrink the window at higher depths
    if (depth > 6)
        alpha = MAX(score - initialWindow, -INFINITE),
        beta  = MIN(score + initialWindow,  INFINITE);

    // Search with aspiration window until the result is inside the window
    while (true) {

        if (alpha < -3500) alpha = -INFINITE;
        if (beta  >  3500) beta  =  INFINITE;

        thread->doPruning =
            Limits.infinite ? TimeSince(Limits.start) > 1000
                            :   TimeSince(Limits.start) >= Limits.optimalUsage / 64
                             || depth > 2 + Limits.optimalUsage / 256;

        score = AlphaBeta(thread, ss, alpha, beta, depth);

        // Give an update when done, or after each iteration in long searches
        if (mainThread && (   (score > alpha && score < beta)
                           || TimeSince(Limits.start) > 3000))
            PrintThinking(thread, ss, score, alpha, beta);

        // Failed low, relax lower bound and search again
        if (score <= alpha) {
            alpha = MAX(alpha - delta, -INFINITE);
            beta  = (alpha + beta) / 2;
            depth = thread->depth;

        // Failed high, relax upper bound and search again
        } else if (score >= beta) {
            beta = MIN(beta + delta, INFINITE);
            depth -= (abs(score) < TBWIN_IN_MAX);

        // Score within the bounds is accepted as correct
        } else return score;

        delta += delta * 2 / 3;
    }
}

// Iterative deepening
static void *IterativeDeepening(void *voidThread) {

    Thread *thread = voidThread;
    Position *pos = &thread->pos;
    Stack *ss = thread->ss+SS_OFFSET;
    bool mainThread = thread->index == 0;

    // Iterative deepening
    while (++thread->depth <= (mainThread ? Limits.depth : MAX_PLY)) {

        // Jump here and return if we run out of allocated time mid-search
        if (setjmp(thread->jumpBuffer)) break;

        // Search position, using aspiration windows for higher depths
        thread->score = AspirationWindow(thread, ss);

        // Only the main thread concerns itself with the rest
        if (!mainThread) continue;

        bool uncertain = ss->pv.line[0] != thread->bestMove;

        // Save bestMove before overwriting the pv next iteration
        thread->bestMove = ss->pv.line[0];

        // Stop searching after finding a short enough mate
        if (MATE - abs(thread->score) <= 2 * abs(Limits.mate)) break;

        // If an iteration finishes after optimal time usage, stop the search
        if (   Limits.timelimit
            && TimeSince(Limits.start) > Limits.optimalUsage * (1 + uncertain))
            break;

        // Clear key history for seldepth calculation
        for (int i = 1; i < MAX_PLY; ++i)
            history(i).key = 0;
    }

    return NULL;
}

// Root of search
void *SearchPosition(void *pos) {

    InitTimeManagement();
    PrepareSearch(pos);
    bool threadsSpawned = false;

    // Probe TBs for a move if already in a TB position
    if (RootProbe(pos)) goto conclusion;

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

    return NULL;
}
