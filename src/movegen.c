// movegen.c

#include <stdio.h>

#include "defs.h"
#include "attack.h"
#include "bitboards.h"
#include "makemove.h"
#include "movegen.h"
#include "validate.h"
#include "board.h"

#define MOVE(f, t, ca, pro, fl) ((f) | ((t) << 7) | ((ca) << 14) | ((pro) << 20) | (fl))
#define SQOFFBOARD(sq) (FilesBrd[(sq)] == OFFBOARD)

bitboard bitB1C1D1 = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
bitboard bitF1G1 = (1ULL << 5) | (1ULL << 6);
bitboard bitB8C8D8 = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);
bitboard bitF8G8 = (1ULL << 61) | (1ULL << 62);

const int LoopSlidePce[8] = {
	wB, wR, wQ, 0, bB, bR, bQ, 0};

const int LoopNonSlidePce[6] = {
	wN, wK, 0, bN, bK, 0};

const int LoopSlideIndex[2] = {0, 4};
const int LoopNonSlideIndex[2] = {0, 3};

const int PceDir[13][8] = {
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{-8, -19, -21, -12, 8, 19, 21, 12},
	{-9, -11, 11, 9, 0, 0, 0, 0},
	{-1, -10, 1, 10, 0, 0, 0, 0},
	{-1, -10, 1, 10, -9, -11, 11, 9},
	{-1, -10, 1, 10, -9, -11, 11, 9},
	{0, 0, 0, 0, 0, 0, 0},
	{-8, -19, -21, -12, 8, 19, 21, 12},
	{-9, -11, 11, 9, 0, 0, 0, 0},
	{-1, -10, 1, 10, 0, 0, 0, 0},
	{-1, -10, 1, 10, -9, -11, 11, 9},
	{-1, -10, 1, 10, -9, -11, 11, 9}};

const int NumDir[13] = {
	0, 0, 8, 4, 4, 8, 8, 0, 8, 4, 4, 8, 8};

/*
PV Move
Cap -> MvvLVA
Killers
HistoryScore
*/
const int VictimScore[13] = {0, 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600};
static int MvvLvaScores[13][13];

void InitMvvLva() {

	int Attacker;
	int Victim;

	for (Attacker = wP; Attacker <= bK; ++Attacker)
		for (Victim = wP; Victim <= bK; ++Victim)
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] + 6 - (VictimScore[Attacker] / 100);
}

int MoveExists(S_BOARD *pos, const int move) {

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int MoveNum = 0;
	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move))
			continue;

		TakeMove(pos);

		if (list->moves[MoveNum].move == move)
			return TRUE;
	}
	return FALSE;
}

static void AddQuietMove(const S_BOARD *pos, int move, S_MOVELIST *list) {

	assert(SqOnBoard(FROMSQ(move)));
	assert(SqOnBoard(TOSQ(move)));
	assert(CheckBoard(pos));
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	list->moves[list->count].move = move;

	if (pos->searchKillers[0][pos->ply] == move)
		list->moves[list->count].score = 900000;
	else if (pos->searchKillers[1][pos->ply] == move)
		list->moves[list->count].score = 800000;
	else
		list->moves[list->count].score = pos->searchHistory[pos->pieces[FROMSQ(move)]][TOSQ(move)];

	list->count++;
}

static void AddCaptureMove(const S_BOARD *pos, int move, S_MOVELIST *list) {

	assert(SqOnBoard(FROMSQ(move)));
	assert(SqOnBoard(TOSQ(move)));
	assert(PieceValid(CAPTURED(move)));
	assert(CheckBoard(pos));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[CAPTURED(move)][pos->pieces[FROMSQ(move)]] + 1000000;
	list->count++;
}

static void AddEnPassantMove(const S_BOARD *pos, int move, S_MOVELIST *list) {

	assert(SqOnBoard(FROMSQ(move)));
	assert(SqOnBoard(TOSQ(move)));
	assert(CheckBoard(pos));
	assert((RanksBrd[TOSQ(move)] == RANK_6 && pos->side == WHITE) || (RanksBrd[TOSQ(move)] == RANK_3 && pos->side == BLACK));

	list->moves[list->count].move = move;
	list->moves[list->count].score = 105 + 1000000;
	list->count++;
}

static void AddWhitePawnCapMove(const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list) {

	assert(PieceValidEmpty(cap));
	assert(SqOnBoard(from));
	assert(SqOnBoard(to));
	assert(CheckBoard(pos));

	if (RanksBrd[from] == RANK_7) {
		AddCaptureMove(pos, MOVE(from, to, cap, wQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wN, 0), list);
	} else
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
}

static void AddWhitePawnMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(SqOnBoard(from));
	assert(SqOnBoard(to));
	assert(CheckBoard(pos));

	if (RanksBrd[from] == RANK_7) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, wQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wN, 0), list);
	} else
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
}

static void AddBlackPawnCapMove(const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list) {

	assert(PieceValidEmpty(cap));
	assert(SqOnBoard(from));
	assert(SqOnBoard(to));
	assert(CheckBoard(pos));

	if (RanksBrd[from] == RANK_2) {
		AddCaptureMove(pos, MOVE(from, to, cap, bQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bN, 0), list);
	} else
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
}

static void AddBlackPawnMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {

	assert(SqOnBoard(from));
	assert(SqOnBoard(to));
	assert(CheckBoard(pos));

	if (RanksBrd[from] == RANK_2) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, bQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bN, 0), list);
	} else
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
}

void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list) {

	assert(CheckBoard(pos));

	list->count = 0;

	int sq, sq120, attack, move;
	int side = pos->side;

	bitboard squareBitMask, attacks, moves, enPassant;

	bitboard allPieces  = pos->allBB;
	bitboard empty		= ~allPieces;
	bitboard enemies 	= pos->colors[!side];

	bitboard pawns 		= pos->colors[side] & pos->pieceBBs[  PAWN];
	bitboard knights 	= pos->colors[side] & pos->pieceBBs[KNIGHT];
	bitboard bishops 	= pos->colors[side] & pos->pieceBBs[BISHOP];
	bitboard rooks 		= pos->colors[side] & pos->pieceBBs[  ROOK];
	bitboard queens 	= pos->colors[side] & pos->pieceBBs[ QUEEN];
	bitboard king 		= pos->colors[side] & pos->pieceBBs[  KING];
	

	// Pawns and castling
	if (side == WHITE) {

		// King side castle
		if (pos->castlePerm & WKCA)
			if (!(allPieces & bitF1G1))
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos))
					AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MOVE_FLAG_CASTLE), list);

		// Queen side castle
		if (pos->castlePerm & WQCA)
			if (!(allPieces & bitB1C1D1))
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos))
					AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MOVE_FLAG_CASTLE), list);

		// Pawns
		while (pawns) {

			sq = PopLsb(&pawns);
			squareBitMask = 1ULL << sq;
			sq120 = SQ120(sq);
			assert(SqOnBoard(sq120));

			// Move forward
			if (!(allPieces & squareBitMask << 8)) {
				AddWhitePawnMove(pos, sq120, sq120 + 10, list);
				// Move forward two squares
				if (!(allPieces & squareBitMask << 16) && (sq < 16))
					AddQuietMove(pos, MOVE(sq120, (sq120 + 20), EMPTY, EMPTY, MOVE_FLAG_PAWNSTART), list);
			}

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(pos, MOVE(sq120, SQ120(pos->enPas), EMPTY, EMPTY, MOVE_FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = SQ120(PopLsb(&attacks));
				AddWhitePawnCapMove(pos, sq120, attack, pos->pieces[attack], list);
			}
		}

	} else {

		// King side castle
		if (pos->castlePerm & BKCA)
			if (!((allPieces & bitF8G8)))
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos))
					AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MOVE_FLAG_CASTLE), list);

		// Queen side castle
		if (pos->castlePerm & BQCA)
			if (!((allPieces & bitB8C8D8)))
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos))
					AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MOVE_FLAG_CASTLE), list);

		// Pawns
		while (pawns) {

			sq = PopLsb(&pawns);
			squareBitMask = 1ULL << sq;
			sq120 = SQ120(sq);
			assert(SqOnBoard(sq120));

			// Move forward
			if (!(allPieces & squareBitMask >> 8)) {
				AddBlackPawnMove(pos, sq120, sq120 - 10, list);
				// Move forward two squares
				if (!(allPieces & squareBitMask >> 16) && (sq > 47))
					AddQuietMove(pos, MOVE(sq120, (sq120 - 20), EMPTY, EMPTY, MOVE_FLAG_PAWNSTART), list);
			}

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(pos, MOVE(sq120, SQ120(pos->enPas), EMPTY, EMPTY, MOVE_FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = SQ120(PopLsb(&attacks));
				AddBlackPawnCapMove(pos, sq120, attack, pos->pieces[attack], list);
			}
		}
	}

	// Knights
	while (knights) {

		sq = PopLsb(&knights);
		sq120 = SQ120(sq);

		attacks = knight_attacks[sq] & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		moves = knight_attacks[sq] & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(sq120, move, EMPTY, EMPTY, 0), list);
		}
	}

	// King
	sq = Lsb(&king);
	sq120 = SQ120(sq);

	attacks = king_attacks[sq] & enemies;
	while (attacks) {
		attack = SQ120(PopLsb(&attacks));
		AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
	}
	moves = king_attacks[sq] & empty;
	while (moves) {
		move = SQ120(PopLsb(&moves));
		AddQuietMove(pos, MOVE(sq120, move, EMPTY, EMPTY, 0), list);
	}

	// Bishops
	while (bishops) {

		sq = PopLsb(&bishops);
		sq120 = SQ120(sq);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mBishopTable) & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(sq120, move, EMPTY, EMPTY, 0), list);
		}
	}

	// Rooks
	while (rooks) {

		sq = PopLsb(&rooks);
		sq120 = SQ120(sq);

		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mRookTable) & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(sq120, move, EMPTY, EMPTY, 0), list);
		}
	}

	// Queens
	while (queens) {

		sq = PopLsb(&queens);
		sq120 = SQ120(sq);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mBishopTable) & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(sq120, move, EMPTY, EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mRookTable) & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(sq120, move, EMPTY, EMPTY, 0), list);
		}
	}

	assert(MoveListOk(list, pos));
}


void GenerateAllCaptures(const S_BOARD *pos, S_MOVELIST *list) {

	assert(CheckBoard(pos));

	list->count = 0;

	int sq, sq120, attack;
	int side = pos->side;

	bitboard attacks, enPassant;

	bitboard allPieces  = pos->allBB;
	bitboard enemies 	= pos->colors[!side];

	bitboard pawns 		= pos->colors[side] & pos->pieceBBs[  PAWN];
	bitboard knights 	= pos->colors[side] & pos->pieceBBs[KNIGHT];
	bitboard bishops 	= pos->colors[side] & pos->pieceBBs[BISHOP];
	bitboard rooks 		= pos->colors[side] & pos->pieceBBs[  ROOK];
	bitboard queens 	= pos->colors[side] & pos->pieceBBs[ QUEEN];
	bitboard king 		= pos->colors[side] & pos->pieceBBs[  KING];


	// Pawns
	if (side == WHITE) {

		while (pawns) {

			sq = PopLsb(&pawns);
			sq120 = SQ120(sq);
			assert(SqOnBoard(sq120));

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(pos, MOVE(sq120, SQ120(pos->enPas), EMPTY, EMPTY, MOVE_FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = SQ120(PopLsb(&attacks));
				AddWhitePawnCapMove(pos, sq120, attack, pos->pieces[attack], list);
			}
		}

	} else {

		while (pawns) {

			sq = PopLsb(&pawns);
			sq120 = SQ120(sq);
			assert(SqOnBoard(sq120));

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(pos, MOVE(sq120, SQ120(pos->enPas), EMPTY, EMPTY, MOVE_FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = SQ120(PopLsb(&attacks));
				AddBlackPawnCapMove(pos, sq120, attack, pos->pieces[attack], list);
			}
		}
	}

	// Knights
	while (knights) {

		sq = PopLsb(&knights);
		sq120 = SQ120(sq);

		attacks = knight_attacks[sq] & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// King
	sq = Lsb(&king);
	sq120 = SQ120(sq);

	attacks = king_attacks[sq] & enemies;
	while (attacks) {
		attack = SQ120(PopLsb(&attacks));
		AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
	}

	// Bishops
	while (bishops) {

		sq = PopLsb(&bishops);
		sq120 = SQ120(sq);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// Rooks
	while (rooks) {

		sq = PopLsb(&rooks);
		sq120 = SQ120(sq);

		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// Queens
	while (queens) {

		sq = PopLsb(&queens);
		sq120 = SQ120(sq);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = SQ120(PopLsb(&attacks));
			AddCaptureMove(pos, MOVE(sq120, attack, pos->pieces[attack], EMPTY, 0), list);
		}
	}

	assert(MoveListOk(list, pos));
}
