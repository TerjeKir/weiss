// movegen.c

#include <stdio.h>

#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "data.h"
#include "makemove.h"
#include "move.h"
#include "validate.h"


const bitboard bitB1C1D1 = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
const bitboard bitF1G1 = (1ULL << 5) | (1ULL << 6);
const bitboard bitB8C8D8 = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);
const bitboard bitF8G8 = (1ULL << 61) | (1ULL << 62);

int MvvLvaScores[13][13];

/*
PV Move
Cap -> MvvLVA
Killers
HistoryScore
*/

void InitMvvLva() {

	const int VictimScore[13]   = {0, 106, 206, 306, 406, 506, 606, 106, 206, 306, 406, 506, 606};
	const int AttackerScore[13] = {0,   1,   2,   3,   4,   5,   6,   1,   2,   3,   4,   5,   6};

	for (int Attacker = wP; Attacker <= bK; ++Attacker)
		for (int Victim = wP; Victim <= bK; ++Victim)
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] - (AttackerScore[Attacker]);
}

static void AddQuietMove(const S_BOARD *pos, int move, S_MOVELIST *list) {

	int from = FROMSQ(move);
	int   to =   TOSQ(move);

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(CheckBoard(pos));
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	list->moves[list->count].move = move;

	if (pos->searchKillers[0][pos->ply] == move)
		list->moves[list->count].score = 900000;
	else if (pos->searchKillers[1][pos->ply] == move)
		list->moves[list->count].score = 800000;
	else
		list->moves[list->count].score = pos->searchHistory[pos->pieces[from]][to];

	list->count++;
}

static void AddCaptureMove(const S_BOARD *pos, int move, S_MOVELIST *list) {

	int from  = FROMSQ(move);
#ifndef NDEBUG
	int to    = TOSQ(move);
#endif
	int piece = CAPTURED(move);

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(PieceValid(piece));
	assert(CheckBoard(pos));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[piece][pos->pieces[from]] + 1000000;
	list->count++;
}

static void AddEnPassantMove(int move, S_MOVELIST *list) {
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

// Generate all possible moves
void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list) {

	assert(CheckBoard(pos));

	list->count = 0;

	int sq, attack, move;
	int side = pos->side;

	bitboard attacks, moves;
	bitboard pawnMoves, pawnStarts, enPassers;

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

		// Pawns
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

		// En passant
		if (pos->enPas != NO_SQ) {
			enPassers = pawns & pawn_attacks[!side][pos->enPas];
			while (enPassers) {
				sq = PopLsb(&enPassers);
				AddEnPassantMove(MOVE(sq, pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
			}
		}

		// Pawn captures
		while (pawns) {
			sq = PopLsb(&pawns);
			attacks = pawn_attacks[side][sq] & enemies;

			while (attacks) {
				attack = PopLsb(&attacks);
				AddWhitePawnCapMove(pos, sq, attack, pos->pieces[attack], list);
			}
		}

	} else {

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

		// Pawns
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

		// En passant
		if (pos->enPas != NO_SQ) {
			enPassers = pawns & pawn_attacks[!side][pos->enPas];
			while (enPassers) {
				sq = PopLsb(&enPassers);
				AddEnPassantMove(MOVE(sq, pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
			}
		}

		// Pawn captures
		while (pawns) {
			sq = PopLsb(&pawns);
			attacks = pawn_attacks[side][sq] & enemies;

			while (attacks) {
				attack = PopLsb(&attacks);
				AddBlackPawnCapMove(pos, sq, attack, pos->pieces[attack], list);
			}
		}
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
		moves = SliderAttacks(sq, allPieces, mBishopTable) & empty;
		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mRookTable) & empty;
		while (moves) {
			move = PopLsb(&moves);
			AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
		}
	}

	// King
	sq = pos->KingSq[side];

	attacks = king_attacks[sq] & enemies;
	while (attacks) {
		attack = PopLsb(&attacks);
		AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
	}
	moves = king_attacks[sq] & empty;
	while (moves) {
		move = PopLsb(&moves);
		AddQuietMove(pos, MOVE(sq, move, EMPTY, EMPTY, 0), list);
	}

	assert(MoveListOk(list, pos));
}

// Generate all moves that capture pieces
void GenerateAllCaptures(const S_BOARD *pos, S_MOVELIST *list) {

	assert(CheckBoard(pos));

	list->count = 0;

	int sq, attack;
	int side = pos->side;

	bitboard attacks, enPassant;

	bitboard allPieces  = pos->allBB;
	bitboard enemies 	= pos->colors[!side];

	bitboard pawns 		= pos->colors[side] & pos->pieceBBs[  PAWN];
	bitboard knights 	= pos->colors[side] & pos->pieceBBs[KNIGHT];
	bitboard bishops 	= pos->colors[side] & pos->pieceBBs[BISHOP];
	bitboard rooks 		= pos->colors[side] & pos->pieceBBs[  ROOK];
	bitboard queens 	= pos->colors[side] & pos->pieceBBs[ QUEEN];


	// Pawns
	if (side == WHITE) {

		while (pawns) {

			sq = PopLsb(&pawns);
			assert(ValidSquare(sq));

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(MOVE(sq, pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = PopLsb(&attacks);
				AddWhitePawnCapMove(pos, sq, attack, pos->pieces[attack], list);
			}
		}

	} else {

		while (pawns) {

			sq = PopLsb(&pawns);
			assert(ValidSquare(sq));

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(MOVE(sq, pos->enPas, EMPTY, EMPTY, FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = PopLsb(&attacks);
				AddBlackPawnCapMove(pos, sq, attack, pos->pieces[attack], list);
			}
		}
	}

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

	attacks = king_attacks[sq] & enemies;
	while (attacks) {
		attack = PopLsb(&attacks);
		AddCaptureMove(pos, MOVE(sq, attack, pos->pieces[attack], EMPTY, 0), list);
	}

	assert(MoveListOk(list, pos));
}

int MoveExists(S_BOARD *pos, const int move) {

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		TakeMove(pos);

		if (list->moves[MoveNum].move == move)
			return TRUE;
	}
	return FALSE;
}