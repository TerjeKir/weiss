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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fathom/tbprobe.h"
#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "movepicker.h"
#include "time.h"
#include "transposition.h"
#include "syzygy.h"


int Reductions[32][32];

SearchLimits Limits;
extern volatile bool ABORT_SIGNAL;


// Initializes the late move reduction array
CONSTR InitReductions() {

    for (int depth = 0; depth < 32; ++depth)
        for (int moves = 0; moves < 32; ++moves)
            Reductions[depth][moves] = 0.75 + log(depth) * log(moves) / 2.25;
}

// Check time situation
static bool OutOfTime(SearchInfo *info) {

    return (info->nodes & 4095) == 4095
        && Limits.timelimit
        && TimeSince(Limits.start) >= Limits.maxUsage;
}

// Check if current position is a repetition
static bool IsRepetition(const Position *pos) {

    // Compare current posKey to posKeys in history, skipping
    // opponents turns as that wouldn't be a repetition
    for (int i = 4; i <= pos->rule50; i += 2)
        if (pos->key == history(-i).posKey)
            return true;

    return false;
}

// Get ready to start a search
static void PrepareSearch(Position *pos, SearchInfo *info) {

    memset(info, 0, sizeof(SearchInfo));

    memset(pos->history, 0, sizeof(pos->history));
    memset(pos->killers, 0, sizeof(pos->killers));

    // Mark TT as used
    TT.dirty = true;
}

// Print thinking
static void PrintThinking(const SearchInfo *info) {

    int score = info->score;

    // Determine whether we have a centipawn or mate score
    char *type = abs(score) >= SLOWEST_MATE ? "mate" : "cp";

    // Convert score to mate score when applicable
    score = score >=  SLOWEST_MATE ?  ((MATE - score) / 2) + 1
          : score <= -SLOWEST_MATE ? -((MATE + score) / 2)
                                   : score * 100 / P_MG;

    TimePoint elapsed = TimeSince(Limits.start);
    Depth seldepth    = info->seldepth;
    int hashFull      = HashFull();
    int nps           = (int)(1000 * info->nodes / (elapsed + 1));

    // Basic info
    printf("info depth %d seldepth %d score %s %d time %" PRId64
           " nodes %" PRIu64 " nps %d tbhits %" PRIu64 " hashfull %d pv",
            info->depth, seldepth, type, score, elapsed,
            info->nodes, nps, info->tbhits, hashFull);

    // Principal variation
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

INLINE bool PawnOn7th(const Position *pos) {
    return colorPieceBB(sideToMove, PAWN) & RankBB[RelativeRank(sideToMove, RANK_7)];
}

// Dynamic delta pruning margin
static int QuiescenceDeltaMargin(const Position *pos) {

    // Optimistic we can improve our position by a pawn without capturing anything,
    // or if we have a pawn on the 7th we can hope to improve by a queen instead
    const int DeltaBase = PawnOn7th(pos) ? Q_MG : P_MG;

    // Look for possible captures on the board
    const Bitboard enemy = colorBB(!sideToMove);

    // Find the most valuable piece we could take and add to our base
    return DeltaBase + ((enemy & pieceBB(QUEEN )) ? Q_MG
                      : (enemy & pieceBB(ROOK  )) ? R_MG
                      : (enemy & pieceBB(BISHOP)) ? B_MG
                      : (enemy & pieceBB(KNIGHT)) ? N_MG
                                                  : P_MG);
}

// Quiescence
static int Quiescence(Position *pos, SearchInfo *info, int alpha, const int beta) {

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
    if (IsRepetition(pos) || pos->rule50 >= 100)
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

    int futility = score + P_EG;

    const bool inCheck = KingAttacked(pos, sideToMove);

    InitNoisyMP(&mp, &list, pos);

    int bestScore = score;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        if (   !inCheck
            && futility + PieceValue[EG][pieceOn(toSq(move))] <= alpha
            && !(  PieceTypeOf(pieceOn(fromSq(move))) == PAWN
                && RelativeRank(sideToMove, RankOf(toSq(move))) > 5))
            continue;

        // Recursively search the positions after making the moves, skipping illegal ones
        if (!MakeMove(pos, move)) continue;
        score = -Quiescence(pos, info, -beta, -alpha);
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
static int AlphaBeta(Position *pos, SearchInfo *info, int alpha, int beta, Depth depth, PV *pv) {

    const bool pvNode = alpha != beta - 1;
    const bool root   = pos->ply == 0;

    PV pvFromHere;
    pv->length = 0;

    MovePicker mp;
    MoveList list;

    // Extend search if in check
    const bool inCheck = KingAttacked(pos, sideToMove);
    if (inCheck && depth + 1 < MAXDEPTH) depth++;

    // Quiescence at the end of search
    if (depth <= 0)
        return Quiescence(pos, info, alpha, beta);

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

        info->tbhits++;

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

    // Improving if not in check, and current eval is higher than 2 plies ago
    bool improving = !inCheck && pos->ply >= 2 && eval > history(-2).eval;

    // Skip pruning while in check and at the root
    if (inCheck || root)
        goto move_loop;

    // Razoring
    if (!pvNode && depth < 2 && eval + 640 < alpha)
        return Quiescence(pos, info, alpha, beta);

    // Reverse Futility Pruning
    if (!pvNode && depth < 7 && eval - 225 * depth + 100 * improving >= beta)
        return eval;

    // Null Move Pruning
    if (  !pvNode
        && history(-1).move != NOMOVE
        && eval >= beta
        && pos->nonPawnCount[sideToMove] > 0
        && depth >= 3) {

        int R = 3 + depth / 5 + MIN(3, (eval - beta) / 256);

        MakeNullMove(pos);
        score = -AlphaBeta(pos, info, -beta, -beta + 1, depth - R, &pvFromHere);
        TakeNullMove(pos);

        // Cutoff
        if (score >= beta) {
            // Don't return unproven terminal win scores
            return score >= SLOWEST_TB_WIN ? beta : score;
        }
    }

    // Internal iterative deepening
    if (depth >= 4 && !ttMove) {

        AlphaBeta(pos, info, alpha, beta, MAX(1, MIN(depth / 2, depth - 4)), pv);

        tte = ProbeTT(posKey, &ttHit);

        ttMove = ttHit ? tte->move : NOMOVE;
    }

move_loop:

    InitNormalMP(&mp, &list, pos, ttMove);

    const int oldAlpha = alpha;
    int moveCount = 0, quietCount = 0;
    Move bestMove = NOMOVE;
    score = -INFINITE;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        bool quiet = !moveIsNoisy(move);

        // Late move pruning
        if (!pvNode && !inCheck && quietCount > (3 + 2 * depth * depth) / (2 - improving))
            break;

        __builtin_prefetch(GetEntry(KeyAfter(pos, move)));

        // Make the move, skipping to the next if illegal
        if (!MakeMove(pos, move)) continue;

        // Increment counts
        moveCount++;
        if (quiet)
            quietCount++;

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
            Depth RDepth = MAX(1, newDepth - MAX(R, 1));

            score = -AlphaBeta(pos, info, -alpha - 1, -alpha, RDepth, &pvFromHere);
        }
        // Full depth zero-window search
        if (doLMR ? score > alpha : !pvNode || moveCount > 1)
            score = -AlphaBeta(pos, info, -alpha - 1, -alpha, newDepth, &pvFromHere);

        // Full depth alpha-beta window search
        if (pvNode && ((score > alpha && score < beta) || moveCount == 1))
            score = -AlphaBeta(pos, info, -beta, -alpha, newDepth, &pvFromHere);

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
                if (pvNode) {
                    pv->length = 1 + pvFromHere.length;
                    pv->line[0] = move;
                    memcpy(pv->line + 1, pvFromHere.line, sizeof(int) * pvFromHere.length);
                }

                // Update search history
                if (quiet)
                    pos->history[pieceOn(fromSq(bestMove))][toSq(bestMove)] += depth * depth;

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
static int AspirationWindow(Position *pos, SearchInfo *info) {

    int score = info->score;
    int depth = info->depth;

    const int initialWindow = 12;
    int delta = 16;

    // Initial window
    int alpha = MAX(score - initialWindow, -INFINITE);
    int beta  = MIN(score + initialWindow,  INFINITE);

    // Search with aspiration window until the result is inside the window
    while (true) {

        score = AlphaBeta(pos, info, alpha, beta, depth, &info->pv);

        // Failed low, relax lower bound and search again
        if (score <= alpha) {
            alpha = MAX(alpha - delta, -INFINITE);
            beta  = (alpha + beta) / 2;
            depth = info->depth;

        // Failed high, relax upper bound and search again
        } else if (score >= beta) {
            beta = MIN(beta + delta, INFINITE);
            depth -= 1;

        // Score within the bounds is accepted as correct
        } else
            return score;

        delta += delta * 2 / 3;
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

    PrepareSearch(pos, info);

    // Iterative deepening
    for (info->depth = 1; info->depth <= Limits.depth; ++info->depth) {

        // Jump here and go straight to printing conclusion when time's up
        if (setjmp(info->jumpBuffer)) break;

        // Search position, using aspiration windows for higher depths
        info->score = info->depth > 6 ? AspirationWindow(pos, info)
                                      : AlphaBeta(pos, info, -INFINITE, INFINITE, info->depth, &info->pv);

        // Print thinking
        PrintThinking(info);

        // Save bestMove and ponderMove before overwriting the pv next iteration
        info->bestMove   = info->pv.line[0];
        info->ponderMove = info->pv.length > 1 ? info->pv.line[1] : NOMOVE;

        info->seldepth = 0;
    }

    // Wait for 'stop' in infinite search
    while (Limits.infinite && !ABORT_SIGNAL) {}

    // Print conclusion
    PrintConclusion(info);
}