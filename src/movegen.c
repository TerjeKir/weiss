// movegen.c

#include <stdio.h>

#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "makemove.h"
#include "move.h"
#include "validate.h"


enum { QUIET, NOISY };

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
INLINE void AddEnPas(const int move, MoveList *list) {
#ifndef NDEBUG
    const int from = FROMSQ(move);
    const int   to =   TOSQ(move);

    assert(ValidSquare(from));
    assert(ValidSquare(to));
#endif
    list->moves[list->count].move = move;
    list->moves[list->count].score = 105 + 1000000;
    list->count++;
}
static inline void AddWPromo(const Position *pos, const int from, const int to, MoveList *list) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(CheckBoard(pos));

    AddQuiet(pos, from, to, wQ, 0, list);
    AddQuiet(pos, from, to, wN, 0, list);
    AddQuiet(pos, from, to, wR, 0, list);
    AddQuiet(pos, from, to, wB, 0, list);
}
static inline void AddWPromoCapture(const Position *pos, const int from, const int to, MoveList *list) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(CheckBoard(pos));

    AddCapture(pos, from, to, wQ, list);
    AddCapture(pos, from, to, wN, list);
    AddCapture(pos, from, to, wR, list);
    AddCapture(pos, from, to, wB, list);
}
static inline void AddBPromo(const Position *pos, const int from, const int to, MoveList *list) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(CheckBoard(pos));

    AddQuiet(pos, from, to, bQ, 0, list);
    AddQuiet(pos, from, to, bN, 0, list);
    AddQuiet(pos, from, to, bR, 0, list);
    AddQuiet(pos, from, to, bB, 0, list);
}
static inline void AddBPromoCapture(const Position *pos, const int from, const int to, MoveList *list) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(CheckBoard(pos));

    AddCapture(pos, from, to, bQ, list);
    AddCapture(pos, from, to, bN, list);
    AddCapture(pos, from, to, bR, list);
    AddCapture(pos, from, to, bB, list);
}

/* Generators for specific color/piece combinations - called by generic generators*/

// King
INLINE void GenCastling(const Position *pos, MoveList *list, const bitboard occupied, const int color) {

    const int KCA = color == WHITE ? WKCA : BKCA;
    const int QCA = color == WHITE ? WQCA : BQCA;
    const bitboard kingbits  = color == WHITE ? bitF1G1 : bitF8G8;
    const bitboard queenbits = color == WHITE ? bitB1C1D1 : bitB8C8D8;
    const int from = color == WHITE ? E1 : E8;
    const int ksto = color == WHITE ? G1 : G8;
    const int qsto = color == WHITE ? C1 : C8;
    const int ksmiddle = color == WHITE ? F1 : F8;
    const int qsmiddle = color == WHITE ? D1 : D8;

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
INLINE void GenKing(const Position *pos, MoveList *list, const bitboard targets, const int color, const int type) {

    const int sq = pos->kingSq[color];
    bitboard moves = king_attacks[sq] & targets;
    while (moves) {
        if (type == NOISY)
            AddCapture(pos, sq, PopLsb(&moves), 0, list);
        if (type == QUIET)
            AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
    }
}

// White pawn
static inline void GenWPawnNoisy(const Position *pos, MoveList *list, const bitboard enemies, const bitboard empty) {

    int sq;
    bitboard enPassers;

    bitboard pawns      = pos->colorBBs[WHITE] & pos->pieceBBs[PAWN];
    bitboard lAttacks   = ((pawns & ~fileABB) << 7) & enemies;
    bitboard rAttacks   = ((pawns & ~fileHBB) << 9) & enemies;
    bitboard lNormalCap = lAttacks & ~rank8BB;
    bitboard lPromoCap  = lAttacks &  rank8BB;
    bitboard rNormalCap = rAttacks & ~rank8BB;
    bitboard rPromoCap  = rAttacks &  rank8BB;
    bitboard promotions = empty & (pawns & rank7BB) << 8;

    // Promoting captures
    while (lPromoCap) {
        sq = PopLsb(&lPromoCap);
        AddWPromoCapture(pos, sq - 7, sq, list);
    }
    while (rPromoCap) {
        sq = PopLsb(&rPromoCap);
        AddWPromoCapture(pos, sq - 9, sq, list);
    }
    // Promotions
    while (promotions) {
        sq = PopLsb(&promotions);
        AddWPromo(pos, (sq - 8), sq, list);
    }
    // Captures
    while (lNormalCap) {
        sq = PopLsb(&lNormalCap);
        AddCapture(pos, sq - 7, sq, EMPTY, list);
    }
    while (rNormalCap) {
        sq = PopLsb(&rNormalCap);
        AddCapture(pos, sq - 9, sq, EMPTY, list);
    }
    // En passant
    if (pos->enPas != NO_SQ) {
        enPassers = pawns & pawn_attacks[BLACK][pos->enPas];
        while (enPassers)
            AddEnPas(MOVE(PopLsb(&enPassers), pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
    }
}
static inline void GenWPawnQuiet(const Position *pos, MoveList *list, const bitboard empty) {

    int sq;
    bitboard pawnMoves, pawnStarts, pawnsNot7th;

    pawnsNot7th = pos->colorBBs[WHITE] & pos->pieceBBs[PAWN] & ~rank7BB;

    pawnMoves  = empty & pawnsNot7th << 8;
    pawnStarts = empty & (pawnMoves & rank3BB) << 8;

    // Normal pawn moves
    while (pawnMoves) {
        sq = PopLsb(&pawnMoves);
        AddQuiet(pos, (sq - 8), sq, EMPTY, 0, list);
    }
    // Pawn starts
    while (pawnStarts) {
        sq = PopLsb(&pawnStarts);
        AddQuiet(pos, (sq - 16), sq, EMPTY, FLAG_PAWNSTART, list);
    }
}
// Black pawn
static inline void GenBPawnNoisy(const Position *pos, MoveList *list, const bitboard enemies, const bitboard empty) {

    int sq;
    bitboard enPassers;

    bitboard pawns      = pos->colorBBs[BLACK] & pos->pieceBBs[PAWN];
    bitboard lAttacks   = ((pawns & ~fileHBB) >> 7) & enemies;
    bitboard rAttacks   = ((pawns & ~fileABB) >> 9) & enemies;
    bitboard lNormalCap = lAttacks & ~rank1BB;
    bitboard lPromoCap  = lAttacks &  rank1BB;
    bitboard rNormalCap = rAttacks & ~rank1BB;
    bitboard rPromoCap  = rAttacks &  rank1BB;
    bitboard promotions = empty & (pawns & rank2BB) >> 8;

    // Promoting captures
    while (lPromoCap) {
        sq = PopLsb(&lPromoCap);
        AddBPromoCapture(pos, sq + 7, sq, list);
    }
    while (rPromoCap) {
        sq = PopLsb(&rPromoCap);
        AddBPromoCapture(pos, sq + 9, sq, list);
    }
    // Promotions
    while (promotions) {
        sq = PopLsb(&promotions);
        AddBPromo(pos, (sq + 8), sq, list);
    }
    // Captures
    while (lNormalCap) {
        sq = PopLsb(&lNormalCap);
        AddCapture(pos, sq + 7, sq, EMPTY, list);
    }
    while (rNormalCap) {
        sq = PopLsb(&rNormalCap);
        AddCapture(pos, sq + 9, sq, EMPTY, list);
    }
    // En passant
    if (pos->enPas != NO_SQ) {
        enPassers = pawns & pawn_attacks[WHITE][pos->enPas];
        while (enPassers)
            AddEnPas(MOVE(PopLsb(&enPassers), pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
    }
}
static inline void GenBPawnQuiet(const Position *pos, MoveList *list, const bitboard empty) {

    int sq;
    bitboard pawnMoves, pawnStarts, pawnsNot7th;

    pawnsNot7th = pos->colorBBs[BLACK] & pos->pieceBBs[PAWN] & ~rank2BB;

    pawnMoves  = empty & pawnsNot7th >> 8;
    pawnStarts = empty & (pawnMoves & rank6BB) >> 8;

    // Normal pawn moves
    while (pawnMoves) {
        sq = PopLsb(&pawnMoves);
        AddQuiet(pos, (sq + 8), sq, EMPTY, 0, list);
    }
    // Pawn starts
    while (pawnStarts) {
        sq = PopLsb(&pawnStarts);
        AddQuiet(pos, (sq + 16), sq, EMPTY, FLAG_PAWNSTART, list);
    }
}

// White knight
INLINE void GenKnight(const Position *pos, MoveList *list, const bitboard enemies, const bitboard empty, const int color, const int type) {

    int sq;
    bitboard moves;

    bitboard knights = pos->colorBBs[color] & pos->pieceBBs[KNIGHT];

    while (knights) {

        sq = PopLsb(&knights);


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
}

// White bishop
INLINE void GenBishop(const Position *pos, MoveList *list, const bitboard enemies, const bitboard empty, const bitboard occupied, const int color, const int type) {

    int sq;
    bitboard moves;

    bitboard bishops = pos->colorBBs[color] & pos->pieceBBs[BISHOP];

    while (bishops) {

        sq = PopLsb(&bishops);

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
}

// White rook
INLINE void GenRook(const Position *pos, MoveList *list, const bitboard enemies, const bitboard empty, const bitboard occupied, const int color, const int type) {

    int sq;
    bitboard moves;

    bitboard rooks = pos->colorBBs[color] & pos->pieceBBs[ROOK];

    while (rooks) {

        sq = PopLsb(&rooks);

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
}

// White queen
INLINE void GenQueen(const Position *pos, MoveList *list, const bitboard enemies, const bitboard empty, const bitboard occupied, const int color, const int type) {

    int sq;
    bitboard moves;

    bitboard queens = pos->colorBBs[color] & pos->pieceBBs[QUEEN];

    while (queens) {

        sq = PopLsb(&queens);

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

/* Generic generators */

// Generate all quiet moves
void GenQuietMoves(const Position *pos, MoveList *list) {

    assert(CheckBoard(pos));

    const int side = pos->side;

    const bitboard occupied = pos->colorBBs[BOTH];
    const bitboard empty    = ~occupied;

    if (side == WHITE) {
        GenCastling  (pos, list, occupied, WHITE);
        GenWPawnQuiet(pos, list, empty);
        GenKnight(pos, list, 0ULL, empty, WHITE, QUIET);
        GenRook  (pos, list, 0ULL, empty, occupied, WHITE, QUIET);
        GenBishop(pos, list, 0ULL, empty, occupied, WHITE, QUIET);
        GenQueen (pos, list, 0ULL, empty, occupied, WHITE, QUIET);
        GenKing  (pos, list, empty, WHITE, QUIET);
    } else {
        GenCastling  (pos, list, occupied, BLACK);
        GenBPawnQuiet(pos, list, empty);
        GenKnight(pos, list, 0ULL, empty, BLACK, QUIET);
        GenRook  (pos, list, 0ULL, empty, occupied, BLACK, QUIET);
        GenBishop(pos, list, 0ULL, empty, occupied, BLACK, QUIET);
        GenQueen (pos, list, 0ULL, empty, occupied, BLACK, QUIET);
        GenKing  (pos, list, empty, BLACK, QUIET);
    }

    assert(MoveListOk(list, pos));
}

// Generate all noisy moves
void GenNoisyMoves(const Position *pos, MoveList *list) {

    assert(CheckBoard(pos));

    const int side = pos->side;

    const bitboard occupied = pos->colorBBs[BOTH];
    const bitboard enemies  = pos->colorBBs[!side];
    const bitboard empty    = ~occupied;

    // Pawns
    if (side == WHITE) {
        GenWPawnNoisy  (pos, list, enemies, empty);
        GenKnight(pos, list, enemies, empty, WHITE, NOISY);
        GenRook  (pos, list, enemies, empty, occupied, WHITE, NOISY);
        GenBishop(pos, list, enemies, empty, occupied, WHITE, NOISY);
        GenQueen (pos, list, enemies, empty, occupied, WHITE, NOISY);
        GenKing  (pos, list, enemies, WHITE, NOISY);

    } else {
        GenBPawnNoisy  (pos, list, enemies, empty);
        GenKnight(pos, list, enemies, empty, BLACK, NOISY);
        GenRook  (pos, list, enemies, empty, occupied, BLACK, NOISY);
        GenBishop(pos, list, enemies, empty, occupied, BLACK, NOISY);
        GenQueen (pos, list, enemies, empty, occupied, BLACK, NOISY);
        GenKing  (pos, list, enemies, BLACK, NOISY);
    }


    assert(MoveListOk(list, pos));
}

// Generate all pseudo legal moves
void GenAllMoves(const Position *pos, MoveList *list) {

    assert(CheckBoard(pos));

    list->count = list->next = 0;

    GenNoisyMoves(pos, list);
    GenQuietMoves(pos, list);

    assert(MoveListOk(list, pos));
}