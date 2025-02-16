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
atomic_bool ABORT_SIGNAL;
atomic_bool SEARCH_STOPPED = true;
atomic_bool Minimal = false;

static int Reductions[2][32][32];


// Initializes the late move reduction array
CONSTR(1) InitReductions() {
    for (int depth = 1; depth < 32; ++depth)
        for (int moves = 1; moves < 32; ++moves)
            Reductions[0][depth][moves] = 0.38 + log(depth) * log(moves) / 3.76, // capture
            Reductions[1][depth][moves] = 2.01 + log(depth) * log(moves) / 2.32; // quiet
}

// Checks whether a move was already searched in multi-pv mode
static bool AlreadySearchedMultiPV(Thread *thread, Move move) {
    for (int i = 0; i < thread->multiPV; ++i)
        if (thread->rootMoves[i].move == move)
            return true;
    return false;
}

// Correct the evaluation based on historic differences between eval and final score
static int CorrectEval(Thread *thread, Stack *ss, int eval, int rule50) {
    int correctedEval = eval + GetCorrectionHistory(thread, ss);
    if (rule50 > 7)
        correctedEval *= (256 - rule50) / 256.0;
    return CLAMP(correctedEval, -TBWIN_IN_MAX + 1, TBWIN_IN_MAX - 1);
}

// Small positive score with some random variance
static int DrawScore(Position *pos) {
    return 8 - (pos->nodes & 0x7);
}

// Update the principal variation with the new move and the continuation
static void UpdatePv(Stack *ss, Move move) {
    ss->pv.length = 1 + (ss+1)->pv.length;
    ss->pv.line[0] = move;
    memcpy(ss->pv.line+1, (ss+1)->pv.line, sizeof(Move) * (ss+1)->pv.length);
}

// Quiescence
static int Quiescence(Thread *thread, Stack *ss, int alpha, int beta) {

    Position *pos = &thread->pos;
    MovePicker mp;
    ss->pv.length = 0;

    const bool pvNode = alpha != beta - 1;
    const bool inCheck = pos->checkers;

    int eval = NOSCORE;
    int futility = -INFINITE;
    int bestScore = -INFINITE;
    int unadjustedEval = NOSCORE;

    // Check time situation
    if (OutOfTime(thread) || loadRelaxed(ABORT_SIGNAL))
        longjmp(thread->jumpBuffer, true);

    // Detect upcoming repetitions
    if (alpha < 0 && HasCycle(pos, ss->ply)) {
        alpha = DrawScore(pos);
        if (alpha >= beta)
            return alpha;
    }

    // Position is drawn by repetition
    if (IsRepetition(pos))
        return DrawScore(pos);

    // Position is drawn by 50 move rule
    if (pos->rule50 >= 100 && (!inCheck || LegalMoveCount(pos) > 0))
        return DrawScore(pos);

    // If we are at max depth, return static eval
    if (ss->ply >= MAX_PLY)
        return EvalPosition(pos, thread->pawnCache);

    // Mate distance pruning
    alpha = MAX(alpha, matedIn(ss->ply));
    beta  = MIN(beta, mateIn(ss->ply + 1));
    if (alpha >= beta)
        return alpha;

    // Probe transposition table
    bool ttHit;
    TTEntry *tte = ProbeTT(pos->key, &ttHit);

    Move ttMove = ttHit ? tte->move : NOMOVE;
    int ttScore = ttHit ? ScoreFromTT(tte->score, ss->ply) : NOSCORE;
    int ttEval  = ttHit ? tte->eval : NOSCORE;
    // Depth ttDepth = tte->depth;
    int ttBound = Bound(tte);

    if (ttMove && !MoveIsPseudoLegal(pos, ttMove))
        ttHit = false, ttMove = NOMOVE, ttScore = NOSCORE, ttEval = NOSCORE;

    // Trust TT if not a pvnode
    if (!pvNode && ttHit && TTScoreIsMoreInformative(ttBound, ttScore, beta))
        return ttScore;

    if (inCheck) goto moveloop;

    // Do a static evaluation for pruning considerations
    eval = (ss-1)->move == NOMOVE ? -(ss-1)->staticEval + 2 * Tempo
         : ttEval != NOSCORE      ? ttEval
                                  : EvalPosition(pos, thread->pawnCache);

    unadjustedEval = eval;
    eval = CorrectEval(thread, ss, eval, pos->rule50);

    // Use ttScore as eval if it is more informative
    if (!isTerminal(ttScore) && TTScoreIsMoreInformative(ttBound, ttScore, eval))
        eval = ttScore;

    // If eval beats beta we assume some move will also beat it
    if (eval >= beta)
        return eval;

    // Use eval as a lower bound if it's above alpha
    if (eval > alpha)
        alpha = eval;

    futility = eval + 165;
    bestScore = eval;

moveloop:

    if (!inCheck) InitNoisyMP(&mp, thread, ss, ttMove);
    else          InitNormalMP(&mp, thread, ss, 0, ttMove, NOMOVE);

    // Move loop
    Move bestMove = NOMOVE;
    Move move;
    while ((move = NextMove(&mp))) {
        if (!MoveIsLegal(pos, move)) continue;

        // Avoid pruning until at least one move avoids a terminal loss score
        if (isLoss(bestScore)) goto search;

        // Only try moves the movepicker deems good
        if (mp.stage > NOISY_GOOD) break;

        // Futility pruning
        if (    futility + PieceValue[EG][capturing(move)] <= alpha
            && !promotion(move)) {
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
        ss->move = move;
        ss->continuation = &thread->continuation[inCheck][moveIsCapture(move)][piece(move)][toSq(move)];
        ss->contCorr = &thread->contCorrHistory[piece(move)][toSq(move)];

        MakeMove(pos, move);
        int score = -Quiescence(thread, ss+1, -beta, -alpha);
        TakeMove(pos);

        // Found a new best move in this position
        if (score > bestScore) {
            bestScore = score;

            // If score beats alpha we update alpha
            if (score > alpha) {
                alpha = score;
                bestMove = move;

                // Update PV
                if (pvNode)
                    UpdatePv(ss, move);

                // If score beats beta we have a cutoff
                if (score >= beta)
                    break;
            }
        }
    }

    // Checkmate
    if (inCheck && bestScore == -INFINITE)
        return matedIn(ss->ply);

    StoreTTEntry(tte, pos->key, bestMove, ScoreToTT(bestScore, ss->ply), unadjustedEval, 0,
                 bestScore >= beta ? BOUND_LOWER : BOUND_UPPER);

    return bestScore;
}

// Alpha Beta
static int AlphaBeta(Thread *thread, Stack *ss, int alpha, int beta, Depth depth, bool cutnode) {

    // Quiescence at the end of search
    if (depth <= 0)
        return Quiescence(thread, ss, alpha, beta);

    Position *pos = &thread->pos;
    MovePicker mp;
    ss->pv.length = 0;
    ss->doubleExtensions = (ss-1)->doubleExtensions;

    const bool pvNode = alpha != beta - 1;
    const bool root = ss->ply == 0;
    const bool inCheck = pos->checkers;

    // Check time situation
    if (OutOfTime(thread) || loadRelaxed(ABORT_SIGNAL))
        longjmp(thread->jumpBuffer, true);

    // Early exits
    if (!root) {

        // Detect upcoming repetitions
        if (alpha < 0 && HasCycle(pos, ss->ply)) {
            alpha = DrawScore(pos);
            if (alpha >= beta)
                return alpha;
        }

        // Position is drawn by repetition
        if (IsRepetition(pos))
            return DrawScore(pos);

        // Position is drawn by 50 move rule
        if (pos->rule50 >= 100 && (!inCheck || LegalMoveCount(pos) > 0))
            return DrawScore(pos);

        // Max depth reached
        if (ss->ply >= MAX_PLY)
            return EvalPosition(pos, thread->pawnCache);

        // Mate distance pruning
        alpha = MAX(alpha, matedIn(ss->ply));
        beta  = MIN(beta, mateIn(ss->ply + 1));
        if (alpha >= beta)
            return alpha;
    }

    // Probe transposition table
    bool ttHit;
    TTEntry *tte = ProbeTT(pos->key, &ttHit);

    Move ttMove = ttHit ? tte->move : NOMOVE;
    int ttScore = ttHit ? ScoreFromTT(tte->score, ss->ply) : NOSCORE;
    int ttEval = ttHit ? tte->eval : NOSCORE;
    Depth ttDepth = tte->depth;
    int ttBound = Bound(tte);

    if (ttMove && (!MoveIsPseudoLegal(pos, ttMove) || ttMove == ss->excluded))
        ttHit = false, ttMove = NOMOVE, ttScore = NOSCORE, ttEval = NOSCORE;

    // Trust TT if not a pvnode and the entry depth is sufficiently high
    if (!pvNode && ttHit && ttDepth >= depth && TTScoreIsMoreInformative(ttBound, ttScore, beta)) {

        // Give a history bonus to quiet tt moves that causes a cutoff
        if (ttScore >= beta && moveIsQuiet(ttMove)) {
            QuietHistoryUpdate(ttMove, Bonus(depth));
            PawnHistoryUpdate(ttMove, Bonus(depth));
        }

        return ttScore;
    }

    int bestScore = -INFINITE;
    int maxScore  =  INFINITE;

    // Probe syzygy TBs
    int tbScore, bound;
    if (!ss->excluded && ProbeWDL(pos, &tbScore, &bound, ss->ply)) {

        thread->tbhits++;

        // Draw scores are exact, while wins are lower bounds and losses upper bounds (mate scores are better/worse)
        if (bound == BOUND_EXACT || (bound == BOUND_LOWER ? tbScore >= beta : tbScore <= alpha)) {
            StoreTTEntry(tte, pos->key, NOMOVE, ScoreToTT(tbScore, ss->ply), NOSCORE, MAX_PLY, bound);
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

    (ss+1)->killer = NOMOVE;

    // Do a static evaluation for pruning considerations
    int eval = ss->staticEval =  inCheck           ? NOSCORE
                               : lastMoveNullMove  ? -(ss-1)->staticEval + 2 * Tempo
                               : ttEval != NOSCORE ? ttEval
                                                   : EvalPosition(pos, thread->pawnCache);

    int unadjustedEval = eval;
    ss->staticEval = eval = CorrectEval(thread, ss, eval, pos->rule50);

    // Use ttScore as eval if it is more informative
    if (!isTerminal(ttScore) && TTScoreIsMoreInformative(ttBound, ttScore, eval))
        eval = ttScore;

    // Improving if not in check, and current eval is higher than 2 plies ago
    bool improving = !inCheck && eval > (ss-2)->staticEval;

    // Internal iterative reduction based on Rebel's idea
    if (pvNode && depth >= 3 && !ttMove)
        depth--;

    if (cutnode && depth >= 8 && !ttMove)
        depth--;

    // Skip pruning in check, pv nodes, early iterations, when proving singularity, looking for terminal scores, or after a null move
    if (inCheck || pvNode || !thread->doPruning || ss->excluded || isTerminal(beta) || (ss-1)->move == NOMOVE)
        goto move_loop;

    // Reverse Futility Pruning
    if (   depth < 7
        && eval >= beta
        && eval - 77 * (depth - improving) - (ss-1)->histScore / 131 >= beta
        && (!ttMove || GetHistory(thread, ss, ttMove) > 6450))
        return eval;

    // Null Move Pruning
    if (   eval >= beta
        && eval >= ss->staticEval
        && ss->staticEval >= beta + 138 - 13 * depth
        && (ss-1)->histScore < 28500
        && pos->nonPawnCount[sideToMove] > (depth > 8)) {

        Depth reduction = 4 + depth / 4 + MIN(3, (eval - beta) / 227);

        ss->move = NOMOVE;
        ss->continuation = &thread->continuation[0][0][EMPTY][0];
        ss->contCorr = &thread->contCorrHistory[EMPTY][0];

        MakeNullMove(pos);
        int score = -AlphaBeta(thread, ss+1, -beta, -alpha, depth - reduction, !cutnode);
        TakeNullMove(pos);

        // Cutoff
        if (score >= beta)
            // Don't return unproven terminal win scores
            return isWin(score) ? beta : score;
    }

    int probCutBeta = beta + 200;

    // ProbCut
    if (   depth >= 5
        && (!ttHit || ttScore >= probCutBeta)) {

        InitProbcutMP(&mp, thread, ss, probCutBeta - ss->staticEval);

        Move move;
        while ((move = NextMove(&mp))) {

            if (mp.stage > NOISY_GOOD) break;

            if (!MoveIsLegal(pos, move)) continue;
            MakeMove(pos, move);

            ss->move = move;
            ss->continuation = &thread->continuation[inCheck][moveIsCapture(move)][piece(move)][toSq(move)];
            ss->contCorr = &thread->contCorrHistory[piece(move)][toSq(move)];

            // See if a quiescence search beats the threshold
            int score = -Quiescence(thread, ss+1, -probCutBeta, -probCutBeta+1);

            // If it did, do a proper search with reduced depth
            if (score >= probCutBeta)
                score = -AlphaBeta(thread, ss+1, -probCutBeta, -probCutBeta+1, depth-4, !cutnode);

            TakeMove(pos);

            // Cut if the reduced depth search beats the threshold, terminal scores are exact
            if (score >= probCutBeta)
                return isWin(score) ? score : score - 160;
        }
    }

move_loop:

    InitNormalMP(&mp, thread, ss, depth, ttMove, ss->killer);

    Move quiets[32];
    Move noisys[32];
    Move bestMove = NOMOVE;
    int moveCount = 0, quietCount = 0, noisyCount = 0;
    int score = -INFINITE;

    Color opponent = !sideToMove;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        if (move == ss->excluded) continue;
        if (root && AlreadySearchedMultiPV(thread, move)) continue;
        if (root && NotInSearchMoves(Limits.searchmoves, move)) continue;
        if (!MoveIsLegal(pos, move)) continue;

        moveCount++;

        bool quiet = moveIsQuiet(move);

        uint64_t startingNodes = pos->nodes;

        ss->histScore = GetHistory(thread, ss, move);

        // Misc pruning
        if (  !root
            && thread->doPruning
            && !isLoss(bestScore)) {

            int R = Reductions[quiet][MIN(31, depth)][MIN(31, moveCount)] - ss->histScore / 9164;
            Depth lmrDepth = depth - 1 - R;

            // Quiet late move pruning
            if (moveCount > (improving ? 2 + depth * depth : depth * depth / 2))
                mp.onlyNoisy = true;

            // History pruning
            if (quiet && lmrDepth < 3 && ss->histScore < -1024 * depth)
                continue;

            // SEE pruning
            if (lmrDepth < 7 && !SEE(pos, move, -73 * depth))
                continue;
        }

        // Extension
        Depth extension = 0;

        // Avoid extending too far
        if (root || ss->ply >= thread->depth * 2)
            goto skip_extensions;

        // Singular extension
        if (   depth > 4
            && move == ttMove
            && !ss->excluded
            && ttDepth > depth - 3
            && ttBound != BOUND_UPPER
            && !isTerminal(ttScore)) {

            // Search to reduced depth with a zero window a bit lower than ttScore
            int singularBeta = ttScore - depth * (2 - pvNode);
            ss->excluded = move;
            score = AlphaBeta(thread, ss, singularBeta-1, singularBeta, depth/2, cutnode);
            ss->excluded = NOMOVE;

            // Singular - extend by 1 or 2 ply
            if (score < singularBeta) {
                extension = 1;
                if (!pvNode && score < singularBeta - 1 && ss->doubleExtensions <= 5)
                    extension = 2;
            // MultiCut - ttMove as well as at least one other move seem good enough to beat beta
            } else if (singularBeta >= beta)
                return singularBeta;
            // Negative extension - not singular but likely still good enough to beat beta
            else if (ttScore >= beta)
                extension = -1;
        }

        // Extend when in check
        if (inCheck)
            extension = MAX(extension, 1);

skip_extensions:

        MakeMove(pos, move);

        ss->move = move;
        ss->doubleExtensions = (ss-1)->doubleExtensions + (extension == 2);
        ss->continuation = &thread->continuation[inCheck][moveIsCapture(move)][piece(move)][toSq(move)];
        ss->contCorr = &thread->contCorrHistory[piece(move)][toSq(move)];

        Depth newDepth = depth - 1 + extension;

        // Base reduction
        int r = Reductions[quiet][MIN(31, depth)][MIN(31, moveCount)];
        // Adjust reduction by move history
        r -= ss->histScore / 8870;
        // Reduce less in pv nodes
        r -= pvNode;
        // Reduce less when improving
        r -= improving;
        // Reduce quiets more if ttMove is a capture
        r += moveIsCapture(ttMove);
        // Reduce more when opponent has few pieces
        r += pos->nonPawnCount[opponent] < 2;
        // Reduce more in cut nodes
        r += 2 * cutnode;

        // Reduced depth zero-window search
        if (   depth > 2
            && moveCount > MAX(1, pvNode + !ttMove + root + !quiet)
            && thread->doPruning) {

            // Depth after reductions, avoiding going straight to quiescence as well as extending
            Depth lmrDepth = CLAMP(newDepth - r, 1, newDepth);

            score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, lmrDepth, true);

            // Re-search with the same window at full depth if the reduced search failed high
            if (score > alpha && lmrDepth < newDepth) {
                bool deeper = score > bestScore + 1 + 6 * (newDepth - lmrDepth);

                newDepth += deeper;

                score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, newDepth, !cutnode);

                // Update continuation history if the re-search failed high or low
                if (quiet && (score <= alpha || score >= beta))
                    UpdateContHistories(ss, move, score >= beta ? Bonus(depth) : Malus(depth));
            }
        }

        // Full depth zero-window search
        else if (!pvNode || moveCount > 1)
            score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, newDepth - (r > 3), !cutnode);

        // Full depth alpha-beta window search
        if (pvNode && (score > alpha || moveCount == 1))
            score = -AlphaBeta(thread, ss+1, -beta, -alpha, newDepth, false);

        // Undo the move
        TakeMove(pos);

        if (root) {
            RootMove *rm;
            for (rm = thread->rootMoves; rm->move; ++rm)
                if (rm->move == move)
                    break;

            rm->nodes += pos->nodes - startingNodes;

            if (moveCount == 1 || score > alpha) {
                rm->score = score;
                rm->pv.length = 1 + (ss+1)->pv.length;
                rm->pv.line[0] = move;
                memcpy(rm->pv.line+1, (ss+1)->pv.line, sizeof(Move) * (ss+1)->pv.length);
            } else {
                rm->score = -INFINITE;
            }
        }

        // New best move
        if (score > bestScore) {
            bestScore = score;

            // Update PV
            if ((score > alpha && pvNode) || (root && moveCount == 1))
                UpdatePv(ss, move);

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
                   : inCheck      ? matedIn(ss->ply)
                                  : 0;

    // Make sure score isn't above the max score given by TBs
    bestScore = MIN(bestScore, maxScore);

    // Store in TT
    if (!ss->excluded && (!root || !thread->multiPV))
        StoreTTEntry(tte, pos->key, bestMove, ScoreToTT(bestScore, ss->ply), unadjustedEval, depth,
                       bestScore >= beta  ? BOUND_LOWER
                     : pvNode && bestMove ? BOUND_EXACT
                                          : BOUND_UPPER);

    // Update correction history
    if (   !inCheck
        && !capturing(bestMove)
        && !(bestScore >= beta && bestScore <= ss->staticEval)
        && !(!bestMove && bestScore >= ss->staticEval))
        UpdateCorrectionHistory(thread, ss, bestScore, ss->staticEval, depth);

    return bestScore;
}

// Aspiration window
static void AspirationWindow(Thread *thread, Stack *ss) {

    Position *pos = &thread->pos;

    const bool mainThread = thread->index == 0;
    const int multiPV = thread->multiPV;
    int depth = thread->depth;

    int prevScore = thread->rootMoves[multiPV].score;

    int delta = 9 + prevScore * prevScore / 16384;

    int alpha = MAX(prevScore - delta, -INFINITE);
    int beta  = MIN(prevScore + delta,  INFINITE);

    int x = CLAMP(prevScore / 2, -32, 32);
    pos->trend = sideToMove == WHITE ? S(x, x/2) : -S(x, x/2);

    // Repeatedly search and adjust the window until the score is inside the window
    while (true) {

        thread->doPruning =
            Limits.infinite ? TimeSince(Limits.start) > 1000
                            :   TimeSince(Limits.start) >= Limits.optimalUsage / 64
                             || depth > 2 + Limits.optimalUsage / 270
                             || Limits.nodeTime;

        int score = AlphaBeta(thread, ss, alpha, beta, depth, false);

        SortRootMoves(thread, multiPV);

        // Give an update when failing high/low in longer searches
        if (   mainThread
            && Limits.multiPV == 1
            && !Minimal
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
            depth = MAX(1, depth - !isTerminal(score));

        // Score inside the window
        } else {
            if (multiPV == 0)
                thread->uncertain = ss->pv.line[0] != thread->rootMoves[0].move;

            return;
        }

        delta += delta / 3;
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

        // Search the position, once for each multi-pv
        for (thread->multiPV = 0; thread->multiPV < multiPV; ++thread->multiPV)
            AspirationWindow(thread, ss);

        // Sort root moves so they are printed in the right order in multi-pv mode
        SortRootMoves(thread, 0);

        // Only the main thread concerns itself with the rest
        if (!mainThread) continue;

        // Print search info
        if (!Minimal)
            PrintThinking(thread, -INFINITE, INFINITE);

        // Stop searching after finding a short enough mate
        if (MATE - abs(thread->rootMoves[0].score) <= 2 * abs(Limits.mate)) break;

        // Limit time spent on forced moves
        if (thread->rootMoveCount == 1 && Limits.timelimit && !Limits.movetime)
            Limits.optimalUsage = MIN(500, Limits.optimalUsage);

        double nodeRatio = 1.0 - (double)thread->rootMoves[0].nodes / (MAX(1, pos->nodes));
        double timeRatio = 0.52 + 3.73 * nodeRatio;

        // If an iteration finishes after optimal time usage, stop the search
        if (   Limits.timelimit
            && !thread->uncertain
            && TimeSince(Limits.start) > Limits.optimalUsage * timeRatio)
            break;

        // Clear key history for seldepth calculation
        for (int i = 1; i < MAX_PLY; ++i)
            history(i).key = 0;
    }

    // Print final search info in minimal mode
    if (mainThread && Minimal) {
        // Fix the depth when the search is stopped due to reaching the depth limit
        if (thread->depth > Limits.depth)
            thread->depth--;
        PrintThinking(thread, -INFINITE, INFINITE);
    }

    return NULL;
}

// Root of search
void *SearchPosition(void *pos) {

    SEARCH_STOPPED = false;

    InitTimeManagement();
    TTNewSearch();
    PrepareSearch(pos, Limits.searchmoves);

    // Probe TBs for a move if already in a TB position
    if (SyzygyMove(pos)) goto conclusion;

    // Probe noobpwnftw's Chess Cloud Database
    if (ProbeNoob(pos)) goto conclusion;

    // Start helper threads and begin searching
    StartHelpers(IterativeDeepening);
    IterativeDeepening(&Threads[0]);

conclusion:

    // Wait for 'stop' in infinite search
    if (Limits.infinite) Wait(&ABORT_SIGNAL);

    // Signal helper threads to stop and wait for them to finish
    ABORT_SIGNAL = true;
    WaitForHelpers();

    // Print the best move found
    PrintBestMove(Threads->rootMoves[0].move);

    SEARCH_STOPPED = true;

    // Wake up waiting UCI thread
    Wake();

    return NULL;
}
