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
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pyrrhic/tbprobe.h"
#include "noobprobe/noobprobe.h"
#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "movepicker.h"
#include "time.h"
#include "threads.h"
#include "transposition.h"
#include "search.h"
#include "syzygy.h"
#include "uci.h"


int Reductions[32][32];

SearchLimits Limits;
volatile bool ABORT_SIGNAL = false;
bool noobbook = false;


// Initializes the late move reduction array
CONSTR InitReductions() {

    for (int depth = 1; depth < 32; ++depth)
        for (int moves = 1; moves < 32; ++moves)
            Reductions[depth][moves] = 0.75 + log(depth) * log(moves) / 2.25;
}

// Check if current position is a repetition
static bool IsRepetition(const Position *pos) {

    // Compare current posKey to posKeys in history, skipping
    // opponents turns as that wouldn't be a repetition
    for (int i = 4; i <= pos->rule50 && i <= pos->histPly; i += 2)
        if (pos->key == history(-i).posKey)
            return true;

    return false;
}

INLINE bool PawnOn7th(const Position *pos) {
    return colorPieceBB(sideToMove, PAWN) & RankBB[RelativeRank(sideToMove, RANK_7)];
}

// Dynamic delta pruning margin
static int QuiescenceDeltaMargin(const Position *pos) {

    // Optimistic we can improve our position by a pawn without capturing anything,
    // or if we have a pawn on the 7th we can hope to improve by a queen instead
    const int DeltaBase = PawnOn7th(pos) ? 1400 : 110;

    // Look for possible captures on the board
    const Bitboard enemy = colorBB(!sideToMove);

    // Find the most valuable piece we could take and add to our base
    return DeltaBase + ((enemy & pieceBB(QUEEN )) ? 1400
                      : (enemy & pieceBB(ROOK  )) ? 670
                      : (enemy & pieceBB(BISHOP)) ? 460
                      : (enemy & pieceBB(KNIGHT)) ? 437
                                                  : 110);
}

// Quiescence
static int Quiescence(Thread *thread, int alpha, const int beta) {

    Position *pos = &thread->pos;
    MovePicker mp;
    MoveList list;

    // Check time situation
    if (OutOfTime(thread) || ABORT_SIGNAL)
        longjmp(thread->jumpBuffer, true);

    // Update node count and selective depth
    thread->nodes++;
    if (pos->ply > thread->seldepth)
        thread->seldepth = pos->ply;

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

    int futility = score + 60;

    InitNoisyMP(&mp, &list, thread);

    int bestScore = score;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        if (   futility + PieceValue[EG][pieceOn(toSq(move))] <= alpha
            && !(  PieceTypeOf(pieceOn(fromSq(move))) == PAWN
                && RelativeRank(sideToMove, RankOf(toSq(move))) > 5))
            continue;

        // Recursively search the positions after making the moves, skipping illegal ones
        if (!MakeMove(pos, move)) continue;
        score = -Quiescence(thread, -beta, -alpha);
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
static int AlphaBeta(Thread *thread, int alpha, int beta, Depth depth, PV *pv) {

    Position *pos = &thread->pos;
    MovePicker mp;
    MoveList list;

    const bool pvNode = alpha != beta - 1;
    const bool root   = pos->ply == 0;

    PV pvFromHere;
    pv->length = 0;

    // Check time situation
    if (OutOfTime(thread) || ABORT_SIGNAL)
        longjmp(thread->jumpBuffer, true);

    // Early exits
    if (!root) {

        // Position is drawn
        if (IsRepetition(pos) || pos->rule50 >= 100)
            return 0;

        // Max depth reached
        if (pos->ply >= MAXDEPTH)
            return EvalPosition(pos);

        // Mate distance pruning
        alpha = MAX(alpha, -MATE + pos->ply);
        beta  = MIN(beta,   MATE - pos->ply - 1);
        if (alpha >= beta)
            return alpha;
    }

    // Extend search if in check
    const bool inCheck = KingAttacked(pos, sideToMove);
    if (inCheck && depth + 1 < MAXDEPTH) depth++;

    // Quiescence at the end of search
    if (depth <= 0)
        return Quiescence(thread, alpha, beta);

    // Update node count and selective depth
    thread->nodes++;
    if (pos->ply > thread->seldepth)
        thread->seldepth = pos->ply;    

    // Probe transposition table
    bool ttHit;
    Key posKey = pos->key;
    TTEntry *tte = ProbeTT(posKey, &ttHit);

    Move ttMove = ttHit ? tte->move : NOMOVE;
    int ttScore = ttHit ? ScoreFromTT(tte->score, pos->ply) : NOSCORE;

    // Trust the ttScore in non-pvNodes as long as the entry depth is equal or higher
    if (!pvNode && ttHit && tte->depth >= depth) {

        assert(ValidBound(tte->bound));
        assert(ValidDepth(tte->depth));
        assert(ValidScore(ttScore));

        // Check if ttScore causes a cutoff
        if (ttScore >= beta ? tte->bound & BOUND_LOWER
                            : tte->bound & BOUND_UPPER)

            return ttScore;
    }

    int bestScore = -INFINITE;

    // Probe syzygy TBs
    int score, bound;
    if (ProbeWDL(pos, &score, &bound)) {

        thread->tbhits++;

        if (    bound == BOUND_EXACT
            || (bound == BOUND_LOWER ? score >= beta : score <= alpha)) {

            StoreTTEntry(tte, posKey, NOMOVE, ScoreToTT(score, pos->ply), MAXDEPTH-1, bound);
            return score;
        }

        if (pvNode && bound == BOUND_LOWER) {
            bestScore = score;
            alpha = MAX(alpha, score);
        }
    }

    // Do a static evaluation for pruning considerations
    int eval = history(0).eval = inCheck          ? NOSCORE
                               : lastMoveNullMove ? -history(-1).eval + 2 * Tempo
                                                  : EvalPosition(pos);

    // Use ttScore as eval if it is more informative
    if (   ttScore != NOSCORE
        && (tte->bound & (ttScore > eval ? BOUND_LOWER : BOUND_UPPER)))
        eval = ttScore;

    // Improving if not in check, and current eval is higher than 2 plies ago
    bool improving = !inCheck && pos->ply >= 2 && eval > history(-2).eval;

    // Skip pruning while in check and at the root
    if (inCheck || root)
        goto move_loop;

    // Razoring
    if (  !pvNode
        && depth < 2
        && eval + 640 < alpha)
        return Quiescence(thread, alpha, beta);

    // Reverse Futility Pruning
    if (  !pvNode
        && depth < 7
        && eval - 225 * depth + 100 * improving >= beta)
        return eval;

    // Null Move Pruning
    if (  !pvNode
        && depth >= 3
        && eval >= beta
        && history( 0).eval >= beta
        && history(-1).move != NOMOVE
        && pos->nonPawnCount[sideToMove] > 0) {

        int R = 3 + depth / 5 + MIN(3, (eval - beta) / 256);

        MakeNullMove(pos);
        score = -AlphaBeta(thread, -beta, -beta + 1, depth - R, &pvFromHere);
        TakeNullMove(pos);

        // Cutoff
        if (score >= beta) {
            // Don't return unproven terminal win scores
            return score >= TBWIN_IN_MAX ? beta : score;
        }
    }

    // ProbCut
    if (  !pvNode
        && depth >= 5
        && abs(beta) < TBWIN_IN_MAX
        && !(   ttHit
             && tte->bound & BOUND_UPPER
             && ttScore < beta)) {

        int pbBeta = beta + 200;

        MovePicker pbMP;
        MoveList pbList;
        InitNoisyMP(&pbMP, &pbList, thread);

        Move pbMove;
        while ((pbMove = NextMove(&pbMP))) {

            if (!MakeMove(pos, pbMove)) continue;

            int pbScore = -Quiescence(thread, -pbBeta, -pbBeta+1);

            if (pbScore >= pbBeta)
                pbScore = -AlphaBeta(thread, -pbBeta, -pbBeta+1, depth-4, &pvFromHere);

            TakeMove(pos);

            if (pbScore >= pbBeta)
                return pbScore;
        }
    }

    // Internal iterative deepening based on Rebel's idea
    if (depth >= 4 && !ttMove)
        depth--;

move_loop:

    InitNormalMP(&mp, &list, thread, ttMove, killer1, killer2);

    Move quiets[32] = { 0 };

    const int oldAlpha = alpha;
    int moveCount = 0, quietCount = 0;
    Move bestMove = NOMOVE;
    score = -INFINITE;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        bool quiet = moveIsQuiet(move);

        // Late move pruning
        if (  !pvNode
            && bestScore > -TBWIN_IN_MAX
            && quietCount > (3 + 2 * depth * depth) / (2 - improving)) {
            mp.onlyNoisy = true;
            continue;
        }

        __builtin_prefetch(GetEntry(KeyAfter(pos, move)));

        // Make the move, skipping to the next if illegal
        if (!MakeMove(pos, move)) continue;

        // Increment counts
        moveCount++;
        if (quiet && quietCount < 32)
            quiets[quietCount++] = move;

        const Depth newDepth = depth - 1;

        bool doLMR = depth > 2 && moveCount > (2 + pvNode);

        // Reduced depth zero-window search
        if (doLMR) {
            // Base reduction
            int R = Reductions[MIN(31, depth)][MIN(31, moveCount)];
            // Reduce less in pv nodes
            R -= pvNode;
            // Reduce less when improving
            R -= improving;
            // Reduce more for quiets
            R += quiet;

            // Depth after reductions, avoiding going straight to quiescence
            Depth RDepth = CLAMP(newDepth - R, 1, newDepth - 1);

            score = -AlphaBeta(thread, -alpha - 1, -alpha, RDepth, &pvFromHere);
        }
        // Full depth zero-window search
        if (doLMR ? score > alpha : !pvNode || moveCount > 1)
            score = -AlphaBeta(thread, -alpha - 1, -alpha, newDepth, &pvFromHere);

        // Full depth alpha-beta window search
        if (pvNode && ((score > alpha && score < beta) || moveCount == 1))
            score = -AlphaBeta(thread, -beta, -alpha, newDepth, &pvFromHere);

        // Undo the move
        TakeMove(pos);

        // Found a new best move in this position
        if (score > bestScore) {

            bestScore = score;
            bestMove  = move;

            // Update the Principle Variation
            if ((score > alpha && pvNode) || (root && moveCount == 1)) {
                pv->length = 1 + pvFromHere.length;
                pv->line[0] = move;
                memcpy(pv->line + 1, pvFromHere.line, sizeof(int) * pvFromHere.length);
            }

            // If score beats alpha we update alpha
            if (score > alpha) {

                alpha = score;

                // Update search history
                if (quiet && depth > 1)
                    thread->history[pieceOn(fromSq(bestMove))][toSq(bestMove)] += depth * depth;

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

    // Lower history scores of moves that failed to produce a cut
    if (bestScore >= beta && moveIsQuiet(bestMove))
        for (int i = 0; i < quietCount; ++i) {
            Move m = quiets[i];
            if (m == bestMove) continue;
            thread->history[pieceOn(fromSq(m))][toSq(m)] -= depth * depth;
        }

    // Checkmate or stalemate
    if (!moveCount)
        return inCheck ? -MATE + pos->ply : 0;

    // Store in TT
    const int flag = bestScore >= beta ? BOUND_LOWER
                   : alpha != oldAlpha ? BOUND_EXACT
                                       : BOUND_UPPER;

    StoreTTEntry(tte, posKey, bestMove, ScoreToTT(bestScore, pos->ply), depth, flag);

    assert(alpha >= oldAlpha);
    assert(ValidScore(alpha));
    assert(ValidScore(bestScore));

    return bestScore;
}

// Aspiration window
static int AspirationWindow(Thread *thread) {

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

        score = AlphaBeta(thread, alpha, beta, depth, &thread->pv);

        // Give an update when done, or after each iteration in long searches
        if (mainThread && (   (score > alpha && score < beta)
                           || TimeSince(Limits.start) > 3000))
            PrintThinking(thread, score, alpha, beta);

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
        } else
            return score;

        delta += delta * 2 / 3;
    }
}

// Iterative deepening
static void *IterativeDeepening(void *voidThread) {

    Thread *thread = voidThread;
    bool mainThread = thread->index == 0;

    // Iterative deepening
    for (thread->depth = 1; thread->depth <= Limits.depth; ++thread->depth) {

        // Jump here and return if we run out of allocated time mid-search
        if (setjmp(thread->jumpBuffer)) break;

        // Search position, using aspiration windows for higher depths
        thread->score = AspirationWindow(thread);

        // Only the main thread concerns itself with the rest
        if (!mainThread) continue;

        bool uncertain = thread->pv.line[0] != thread->bestMove;

        // Save bestMove and ponderMove before overwriting the pv next iteration
        thread->bestMove   = thread->pv.line[0];
        thread->ponderMove = thread->pv.length > 1 ? thread->pv.line[1] : NOMOVE;

        if (   Limits.timelimit
            && TimeSince(Limits.start) > Limits.optimalUsage * (1 + uncertain))
            break;

        thread->seldepth = 0;
    }

    return NULL;
}

// Get ready to start a search
static void PrepareSearch(Position *pos, Thread *threads) {

    // Setup threads for a new search
    for (int i = 0; i < threads->count; ++i) {
        memset(&threads[i], 0, offsetof(Thread, pos));
        memcpy(&threads[i].pos, pos, sizeof(Position));
    }

    // Mark TT as used
    TT.dirty = true;
}

// Root of search
void SearchPosition(Position *pos, Thread *threads) {

    InitTimeManagement();

    PrepareSearch(pos, threads);

    bool threadsSpawned = false;

    // Probe TBs for a move if already in a TB position
    if (RootProbe(pos, threads)) goto conclusion;

    // Probe noobpwnftw's Chess Cloud Database
    if (   noobbook && (!Limits.timelimit || Limits.maxUsage > 2000)
        && failedQueries < 3 && ProbeNoob(pos, threads)) goto conclusion;

    // Make extra threads and begin searching
    threadsSpawned = true;
    for (int i = 1; i < threads->count; ++i)
        pthread_create(&threads->pthreads[i], NULL, &IterativeDeepening, &threads[i]);
    IterativeDeepening(&threads[0]);

conclusion:

    // Wait for 'stop' in infinite search
    if (Limits.infinite) Wait(threads, &ABORT_SIGNAL);

    // Signal any extra threads to stop and wait for them
    ABORT_SIGNAL = true;
    if (threadsSpawned)
        for (int i = 1; i < threads->count; ++i)
            pthread_join(threads->pthreads[i], NULL);

    // Print conclusion
    PrintConclusion(threads);
}
