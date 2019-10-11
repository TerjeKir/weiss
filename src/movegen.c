// movegen.c

#include <stdio.h>

#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "data.h"
#include "makemove.h"
#include "move.h"
#include "validate.h"


static const bitboard bitB1C1D1 = (1ULL <<  1) | (1ULL <<  2) | (1ULL <<  3);
static const bitboard bitB8C8D8 = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);
static const bitboard bitF1G1   = (1ULL <<  5) | (1ULL <<  6);
static const bitboard bitF8G8   = (1ULL << 61) | (1ULL << 62);

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

static inline void AddQuiet(const S_BOARD *pos, const int from, const int to, const int promo, const int flag, S_MOVELIST *list) {

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
	else
		list->moves[list->count].score = pos->searchHistory[pos->board[from]][to];

	list->count++;
}
static inline void AddCapture(const S_BOARD *pos, const int from, const int to, const int promo, S_MOVELIST *list) {

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
static inline void AddEnPas(const int move, S_MOVELIST *list) {
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
static inline void AddWPromo(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));

	AddQuiet(pos, from, to, wQ, 0, list);
	AddQuiet(pos, from, to, wR, 0, list);
	AddQuiet(pos, from, to, wB, 0, list);
	AddQuiet(pos, from, to, wN, 0, list);
}
static inline void AddWPromoCapture(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));

	AddCapture(pos, from, to, wQ, list);
	AddCapture(pos, from, to, wR, list);
	AddCapture(pos, from, to, wB, list);
	AddCapture(pos, from, to, wN, list);
}
static inline void AddBPromo(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));

	AddQuiet(pos, from, to, bQ, 0, list);
	AddQuiet(pos, from, to, bR, 0, list);
	AddQuiet(pos, from, to, bB, 0, list);
	AddQuiet(pos, from, to, bN, 0, list);
}
static inline void AddBPromoCapture(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));

	AddCapture(pos, from, to, bQ, list);
	AddCapture(pos, from, to, bR, list);
	AddCapture(pos, from, to, bB, list);
	AddCapture(pos, from, to, bN, list);
}

/* Generators for specific color/piece combinations - called by generic generators*/

// King
static inline void GenWCastling(const S_BOARD *pos, S_MOVELIST *list, const bitboard occupied) {

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
static inline void GenBCastling(const S_BOARD *pos, S_MOVELIST *list, const bitboard occupied) {

	// King side castle
	if (pos->castlePerm & BKCA)
		if (!((occupied & bitF8G8)))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos))
				AddQuiet(pos, E8, G8, EMPTY, FLAG_CASTLE, list);

	// Queen side castle
	if (pos->castlePerm & BQCA)
		if (!((occupied & bitB8C8D8)))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos))
				AddQuiet(pos, E8, C8, EMPTY, FLAG_CASTLE, list);
}
static inline void GenKingMoves(const S_BOARD *pos, S_MOVELIST *list, const int sq, const bitboard empty) {

	bitboard moves = king_attacks[sq] & empty;
	while (moves) {
		AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenKingCaptures(const S_BOARD *pos, S_MOVELIST *list, const int sq, const bitboard enemies) {

	bitboard attacks = king_attacks[sq] & enemies;
	while (attacks)
		AddCapture(pos, sq, PopLsb(&attacks), 0, list);
}

// White pawn
static inline void GenWPawnCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	int sq;
	bitboard enPassers;

	bitboard pawns       = pos->colorBBs[WHITE] & pos->pieceBBs[PAWN];
	bitboard lAttacks    = ((pawns & ~fileABB) << 7) & enemies;
	bitboard rAttacks    = ((pawns & ~fileHBB) << 9) & enemies;
	bitboard lNormalCap  = lAttacks & ~rank7BB;
	bitboard lPromoCap   = lAttacks &  rank7BB;
	bitboard rNormalCap  = rAttacks & ~rank7BB;
	bitboard rPromoCap   = rAttacks &  rank7BB;
	bitboard promotions  = empty & (pawns & rank7BB) << 8;

	// Capture promotions
	while (lPromoCap) {
		sq = PopLsb(&lPromoCap);
		AddWPromoCapture(pos, sq - 7, sq, list);
	}
	while (rPromoCap) {
		sq = PopLsb(&rPromoCap);
		AddWPromoCapture(pos, sq - 9, sq, list);
	}
	// Normal promotions
	while (promotions) {
		sq = PopLsb(&promotions);
		AddWPromo(pos, (sq - 8), sq, list);
	}
	// Pawn captures
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
static inline void GenWPawnBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	GenWPawnCaptures(pos, list, enemies, empty);

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
static inline void GenBPawnCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {


	int sq;
	bitboard enPassers;

	bitboard pawns       = pos->colorBBs[BLACK] & pos->pieceBBs[PAWN];
	bitboard lAttacks    = ((pawns & ~fileHBB) >> 7) & enemies;
	bitboard rAttacks    = ((pawns & ~fileABB) >> 9) & enemies;
	bitboard lNormalCap  = lAttacks & ~rank2BB;
	bitboard lPromoCap   = lAttacks &  rank2BB;
	bitboard rNormalCap  = rAttacks & ~rank2BB;
	bitboard rPromoCap   = rAttacks &  rank2BB;
	bitboard promotions  = empty & (pawns & rank2BB) >> 8;

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
	// Pawn captures
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
static inline void GenBPawnBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	GenBPawnCaptures(pos, list, enemies, empty);

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
static inline void GenWKnightBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	int sq;
	bitboard attacks, moves;

	bitboard knights = pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);

		moves = knight_attacks[sq] & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenWKnightCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies) {

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
static inline void GenBKnightBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	int sq;
	bitboard attacks, moves;

	bitboard knights = pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);

		moves = knight_attacks[sq] & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenBKnightCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies) {

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
static inline void GenWBishopBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard bishops = pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		temp = BishopAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenWBishopCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

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
static inline void GenBBishopBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard bishops = pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		temp = BishopAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenBBishopCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

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
static inline void GenWRookBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard rooks = pos->colorBBs[WHITE] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		temp = RookAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenWRookCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

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
static inline void GenBRookBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard rooks = pos->colorBBs[BLACK] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		temp = RookAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenBRookCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

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
static inline void GenWQueenBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard queens  = pos->colorBBs[WHITE] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		temp = BishopAttacks(sq, occupied) | RookAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenWQueenCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

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
static inline void GenBQueenBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard queens  = pos->colorBBs[BLACK] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		temp = BishopAttacks(sq, occupied) | RookAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCapture(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuiet(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenBQueenCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

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

// Generate all possible moves
void GenAllMoves(const S_BOARD *pos, S_MOVELIST *list) {

	assert(CheckBoard(pos));

	list->count = list->next = 0;

	const int side = pos->side;

	const bitboard enemies 	= pos->colorBBs[!side];
	const bitboard occupied = pos->colorBBs[BOTH];
	const bitboard empty    = ~occupied;

	if (side == WHITE) {
		GenWCastling  (pos, list, occupied);
		GenWPawnBoth  (pos, list, enemies, empty);
		GenWKnightBoth(pos, list, enemies, empty);
		GenWRookBoth  (pos, list, enemies, empty, occupied);
		GenWBishopBoth(pos, list, enemies, empty, occupied);
		GenWQueenBoth (pos, list, enemies, empty, occupied);
	} else {
		GenBCastling  (pos, list, occupied);
		GenBPawnBoth  (pos, list, enemies, empty);
		GenBKnightBoth(pos, list, enemies, empty);
		GenBRookBoth  (pos, list, enemies, empty, occupied);
		GenBBishopBoth(pos, list, enemies, empty, occupied);
		GenBQueenBoth (pos, list, enemies, empty, occupied);
	}

	GenKingCaptures(pos, list, pos->kingSq[side], enemies);
	GenKingMoves   (pos, list, pos->kingSq[side], empty  );

	assert(MoveListOk(list, pos));
}

// Generate all moves that capture pieces
void GenNoisyMoves(const S_BOARD *pos, S_MOVELIST *list) {

	assert(CheckBoard(pos));

	list->count = list->next = 0;

	const int side = pos->side;

	const bitboard occupied = pos->colorBBs[BOTH];
	const bitboard enemies  = pos->colorBBs[!side];
	const bitboard empty    = ~occupied;

	// Pawns
	if (side == WHITE) {
		GenWPawnCaptures  (pos, list, enemies, empty);
		GenWKnightCaptures(pos, list, enemies);
		GenWRookCaptures  (pos, list, enemies, occupied);
		GenWBishopCaptures(pos, list, enemies, occupied);
		GenWQueenCaptures (pos, list, enemies, occupied);

	} else {
		GenBPawnCaptures  (pos, list, enemies, empty);
		GenBKnightCaptures(pos, list, enemies);
		GenBRookCaptures  (pos, list, enemies, occupied);
		GenBBishopCaptures(pos, list, enemies, occupied);
		GenBQueenCaptures (pos, list, enemies, occupied);
	}

	GenKingCaptures(pos, list, pos->kingSq[side], enemies);

	assert(MoveListOk(list, pos));
}

// Checks the given move is legal in the given position
int MoveExists(S_BOARD *pos, const int move) {

	S_MOVELIST list[1];

	// Generate all moves in the position
	GenAllMoves(pos, list);

	// Loop through them, looking for a match
	for (unsigned int i = 0; i < list->count; ++i) {

		if (!(list->moves[i].move == move))
			continue;

		// The movegen gens some illegal moves, so we must verify it is legal.
		// If it isn't legal then the given move is not legal in the position.
		// MakeMove takes it back immediately so we just break and return false.
		if (!MakeMove(pos, list->moves[i].move))
			break;

		// If it's legal we take it back and return true.
		TakeMove(pos);
		return true;
	}
	return false;
}