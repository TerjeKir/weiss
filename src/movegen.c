// movegen.c

#include <stdio.h>

#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "makemove.h"
#include "move.h"
#include "validate.h"


static int MvvLvaScores[PIECE_NB][PIECE_NB];

// Initializes the MostValuableVictim-LeastValuableAttacker scores used for ordering captures
static void InitMvvLva() __attribute__((constructor));
static void InitMvvLva() {

	const int VictimScore[PIECE_NB]   = {0, 106, 206, 306, 406, 506, 606, 0, 0, 106, 206, 306, 406, 506, 606, 0};
	const int AttackerScore[PIECE_NB] = {0,   1,   2,   3,   4,   5,   6, 0, 0,   1,   2,   3,   4,   5,   6, 0};

	for (int Attacker = PIECE_MIN; Attacker < PIECE_NB; ++Attacker)
		for (int Victim = PIECE_MIN; Victim < PIECE_NB; ++Victim)
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] - AttackerScore[Attacker];
}

/* Functions that add moves to the movelist - called by generators */

static inline void AddQuiet(const Position *pos, const int from, const int to, const int promo, const int flag, MoveList *list) {

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
static inline void AddCapture(const Position *pos, const int from, const int to, const int promo, MoveList *list) {

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
static inline void AddEnPas(const int move, MoveList *list) {
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
static inline void GenWCastling(const Position *pos, MoveList *list, const bitboard occupied) {

	// King side castle
	if (pos->castlePerm & WKCA)
		if (!(occupied & bitF1G1))
			if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos))
				AddQuiet(pos, E1, G1, EMPTY, FLAG_CASTLE, list);

	// Queen side castle
	if (pos->castlePerm & WQCA)
		if (!(occupied & bitB1C1D1))
			if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos))
				AddQuiet(pos, E1, C1, EMPTY, FLAG_CASTLE, list);
}
static inline void GenBCastling(const Position *pos, MoveList *list, const bitboard occupied) {

	// King side castle
	if (pos->castlePerm & BKCA)
		if (!(occupied & bitF8G8))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos))
				AddQuiet(pos, E8, G8, EMPTY, FLAG_CASTLE, list);

	// Queen side castle
	if (pos->castlePerm & BQCA)
		if (!(occupied & bitB8C8D8))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos))
				AddQuiet(pos, E8, C8, EMPTY, FLAG_CASTLE, list);
}
static inline void GenKingQuiet(const Position *pos, MoveList *list, const int sq, const bitboard empty) {

	bitboard moves = king_attacks[sq] & empty;
	while (moves) {
		AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenKingNoisy(const Position *pos, MoveList *list, const int sq, const bitboard enemies) {

	bitboard attacks = king_attacks[sq] & enemies;
	while (attacks)
		AddCapture(pos, sq, PopLsb(&attacks), 0, list);
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
static inline void GenWKnightQuiet(const Position *pos, MoveList *list, const bitboard empty) {

	int sq;
	bitboard moves;

	bitboard knights = pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		moves = knight_attacks[sq] & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenWKnightNoisy(const Position *pos, MoveList *list, const bitboard enemies) {

	int sq;
	bitboard attacks;

	bitboard knights = pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), EMPTY, list);
	}
}
// Black knight
static inline void GenBKnightQuiet(const Position *pos, MoveList *list, const bitboard empty) {

	int sq;
	bitboard moves;

	bitboard knights = pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		moves = knight_attacks[sq] & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenBKnightNoisy(const Position *pos, MoveList *list, const bitboard enemies) {

	int sq;
	bitboard attacks;

	bitboard knights = pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);
	}
}

// White bishop
static inline void GenWBishopQuiet(const Position *pos, MoveList *list, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard moves;

	bitboard bishops = pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		moves = BishopAttacks(sq, occupied) & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenWBishopNoisy(const Position *pos, MoveList *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard bishops = pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = BishopAttacks(sq, occupied) & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);
	}
}
// Black bishop
static inline void GenBBishopQuiet(const Position *pos, MoveList *list, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard moves;

	bitboard bishops = pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		moves = BishopAttacks(sq, occupied) & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenBBishopNoisy(const Position *pos, MoveList *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard bishops = pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = BishopAttacks(sq, occupied) & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);
	}
}

// White rook
static inline void GenWRookQuiet(const Position *pos, MoveList *list, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard moves;

	bitboard rooks = pos->colorBBs[WHITE] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		moves = RookAttacks(sq, occupied) & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenWRookNoisy(const Position *pos, MoveList *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard rooks = pos->colorBBs[WHITE] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = RookAttacks(sq, occupied) & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);
	}
}
// Black rook
static inline void GenBRookQuiet(const Position *pos, MoveList *list, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard moves;

	bitboard rooks = pos->colorBBs[BLACK] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		moves = RookAttacks(sq, occupied) & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenBRookNoisy(const Position *pos, MoveList *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard rooks = pos->colorBBs[BLACK] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = RookAttacks(sq, occupied) & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);
	}
}

// White queen
static inline void GenWQueenQuiet(const Position *pos, MoveList *list, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard moves;

	bitboard queens  = pos->colorBBs[WHITE] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		moves = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenWQueenNoisy(const Position *pos, MoveList *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard queens  = pos->colorBBs[WHITE] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		attacks = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);
	}
}
// Black queen
static inline void GenBQueenQuiet(const Position *pos, MoveList *list, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard moves;

	bitboard queens  = pos->colorBBs[BLACK] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		moves = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenBQueenNoisy(const Position *pos, MoveList *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard queens  = pos->colorBBs[BLACK] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		attacks = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);
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
		GenWCastling   (pos, list, occupied);
		GenWPawnQuiet  (pos, list, empty);
		GenWKnightQuiet(pos, list, empty);
		GenWRookQuiet  (pos, list, empty, occupied);
		GenWBishopQuiet(pos, list, empty, occupied);
		GenWQueenQuiet (pos, list, empty, occupied);
	} else {
		GenBCastling   (pos, list, occupied);
		GenBPawnQuiet  (pos, list, empty);
		GenBKnightQuiet(pos, list, empty);
		GenBRookQuiet  (pos, list, empty, occupied);
		GenBBishopQuiet(pos, list, empty, occupied);
		GenBQueenQuiet (pos, list, empty, occupied);
	}

	GenKingQuiet(pos, list, pos->kingSq[side], empty);

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
		GenWKnightNoisy(pos, list, enemies);
		GenWRookNoisy  (pos, list, enemies, occupied);
		GenWBishopNoisy(pos, list, enemies, occupied);
		GenWQueenNoisy (pos, list, enemies, occupied);

	} else {
		GenBPawnNoisy  (pos, list, enemies, empty);
		GenBKnightNoisy(pos, list, enemies);
		GenBRookNoisy  (pos, list, enemies, occupied);
		GenBBishopNoisy(pos, list, enemies, occupied);
		GenBQueenNoisy (pos, list, enemies, occupied);
	}

	GenKingNoisy(pos, list, pos->kingSq[side], enemies);

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