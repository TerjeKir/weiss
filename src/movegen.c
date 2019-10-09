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

/* Functions that add moves to the movelist */

static void AddQuietMove(const S_BOARD *pos, const int move, S_MOVELIST *list) {

	const int from = FROMSQ(move);
	const int   to =   TOSQ(move);

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

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

static void AddCaptureMove(const S_BOARD *pos, const int move, S_MOVELIST *list) {
#ifndef NDEBUG
	const int to    = TOSQ(move);
#endif
	const int from  = FROMSQ(move);
	const int piece = CAPTURED(move);

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(ValidPiece(piece));
	assert(CheckBoard(pos));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[piece][pos->board[from]] + 1000000;
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

static void AddWhitePawnCapMove(const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(ValidPieceOrEmpty(cap));
	assert(CheckBoard(pos));

	if (rankOf(from) == RANK_7) {
		AddCaptureMove(pos, MOVE(from, to, cap, wQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wN, 0), list);
	} else
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
}

static void AddWhitePawnMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));

	if (rankOf(from) == RANK_7) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, wQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wN, 0), list);
	} else
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
}

static void AddBlackPawnCapMove(const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(ValidPieceOrEmpty(cap));
	assert(CheckBoard(pos));

	if (rankOf(from) == RANK_2) {
		AddCaptureMove(pos, MOVE(from, to, cap, bQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bN, 0), list);
	} else
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
}

static void AddBlackPawnMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));

	if (rankOf(from) == RANK_2) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, bQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bN, 0), list);
	} else
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
}

/* Functions that generate specific color/piece moves */

// King
static inline void GenerateWhiteCastling(const S_BOARD *pos, S_MOVELIST *list, const bitboard occupied) {

	// King side castle
	if (pos->castlePerm & WKCA)
		if (!(occupied & bitF1G1))
			if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos))
				AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, FLAG_CASTLE), list);

	// Queen side castle
	if (pos->castlePerm & WQCA)
		if (!(occupied & bitB1C1D1))
			if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos))
				AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, FLAG_CASTLE), list);
}

static inline void GenerateBlackCastling(const S_BOARD *pos, S_MOVELIST *list, const bitboard occupied) {

	// King side castle
	if (pos->castlePerm & BKCA)
		if (!((occupied & bitF8G8)))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos))
				AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, FLAG_CASTLE), list);

	// Queen side castle
	if (pos->castlePerm & BQCA)
		if (!((occupied & bitB8C8D8)))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos))
				AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, FLAG_CASTLE), list);
}

static inline void GenerateKingMoves(const S_BOARD *pos, S_MOVELIST *list, const int sq, const bitboard empty, bitboard moves, int move) {

	moves = king_attacks[sq] & empty;
	while (moves) {
		move = PopLsb(&moves);
		AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
	}
}

static inline void GenerateKingCaptures(const S_BOARD *pos, S_MOVELIST *list, const int sq, const bitboard enemies, bitboard attacks, int attack) {

	attacks = king_attacks[sq] & enemies;
	while (attacks) {
		attack = PopLsb(&attacks);
		AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
	}
}

// Pawn
static inline void GenerateWhitePawnMoves(const S_BOARD *pos, S_MOVELIST *list, const bitboard pawns, bitboard pawnMoves, bitboard pawnStarts, const bitboard empty, int sq) {

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
		AddQuietMove(pos, MOVE((sq - 16), sq, EMPTY, EMPTY, FLAG_PAWNSTART), list);
	}
}

static inline void GenerateBlackPawnMoves(const S_BOARD *pos, S_MOVELIST *list, const bitboard pawns, bitboard pawnMoves, bitboard pawnStarts, const bitboard empty, int sq) {

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
		AddQuietMove(pos, MOVE((sq + 16), sq, EMPTY, EMPTY, FLAG_PAWNSTART), list);
	}
}

static inline void GenerateWhitePawnCaptures(const S_BOARD *pos, S_MOVELIST *list, bitboard pawns, const bitboard enemies, bitboard enPassers, bitboard attacks, int sq, int attack) {

	// En passant
	if (pos->enPas != NO_SQ) {
		enPassers = pawns & pawn_attacks[BLACK][pos->enPas];
		while (enPassers) {
			sq = PopLsb(&enPassers);
			AddEnPassantMove(MOVE(sq, pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
		}
	}

	// Pawn captures
	while (pawns) {
		sq = PopLsb(&pawns);
		attacks = pawn_attacks[WHITE][sq] & enemies;

		while (attacks) {
			attack = PopLsb(&attacks);
			AddWhitePawnCapMove(pos, sq, attack, pos->board[attack], list);
		}
	}
}

static inline void GenerateBlackPawnCaptures(const S_BOARD *pos, S_MOVELIST *list, bitboard pawns, const bitboard enemies, bitboard enPassers, bitboard attacks, int sq, int attack) {

	// En passant
	if (pos->enPas != NO_SQ) {
		enPassers = pawns & pawn_attacks[WHITE][pos->enPas];
		while (enPassers) {
			sq = PopLsb(&enPassers);
			AddEnPassantMove(MOVE(sq, pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
		}
	}

	// Pawn captures
	while (pawns) {
		sq = PopLsb(&pawns);
		attacks = pawn_attacks[BLACK][sq] & enemies;

		while (attacks) {
			attack = PopLsb(&attacks);
			AddBlackPawnCapMove(pos, sq, attack, pos->board[attack], list);
		}
	}
}

// Generate all possible moves
void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list) {
	assert(CheckBoard(pos));

	list->count = list->next = 0;

	int sq, attack, move;
	const int side = pos->side;

	bitboard attacks, moves;
	bitboard pawnMoves, pawnStarts, enPassers;

	const bitboard enemies 	= pos->colorBBs[!side];
	const bitboard occupied = pos->colorBBs[BOTH];
	const bitboard empty    = ~occupied;

	bitboard pawns 		= pos->colorBBs[side] & pos->pieceBBs[  PAWN];
	bitboard knights 	= pos->colorBBs[side] & pos->pieceBBs[KNIGHT];
	bitboard bishops 	= pos->colorBBs[side] & pos->pieceBBs[BISHOP];
	bitboard rooks 		= pos->colorBBs[side] & pos->pieceBBs[  ROOK];
	bitboard queens 	= pos->colorBBs[side] & pos->pieceBBs[ QUEEN];


	// Pawns and castling
	if (side == WHITE) {
		// Castling
		GenerateWhiteCastling(pos, list, occupied);

		// Pawns
		GenerateWhitePawnCaptures(pos, list, pawns, enemies, enPassers, attacks, sq, attack);
		GenerateWhitePawnMoves   (pos, list, pawns, pawnMoves, pawnStarts, empty, sq);

	} else {
		// Castling
		GenerateBlackCastling(pos, list, occupied);

		// Pawns
		GenerateBlackPawnCaptures(pos, list, pawns, enemies, enPassers, attacks, sq, attack);
		GenerateBlackPawnMoves   (pos, list, pawns, pawnMoves, pawnStarts, empty, sq);
	}

	// Knights
	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
		}
		moves = knight_attacks[sq] & empty;
		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
	}

	// Rooks
	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = RookAttacks(sq, occupied) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
		}
		moves = RookAttacks(sq, occupied) & empty;
		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
	}

	// Bishops
	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = BishopAttacks(sq, occupied) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
		}
		moves = BishopAttacks(sq, occupied) & empty;
		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
	}

	// Queens
	while (queens) {

		sq = PopLsb(&queens);

		const bitboard tempQueen = BishopAttacks(sq, occupied) | RookAttacks(sq, occupied);
		attacks = tempQueen & enemies;
		moves   = tempQueen & empty;

		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
		}

		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
	}

	// King
	sq = pos->kingSq[side];
	GenerateKingCaptures(pos, list, sq, enemies, attacks, attack);
	GenerateKingMoves   (pos, list, sq, empty, moves, move);

	assert(MoveListOk(list, pos));
}

// Generate all moves that capture pieces
void GenerateAllCaptures(const S_BOARD *pos, S_MOVELIST *list) {
	assert(CheckBoard(pos));

	list->count = list->next = 0;

	int sq, attack;
	const int side = pos->side;

	bitboard attacks, enPassers;

	const bitboard occupied = pos->colorBBs[BOTH];
	const bitboard enemies  = pos->colorBBs[!side];

	bitboard pawns 		= pos->colorBBs[side] & pos->pieceBBs[  PAWN];
	bitboard knights 	= pos->colorBBs[side] & pos->pieceBBs[KNIGHT];
	bitboard bishops 	= pos->colorBBs[side] & pos->pieceBBs[BISHOP];
	bitboard rooks 		= pos->colorBBs[side] & pos->pieceBBs[  ROOK];
	bitboard queens 	= pos->colorBBs[side] & pos->pieceBBs[ QUEEN];


	// Pawns
	if (side == WHITE)
		GenerateWhitePawnCaptures(pos, list, pawns, enemies, enPassers, attacks, sq, attack);
	else
		GenerateBlackPawnCaptures(pos, list, pawns, enemies, enPassers, attacks, sq, attack);

	// Knights
	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
		}
	}

	// Rooks
	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = RookAttacks(sq, occupied) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
		}
	}

	// Bishops
	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = BishopAttacks(sq, occupied) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
		}
	}

	// Queens
	while (queens) {

		sq = PopLsb(&queens);

		attacks = (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied)) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->board[attack], EMPTY, 0), list);
		}
	}

	// King
	sq = pos->kingSq[side];
	GenerateKingCaptures(pos, list, sq, enemies, attacks, attack);

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