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

static void AddQuietMove(const S_BOARD *pos, const int from, const int to, const int promo, const int flag, S_MOVELIST *list) {

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
static void AddCaptureMove(const S_BOARD *pos, const int from, const int to, const int promo, S_MOVELIST *list) {

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
static void AddEnPassantMove(const int move, S_MOVELIST *list) {
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
static void AddWhitePawnCapMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(ValidPieceOrEmpty(cap));
	assert(CheckBoard(pos));

	if (rankOf(from) == RANK_7) {
		AddCaptureMove(pos, from, to, wQ, list);
		AddCaptureMove(pos, from, to, wR, list);
		AddCaptureMove(pos, from, to, wB, list);
		AddCaptureMove(pos, from, to, wN, list);
	} else
		AddCaptureMove(pos, from, to, EMPTY, list);
}
static void AddWhitePawnMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));

	if (rankOf(from) == RANK_7) {
		AddQuietMove(pos, from, to, wQ, 0, list);
		AddQuietMove(pos, from, to, wR, 0, list);
		AddQuietMove(pos, from, to, wB, 0, list);
		AddQuietMove(pos, from, to, wN, 0, list);
	} else
		AddQuietMove(pos, from, to, EMPTY, 0, list);
}
static void AddBlackPawnCapMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(ValidPieceOrEmpty(cap));
	assert(CheckBoard(pos));

	if (rankOf(from) == RANK_2) {
		AddCaptureMove(pos, from, to, bQ, list);
		AddCaptureMove(pos, from, to, bR, list);
		AddCaptureMove(pos, from, to, bB, list);
		AddCaptureMove(pos, from, to, bN, list);
	} else
		AddCaptureMove(pos, from, to, EMPTY, list);
}
static void AddBlackPawnMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));

	if (rankOf(from) == RANK_2) {
		AddQuietMove(pos, from, to, bQ, 0, list);
		AddQuietMove(pos, from, to, bR, 0, list);
		AddQuietMove(pos, from, to, bB, 0, list);
		AddQuietMove(pos, from, to, bN, 0, list);
	} else
		AddQuietMove(pos, from, to, EMPTY, 0, list);
}

/* Generators for specific color/piece combinations - called by generic generators*/

// King
static inline void GenerateWhiteCastling(const S_BOARD *pos, S_MOVELIST *list, const bitboard occupied) {

	// King side castle
	if (pos->castlePerm & WKCA)
		if (!(occupied & bitF1G1))
			if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos))
				AddQuietMove(pos, E1, G1, EMPTY, FLAG_CASTLE, list);

	// Queen side castle
	if (pos->castlePerm & WQCA)
		if (!(occupied & bitB1C1D1))
			if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos))
				AddQuietMove(pos, E1, C1, EMPTY, FLAG_CASTLE, list);
}
static inline void GenerateBlackCastling(const S_BOARD *pos, S_MOVELIST *list, const bitboard occupied) {

	// King side castle
	if (pos->castlePerm & BKCA)
		if (!((occupied & bitF8G8)))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos))
				AddQuietMove(pos, E8, G8, EMPTY, FLAG_CASTLE, list);

	// Queen side castle
	if (pos->castlePerm & BQCA)
		if (!((occupied & bitB8C8D8)))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos))
				AddQuietMove(pos, E8, C8, EMPTY, FLAG_CASTLE, list);
}
static inline void GenerateKingMoves(const S_BOARD *pos, S_MOVELIST *list, const int sq, const bitboard empty) {

	bitboard moves = king_attacks[sq] & empty;
	while (moves) {
		AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateKingCaptures(const S_BOARD *pos, S_MOVELIST *list, const int sq, const bitboard enemies) {

	bitboard attacks = king_attacks[sq] & enemies;
	while (attacks)
		AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);
}

// White pawn
static inline void GenerateWhitePawnCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, bitboard pawns) {

	int sq;
	bitboard attacks, enPassers;

	// En passant
	if (pos->enPas != NO_SQ) {
		enPassers = pawns & pawn_attacks[BLACK][pos->enPas];
		while (enPassers)
			AddEnPassantMove(MOVE(PopLsb(&enPassers), pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
	}

	// Pawn captures
	while (pawns) {
		sq = PopLsb(&pawns);

		attacks = pawn_attacks[WHITE][sq] & enemies;

		while (attacks)
			AddWhitePawnCapMove(pos, sq, PopLsb(&attacks), list);
	}
}
static inline void GenerateWhitePawnBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	bitboard pawns = pos->colorBBs[WHITE] & pos->pieceBBs[PAWN];

	GenerateWhitePawnCaptures(pos, list, enemies, pawns);

	int sq;
	bitboard pawnMoves, pawnStarts;

	pawnMoves  = empty & pawns << 8;
	pawnStarts = empty & (pawnMoves & rank3BB) << 8;

	// Pawn moves
	while (pawnMoves) {
		sq = PopLsb(&pawnMoves);
		AddWhitePawnMove(pos, (sq - 8), sq, list);
	}

	// Pawn starts
	while (pawnStarts) {
		sq = PopLsb(&pawnStarts);
		AddQuietMove(pos, (sq - 16), sq, EMPTY, FLAG_PAWNSTART, list);
	}
}
// Black pawn
static inline void GenerateBlackPawnCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, bitboard pawns) {

	int sq;
	bitboard attacks, enPassers;

	// En passant
	if (pos->enPas != NO_SQ) {
		enPassers = pawns & pawn_attacks[WHITE][pos->enPas];
		while (enPassers)
			AddEnPassantMove(MOVE(PopLsb(&enPassers), pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
	}

	// Pawn captures
	while (pawns) {
		sq = PopLsb(&pawns);

		attacks = pawn_attacks[BLACK][sq] & enemies;

		while (attacks)
			AddBlackPawnCapMove(pos, sq, PopLsb(&attacks), list);
	}
}
static inline void GenerateBlackPawnBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	bitboard pawns = pos->colorBBs[BLACK] & pos->pieceBBs[PAWN];

	GenerateBlackPawnCaptures(pos, list, enemies, pawns);

	int sq;
	bitboard pawnMoves, pawnStarts;

	pawnMoves  = empty & pawns >> 8;
	pawnStarts = empty & (pawnMoves & rank6BB) >> 8;

	// Pawn moves
	while (pawnMoves) {
		sq = PopLsb(&pawnMoves);
		AddBlackPawnMove(pos, (sq + 8), sq, list);
	}

	// Pawn starts
	while (pawnStarts) {
		sq = PopLsb(&pawnStarts);
		AddQuietMove(pos, (sq + 16), sq, EMPTY, FLAG_PAWNSTART, list);
	}
}

// White knight
static inline void GenerateWhiteKnightBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	int sq;
	bitboard attacks, moves;

	bitboard knights = pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);

		moves = knight_attacks[sq] & empty;
		while (moves)
			AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateWhiteKnightCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies) {

	int sq;
	bitboard attacks;

	bitboard knights = pos->colorBBs[WHITE] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), EMPTY, list);
	}
}
// Black knight
static inline void GenerateBlackKnightBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty) {

	int sq;
	bitboard attacks, moves;

	bitboard knights = pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);

		moves = knight_attacks[sq] & empty;
		while (moves)
			AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateBlackKnightCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies) {

	int sq;
	bitboard attacks;

	bitboard knights = pos->colorBBs[BLACK] & pos->pieceBBs[KNIGHT];

	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);
	}
}

// White bishop
static inline void GenerateWhiteBishopBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard bishops = pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		temp = BishopAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateWhiteBishopCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard bishops = pos->colorBBs[WHITE] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = BishopAttacks(sq, occupied) & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);
	}
}
// Black bishop
static inline void GenerateBlackBishopBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard bishops = pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		temp = BishopAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateBlackBishopCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard bishops = pos->colorBBs[BLACK] & pos->pieceBBs[BISHOP];

	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = BishopAttacks(sq, occupied) & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);
	}
}

// White rook
static inline void GenerateWhiteRookBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard rooks = pos->colorBBs[WHITE] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		temp = RookAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateWhiteRookCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard rooks = pos->colorBBs[WHITE] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = RookAttacks(sq, occupied) & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);
	}
}
// Black rook
static inline void GenerateBlackRookBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard rooks = pos->colorBBs[BLACK] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		temp = RookAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateBlackRookCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard rooks = pos->colorBBs[BLACK] & pos->pieceBBs[ROOK];

	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = RookAttacks(sq, occupied) & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);
	}
}

// White queen
static inline void GenerateWhiteQueenBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard queens  = pos->colorBBs[WHITE] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		temp = BishopAttacks(sq, occupied) | RookAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateWhiteQueenCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard queens  = pos->colorBBs[WHITE] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		attacks = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);
	}
}
// Black queen
static inline void GenerateBlackQueenBoth(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard empty, const bitboard occupied) {

	int sq;
	bitboard attacks, moves, temp;

	bitboard queens  = pos->colorBBs[BLACK] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		temp = BishopAttacks(sq, occupied) | RookAttacks(sq, occupied);
		attacks = temp & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);

		moves = temp & empty;
		while (moves)
			AddQuietMove(pos, sq, PopLsb(&moves), EMPTY, 0, list);
	}
}
static inline void GenerateBlackQueenCaptures(const S_BOARD *pos, S_MOVELIST *list, const bitboard enemies, const bitboard occupied) {

	int sq;
	bitboard attacks;

	bitboard queens  = pos->colorBBs[BLACK] & pos->pieceBBs[ QUEEN];

	while (queens) {

		sq = PopLsb(&queens);

		attacks = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & enemies;
		while (attacks)
			AddCaptureMove(pos, sq, PopLsb(&attacks), 0, list);
	}
}

/* Generic generators */

// Generate all possible moves
void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list) {

	assert(CheckBoard(pos));

	list->count = list->next = 0;

	const int side = pos->side;

	const bitboard enemies 	= pos->colorBBs[!side];
	const bitboard occupied = pos->colorBBs[BOTH];
	const bitboard empty    = ~occupied;

	if (side == WHITE) {
		GenerateWhiteCastling  (pos, list, occupied);
		GenerateWhitePawnBoth  (pos, list, enemies, empty);
		GenerateWhiteKnightBoth(pos, list, enemies, empty);
		GenerateWhiteRookBoth  (pos, list, enemies, empty, occupied);
		GenerateWhiteBishopBoth(pos, list, enemies, empty, occupied);
		GenerateWhiteQueenBoth (pos, list, enemies, empty, occupied);
	} else {
		GenerateBlackCastling  (pos, list, occupied);
		GenerateBlackPawnBoth  (pos, list, enemies, empty);
		GenerateBlackKnightBoth(pos, list, enemies, empty);
		GenerateBlackRookBoth  (pos, list, enemies, empty, occupied);
		GenerateBlackBishopBoth(pos, list, enemies, empty, occupied);
		GenerateBlackQueenBoth (pos, list, enemies, empty, occupied);
	}

	GenerateKingCaptures(pos, list, pos->kingSq[side], enemies);
	GenerateKingMoves   (pos, list, pos->kingSq[side], empty  );

	assert(MoveListOk(list, pos));
}

// Generate all moves that capture pieces
void GenerateAllCaptures(const S_BOARD *pos, S_MOVELIST *list) {

	assert(CheckBoard(pos));

	list->count = list->next = 0;

	const int side = pos->side;

	const bitboard occupied = pos->colorBBs[BOTH];
	const bitboard enemies  = pos->colorBBs[!side];

	bitboard pawns = pos->colorBBs[side] & pos->pieceBBs[PAWN];

	// Pawns
	if (side == WHITE) {
		GenerateWhitePawnCaptures  (pos, list, enemies, pawns);
		GenerateWhiteKnightCaptures(pos, list, enemies);
		GenerateWhiteRookCaptures  (pos, list, enemies, occupied);
		GenerateWhiteBishopCaptures(pos, list, enemies, occupied);
		GenerateWhiteQueenCaptures (pos, list, enemies, occupied);
	} else {
		GenerateBlackPawnCaptures  (pos, list, enemies, pawns);
		GenerateBlackKnightCaptures(pos, list, enemies);
		GenerateBlackRookCaptures  (pos, list, enemies, occupied);
		GenerateBlackBishopCaptures(pos, list, enemies, occupied);
		GenerateBlackQueenCaptures (pos, list, enemies, occupied);
	}

	GenerateKingCaptures(pos, list, pos->kingSq[side], enemies);

	assert(MoveListOk(list, pos));
}

// Checks the given move is legal in the given position
int MoveExists(S_BOARD *pos, const int move) {

	S_MOVELIST list[1];

	// Generate all moves in the position
	GenerateAllMoves(pos, list);

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