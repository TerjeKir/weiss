// movegen.c

#include <stdio.h>

#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "makemove.h"
#include "move.h"
#include "validate.h"


enum { QUIET, NOISY };
enum { ENPAS, PROMO, PROMOCAP };

static int MvvLvaScores[PIECE_NB][PIECE_NB];

// Initializes the MostValuableVictim-LeastValuableAttacker scores used for ordering captures
CONSTR InitMvvLva() {

    const int VictimScore[PIECE_NB]   = {0, 106, 206, 306, 406, 506, 606, 0, 0, 106, 206, 306, 406, 506, 606, 0};
    const int AttackerScore[PIECE_NB] = {0,   1,   2,   3,   4,   5,   6, 0, 0,   1,   2,   3,   4,   5,   6, 0};

    for (int Attacker = PIECE_MIN; Attacker < PIECE_NB; ++Attacker)
        for (int Victim = PIECE_MIN; Victim < PIECE_NB; ++Victim)
            MvvLvaScores[Victim][Attacker] = VictimScore[Victim] - AttackerScore[Attacker];
}

/* Functions that add moves to the movelist - called by generators */

INLINE void AddQuiet(const Position *pos, const int from, const int to, const int promo, const int flag, MoveList *list) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(CheckBoard(pos));
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

    int move = MOVE(from, to, EMPTY, promo, flag);

    list->moves[list->count].move = move;

    // Add scores to help move ordering based on search history heuristics
    if (pos->searchKillers[0][pos->ply] == move)
        list->moves[list->count].score = 900000;
    else if (pos->searchKillers[1][pos->ply] == move)
        list->moves[list->count].score = 800000;
    else if (promo)
        list->moves[list->count].score = 700000;
    else
        list->moves[list->count].score = pos->searchHistory[pos->board[from]][to];

    list->count++;
}
INLINE void AddCapture(const Position *pos, const int from, const int to, const int promo, MoveList *list) {

    const int captured = pos->board[to];
    const int move     = MOVE(from, to, captured, promo, 0);

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(ValidPiece(captured));
    assert(CheckBoard(pos));

    list->moves[list->count].move = move;
    list->moves[list->count].score = MvvLvaScores[captured][pos->board[from]] + 1000000;
    list->count++;
}
INLINE void AddSpecialPawn(const Position *pos, MoveList *list, const int from, const int to, const int color, const int type) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(CheckBoard(pos));

    if (type == ENPAS) {
        int move = MOVE(from, to, EMPTY, EMPTY, FLAG_ENPAS);
        list->moves[list->count].move = move;
        list->moves[list->count].score = 105 + 1000000;
        list->count++;
    }
    if (type == PROMO) {
        AddQuiet(pos, from, to, makePiece(color, QUEEN ), 0, list);
        AddQuiet(pos, from, to, makePiece(color, KNIGHT), 0, list);
        AddQuiet(pos, from, to, makePiece(color, ROOK  ), 0, list);
        AddQuiet(pos, from, to, makePiece(color, BISHOP), 0, list);
    }
    if (type == PROMOCAP) {
        AddCapture(pos, from, to, makePiece(color, QUEEN ), list);
        AddCapture(pos, from, to, makePiece(color, KNIGHT), list);
        AddCapture(pos, from, to, makePiece(color, ROOK  ), list);
        AddCapture(pos, from, to, makePiece(color, BISHOP), list);
    }
}

/* Generators for specific color/piece combinations - called by generic generators*/

// King
INLINE void GenCastling(const Position *pos, MoveList *list, const int color) {

    const int KCA = color == WHITE ? WKCA : BKCA;
    const int QCA = color == WHITE ? WQCA : BQCA;
    const bitboard kingbits  = color == WHITE ? bitF1G1 : bitF8G8;
    const bitboard queenbits = color == WHITE ? bitB1C1D1 : bitB8C8D8;
    const int from = color == WHITE ? E1 : E8;
    const int ksto = color == WHITE ? G1 : G8;
    const int qsto = color == WHITE ? C1 : C8;
    const int ksmiddle = color == WHITE ? F1 : F8;
    const int qsmiddle = color == WHITE ? D1 : D8;

    const bitboard occupied = pos->colorBBs[BOTH];

    // King side castle
    if (pos->castlePerm & KCA)
        if (!(occupied & kingbits))
            if (   !SqAttacked(from, !color, pos)
                && !SqAttacked(ksmiddle, !color, pos))
                AddQuiet(pos, from, ksto, EMPTY, FLAG_CASTLE, list);

    // Queen side castle
    if (pos->castlePerm & QCA)
        if (!(occupied & queenbits))
            if (   !SqAttacked(from, !color, pos)
                && !SqAttacked(qsmiddle, !color, pos))
                AddQuiet(pos, from, qsto, EMPTY, FLAG_CASTLE, list);
}
INLINE void GenKing(const Position *pos, MoveList *list, const int color, const int type) {

    const bitboard targets = type == QUIET ? ~pos->colorBBs[BOTH]
                                           :  pos->colorBBs[!color];

    const int sq = pos->kingSq[color];
    bitboard moves = king_attacks[sq] & targets;
    while (moves) {
        if (type == NOISY)
            AddCapture(pos, sq, PopLsb(&moves), 0, list);
        if (type == QUIET)
            AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
    }
}

INLINE int relSqDiff(const int color, const int sq, const int diff) {
    return color == WHITE ? sq - diff : sq + diff;
}

// White pawn
INLINE void GenPawnNoisy(const Position *pos, MoveList *list, const int color) {

    int sq;
    bitboard enPassers;

    const bitboard enemies =  pos->colorBBs[!color];
    const bitboard empty   = ~pos->colorBBs[BOTH];

    bitboard relativeRank8BB = color == WHITE ? rank8BB : rank1BB;

    bitboard pawns      = pos->colorBBs[color] & pos->pieceBBs[PAWN];
    bitboard lAttacks   = color == WHITE ? ((pawns & ~fileABB) << 7) & enemies
                                         : ((pawns & ~fileHBB) >> 7) & enemies;

    bitboard rAttacks   = color == WHITE ? ((pawns & ~fileHBB) << 9) & enemies
                                         : ((pawns & ~fileABB) >> 9) & enemies;

    bitboard promotions = color == WHITE ? ((pawns & rank7BB) << 8) & empty
                                         : ((pawns & rank2BB) >> 8) & empty;

    bitboard lNormalCap = lAttacks & ~relativeRank8BB;
    bitboard lPromoCap  = lAttacks &  relativeRank8BB;
    bitboard rNormalCap = rAttacks & ~relativeRank8BB;
    bitboard rPromoCap  = rAttacks &  relativeRank8BB;

    // Promoting captures
    while (lPromoCap) {
        sq = PopLsb(&lPromoCap);
        AddSpecialPawn(pos, list, relSqDiff(color, sq, 7), sq, color, PROMOCAP);
    }
    while (rPromoCap) {
        sq = PopLsb(&rPromoCap);
        AddSpecialPawn(pos, list, relSqDiff(color, sq, 9), sq, color, PROMOCAP);
    }
    // Promotions
    while (promotions) {
        sq = PopLsb(&promotions);
        AddSpecialPawn(pos, list, relSqDiff(color, sq, 8), sq, color, PROMO);
    }
    // Captures
    while (lNormalCap) {
        sq = PopLsb(&lNormalCap);
        AddCapture(pos, relSqDiff(color, sq, 7), sq, EMPTY, list);
    }
    while (rNormalCap) {
        sq = PopLsb(&rNormalCap);
        AddCapture(pos, relSqDiff(color, sq, 9), sq, EMPTY, list);
    }
    // En passant
    if (pos->enPas != NO_SQ) {
        enPassers = pawns & pawn_attacks[!color][pos->enPas];
        while (enPassers)
            AddSpecialPawn(pos, list, PopLsb(&enPassers), pos->enPas, color, ENPAS);
    }
}
INLINE void GenPawnQuiet(const Position *pos, MoveList *list, const int color) {

    int sq;
    bitboard pawnMoves, pawnStarts, pawnsNot7th;

    const bitboard empty = ~pos->colorBBs[BOTH];

    bitboard relRank7BB = color == WHITE ? rank7BB : rank2BB;

    pawnsNot7th = pos->colorBBs[color] & pos->pieceBBs[PAWN] & ~relRank7BB;

    pawnMoves  = color == WHITE ? empty & pawnsNot7th << 8
                                : empty & pawnsNot7th >> 8;

    pawnStarts = color == WHITE ? empty & (pawnMoves & rank3BB) << 8
                                : empty & (pawnMoves & rank6BB) >> 8;

    // Normal pawn moves
    while (pawnMoves) {
        sq = PopLsb(&pawnMoves);
        AddQuiet(pos, relSqDiff(color, sq, 8), sq, EMPTY, 0, list);
    }
    // Pawn starts
    while (pawnStarts) {
        sq = PopLsb(&pawnStarts);
        AddQuiet(pos, relSqDiff(color, sq, 16), sq, EMPTY, FLAG_PAWNSTART, list);
    }
}

// White knight
INLINE void GenPieceType(const Position *pos, MoveList *list, const int color, const int type, const int pt) {

    int sq;
    bitboard moves;

    const bitboard occupied = pos->colorBBs[BOTH];
    const bitboard enemies  = pos->colorBBs[!color];
    const bitboard empty    = ~occupied;

    bitboard pieces = pos->colorBBs[color] & pos->pieceBBs[pt];

    while (pieces) {

        sq = PopLsb(&pieces);

        if (pt == KNIGHT) {
            if (type == NOISY) {
                moves = knight_attacks[sq] & enemies;
                while (moves)
                    AddCapture(pos, sq, PopLsb(&moves), EMPTY, list);
            }

            if (type == QUIET) {
                moves = knight_attacks[sq] & empty;
                while (moves)
                    AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
            }
        }
        if (pt == BISHOP) {
            if (type == NOISY) {
                moves = BishopAttacks(sq, occupied) & enemies;
                while (moves)
                    AddCapture(pos, sq, PopLsb(&moves), 0, list);
            }

            if (type == QUIET) {
                moves = BishopAttacks(sq, occupied) & empty;
                while (moves)
                    AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
            }
        }
        if (pt == ROOK) {
            if (type == NOISY) {
                moves = RookAttacks(sq, occupied) & enemies;
                while (moves)
                    AddCapture(pos, sq, PopLsb(&moves), 0, list);
            }

            if (type == QUIET) {
                moves = RookAttacks(sq, occupied) & empty;
                while (moves)
                    AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
            }
        }
        if (pt == QUEEN) {
            if (type == NOISY) {
                moves = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & enemies;
                while (moves)
                    AddCapture(pos, sq, PopLsb(&moves), 0, list);
            }

            if (type == QUIET) {
                moves = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & empty;
                while (moves)
                    AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
            }
        }
    }
}

/* Generic generators */

static void GenMoves(const Position *pos, MoveList *list, const int color, const int type) {

    assert(CheckBoard(pos));

    if (type == NOISY)
        GenPawnNoisy(pos, list, color);

    if (type == QUIET) {
        GenCastling (pos, list, color);
        GenPawnQuiet(pos, list, color);
    }

    GenPieceType(pos, list, color, type, KNIGHT);
    GenPieceType(pos, list, color, type, ROOK);
    GenPieceType(pos, list, color, type, BISHOP);
    GenPieceType(pos, list, color, type, QUEEN);
    GenKing     (pos, list, color, type);

    assert(MoveListOk(list, pos));
}

// Generate all quiet moves
void GenQuietMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, pos->side, QUIET);
}

// Generate all noisy moves
void GenNoisyMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, pos->side, NOISY);
}

// Generate all pseudo legal moves
void GenAllMoves(const Position *pos, MoveList *list) {

    list->count = list->next = 0;

    GenNoisyMoves(pos, list);
    GenQuietMoves(pos, list);
}