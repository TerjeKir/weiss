// movegen.c

#include <stdio.h>

#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "data.h"
#include "makemove.h"
#include "move.h"
#include "validate.h"


const bitboard bitB1C1D1 = (1ULL <<  1) | (1ULL <<  2) | (1ULL <<  3);
const bitboard bitB8C8D8 = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);
const bitboard bitF1G1   = (1ULL <<  5) | (1ULL <<  6);
const bitboard bitF8G8   = (1ULL << 61) | (1ULL << 62);

int MvvLvaScores[13][13];

// Initializes the MostValuableVictim-LeastValuableAttacker scores used for ordering captures
void InitMvvLva() {

	const int VictimScore[13]   = {0, 106, 206, 306, 406, 506, 606, 106, 206, 306, 406, 506, 606};
	const int AttackerScore[13] = {0,   1,   2,   3,   4,   5,   6,   1,   2,   3,   4,   5,   6};

	for (int Attacker = wP; Attacker <= bK; ++Attacker)
		for (int Victim = wP; Victim <= bK; ++Victim)
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] - AttackerScore[Attacker];
}

/* Functions that add moves to the movelist */

static void AddQuietMove(const S_BOARD *pos, const int move, S_MOVELIST *list) {

	int from = FROMSQ(move);
	int   to =   TOSQ(move);

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
		list->moves[list->count].score = pos->searchHistory[pos->pieces[from]][to];

	list->count++;
}

static void AddCaptureMove(const S_BOARD *pos, const int move, S_MOVELIST *list) {
#ifndef NDEBUG
	int to    = TOSQ(move);
#endif
	int from  = FROMSQ(move);
	int piece = CAPTURED(move);

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(PieceValid(piece));
	assert(CheckBoard(pos));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[piece][pos->pieces[from]] + 1000000;
	list->count++;
}

static void AddEnPassantMove(const int move, S_MOVELIST *list) {
#ifndef NDEBUG
	int from = FROMSQ(move);
	int   to =   TOSQ(move);

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
	assert(PieceValidEmpty(cap));
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
	assert(PieceValidEmpty(cap));
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
static inline void GenerateWhiteCastling(const S_BOARD *pos, S_MOVELIST *list, const bitboard allPieces) {

	// King side castle
	if (pos->castlePerm & WKCA)
		if (!(allPieces & bitF1G1))
			if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos))
				AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, FLAG_CASTLE), list);

	// Queen side castle
	if (pos->castlePerm & WQCA)
		if (!(allPieces & bitB1C1D1))
			if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos))
				AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, FLAG_CASTLE), list);
}

static inline void GenerateBlackCastling(const S_BOARD *pos, S_MOVELIST *list, const bitboard allPieces) {

	// King side castle
	if (pos->castlePerm & BKCA)
		if (!((allPieces & bitF8G8)))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos))
				AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, FLAG_CASTLE), list);

	// Queen side castle
	if (pos->castlePerm & BQCA)
		if (!((allPieces & bitB8C8D8)))
			if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos))
				AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, FLAG_CASTLE), list);
}

static inline void GenerateKingMoves(const S_BOARD *pos, S_MOVELIST *list, const int sq, const bitboard empty) {

	int move;
	bitboard moves;

	moves = king_attacks[sq] & empty;
	while (moves) {
		move = PopLsb(&moves);
		AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
	}
}

static inline void GenerateKingCaptures(const S_BOARD *pos, S_MOVELIST *list, const int sq, const bitboard enemies) {

	int attack;
	bitboard attacks;

	attacks = king_attacks[sq] & enemies;
	while (attacks) {
		attack = PopLsb(&attacks);
		AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
	}
}

// Pawn
static inline void GenerateWhitePawnMoves(const S_BOARD *pos, S_MOVELIST *list, const bitboard pawns, const bitboard empty) {

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
		AddQuietMove(pos, MOVE((sq - 16), sq, EMPTY, EMPTY, FLAG_PAWNSTART), list);
	}
}

static inline void GenerateBlackPawnMoves(const S_BOARD *pos, S_MOVELIST *list, const bitboard pawns, const bitboard empty) {

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
		AddQuietMove(pos, MOVE((sq + 16), sq, EMPTY, EMPTY, FLAG_PAWNSTART), list);
	}
}

static inline void GenerateWhitePawnCaptures(const S_BOARD *pos, S_MOVELIST *list, bitboard pawns, const bitboard enemies) {

	int sq, attack;
	bitboard attacks, enPassers;

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
			AddWhitePawnCapMove(pos, sq, attack, pos->pieces[attack], list);
		}
	}
}

static inline void GenerateBlackPawnCaptures(const S_BOARD *pos, S_MOVELIST *list, bitboard pawns, const bitboard enemies) {

	int sq, attack;
	bitboard attacks, enPassers;

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
			AddBlackPawnCapMove(pos, sq, attack, pos->pieces[attack], list);
		}
	}
}

// Generate all possible moves
void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list) {
	assert(CheckBoard(pos));

	list->count = 0;

	int sq, attack, move;
	int side = pos->side;

	bitboard attacks, moves, tempQueen;

	bitboard allPieces  = pos->allBB;
	bitboard empty		= ~allPieces;
	bitboard enemies 	= pos->colors[!side];

	bitboard pawns 		= pos->colors[side] & pos->pieceBBs[  PAWN];
	bitboard knights 	= pos->colors[side] & pos->pieceBBs[KNIGHT];
	bitboard bishops 	= pos->colors[side] & pos->pieceBBs[BISHOP];
	bitboard rooks 		= pos->colors[side] & pos->pieceBBs[  ROOK];
	bitboard queens 	= pos->colors[side] & pos->pieceBBs[ QUEEN];


	// Pawns and castling
	if (side == WHITE) {
		// Castling
		GenerateWhiteCastling(pos, list, allPieces);

		// Pawns
		GenerateWhitePawnCaptures(pos, list, pawns, enemies);
		GenerateWhitePawnMoves   (pos, list, pawns, empty);

	} else {
		// Castling
		GenerateBlackCastling(pos, list, allPieces);

		// Pawns
		GenerateBlackPawnCaptures(pos, list, pawns, enemies);
		GenerateBlackPawnMoves   (pos, list, pawns, empty);
	}

	// Knights
	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
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

		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mRookTable) & empty;
		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
	}

	// Bishops
	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mBishopTable) & empty;
		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
	}

	// Queens
	while (queens) {

		sq = PopLsb(&queens);

		tempQueen = SliderAttacks(sq, allPieces, mBishopTable) | SliderAttacks(sq, allPieces, mRookTable);
		attacks   = enemies & tempQueen;
		moves     = empty   & tempQueen;

		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
		}

		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
	}

	// King
	sq = pos->KingSq[side];
	GenerateKingCaptures(pos, list, sq, enemies);
	GenerateKingMoves   (pos, list, sq, empty);

	assert(MoveListOk(list, pos));
}

// Generate all moves that capture pieces
void GenerateAllCaptures(const S_BOARD *pos, S_MOVELIST *list) {
	assert(CheckBoard(pos));

	list->count = 0;

	int sq, attack;
	int side = pos->side;

	bitboard attacks;

	bitboard allPieces  = pos->allBB;
	bitboard enemies 	= pos->colors[!side];

	bitboard pawns 		= pos->colors[side] & pos->pieceBBs[  PAWN];
	bitboard knights 	= pos->colors[side] & pos->pieceBBs[KNIGHT];
	bitboard bishops 	= pos->colors[side] & pos->pieceBBs[BISHOP];
	bitboard rooks 		= pos->colors[side] & pos->pieceBBs[  ROOK];
	bitboard queens 	= pos->colors[side] & pos->pieceBBs[ QUEEN];


	// Pawns
	if (side == WHITE)
		GenerateWhitePawnCaptures(pos, list, pawns, enemies);
	else
		GenerateBlackPawnCaptures(pos, list, pawns, enemies);

	// Knights
	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// Rooks
	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// Bishops
	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// Queens
	while (queens) {

		sq = PopLsb(&queens);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// King
	sq = pos->KingSq[side];
	GenerateKingCaptures(pos, list, sq, enemies);

	assert(MoveListOk(list, pos));
}

// Checks the given move is legal in the given position
int MoveExists(S_BOARD *pos, const int move) {

	S_MOVELIST list[1];

	// Generate all moves in the position
	GenerateAllMoves(pos, list);

	// Loop through them, looking for a match
	for (int i = 0; i < list->count; ++i) {

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