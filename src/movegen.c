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


const bitboard bitB1C1D1 = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
const bitboard bitF1G1 = (1ULL << 5) | (1ULL << 6);
const bitboard bitB8C8D8 = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);
const bitboard bitF8G8 = (1ULL << 61) | (1ULL << 62);

const int VictimScore[13] = {0, 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600};
static int MvvLvaScores[13][13];

/*
PV Move
Cap -> MvvLVA
Killers
HistoryScore
*/

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

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

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
		list->moves[list->count].score = pos->searchHistory[pos->pieces[SQ64(FROMSQ(move))]][TOSQ(move)];

	list->count++;
}

static void AddCaptureMove(const S_BOARD *pos, int move, S_MOVELIST *list) {

	assert(SqOnBoard(FROMSQ(move)));
	assert(SqOnBoard(TOSQ(move)));
	assert(PieceValid(CAPTURED(move)));
	assert(CheckBoard(pos));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[CAPTURED(move)][pos->pieces[SQ64(FROMSQ(move))]] + 1000000;
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

	int sq, attack, move;
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
				if (!SqAttacked(4, BLACK, pos) && !SqAttacked(5, BLACK, pos)) // TODO: Add back enums
					AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MOVE_FLAG_CASTLE), list);

		// Queen side castle
		if (pos->castlePerm & WQCA)
			if (!(allPieces & bitB1C1D1))
				if (!SqAttacked(4, BLACK, pos) && !SqAttacked(3, BLACK, pos)) // TODO: Add back enums
					AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MOVE_FLAG_CASTLE), list);

		// Pawns
		while (pawns) {

			sq = PopLsb(&pawns);
			squareBitMask = 1ULL << sq;
			assert(SqOnBoard(SQ120(sq)));

			// Move forward
			if (!(allPieces & squareBitMask << 8)) {
				AddWhitePawnMove(pos, SQ120(sq), SQ120(sq) + 10, list);
				// Move forward two squares
				if (!(allPieces & squareBitMask << 16) && (sq < 16))
					AddQuietMove(pos, MOVE(SQ120(sq), (SQ120(sq) + 20), EMPTY, EMPTY, MOVE_FLAG_PAWNSTART), list);
			}

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(pos, MOVE(SQ120(sq), SQ120(pos->enPas), EMPTY, EMPTY, MOVE_FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = PopLsb(&attacks);
				AddWhitePawnCapMove(pos, SQ120(sq), SQ120(attack), pos->pieces[attack], list);
			}
		}

	} else {

		// King side castle
		if (pos->castlePerm & BKCA)
			if (!((allPieces & bitF8G8)))
				if (!SqAttacked(60, WHITE, pos) && !SqAttacked(61, WHITE, pos)) // TODO: Add back enums
					AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MOVE_FLAG_CASTLE), list);

		// Queen side castle
		if (pos->castlePerm & BQCA)
			if (!((allPieces & bitB8C8D8)))
				if (!SqAttacked(60, WHITE, pos) && !SqAttacked(59, WHITE, pos)) // TODO: Add back enums
					AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MOVE_FLAG_CASTLE), list);

		// Pawns
		while (pawns) {

			sq = PopLsb(&pawns);
			squareBitMask = 1ULL << sq;
			assert(SqOnBoard(SQ120(sq)));

			// Move forward
			if (!(allPieces & squareBitMask >> 8)) {
				AddBlackPawnMove(pos, SQ120(sq), SQ120(sq) - 10, list);
				// Move forward two squares
				if (!(allPieces & squareBitMask >> 16) && (sq > 47))
					AddQuietMove(pos, MOVE(SQ120(sq), (SQ120(sq) - 20), EMPTY, EMPTY, MOVE_FLAG_PAWNSTART), list);
			}

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(pos, MOVE(SQ120(sq), SQ120(pos->enPas), EMPTY, EMPTY, MOVE_FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = PopLsb(&attacks);
				AddBlackPawnCapMove(pos, SQ120(sq), SQ120(attack), pos->pieces[attack], list);
			}
		}
	}

	// Knights
	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
		moves = knight_attacks[sq] & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(SQ120(sq), move, EMPTY, EMPTY, 0), list);
		}
	}

	// King
	sq = Lsb(king);

	attacks = king_attacks[sq] & enemies;
	while (attacks) {
		attack = PopLsb(&attacks);
		AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
	}
	moves = king_attacks[sq] & empty;
	while (moves) {
		move = SQ120(PopLsb(&moves));
		AddQuietMove(pos, MOVE(SQ120(sq), move, EMPTY, EMPTY, 0), list);
	}

	// Bishops
	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mBishopTable) & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(SQ120(sq), move, EMPTY, EMPTY, 0), list);
		}
	}

	// Rooks
	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mRookTable) & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(SQ120(sq), move, EMPTY, EMPTY, 0), list);
		}
	}

	// Queens
	while (queens) {

		sq = PopLsb(&queens);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mBishopTable) & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(SQ120(sq), move, EMPTY, EMPTY, 0), list);
		}
		moves = SliderAttacks(sq, allPieces, mRookTable) & empty;
		while (moves) {
			move = SQ120(PopLsb(&moves));
			AddQuietMove(pos, MOVE(SQ120(sq), move, EMPTY, EMPTY, 0), list);
		}
	}

	assert(MoveListOk(list, pos));
}


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
	bitboard king 		= pos->colors[side] & pos->pieceBBs[  KING];


	// Pawns
	if (side == WHITE) {

		while (pawns) {

			sq = PopLsb(&pawns);
			assert(SqOnBoard(SQ120(sq)));

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(pos, MOVE(SQ120(sq), SQ120(pos->enPas), EMPTY, EMPTY, MOVE_FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = PopLsb(&attacks);
				AddWhitePawnCapMove(pos, SQ120(sq), SQ120(attack), pos->pieces[attack], list);
			}
		}

	} else {

		while (pawns) {

			sq = PopLsb(&pawns);
			assert(SqOnBoard(SQ120(sq)));

			// Pawn captures
			attacks = pawn_attacks[side][sq] & enemies;

			// En passant
			if (pos->enPas != NO_SQ) {
				assert(pos->enPas >= 0 && pos->enPas < 64);
				enPassant = 1ULL << pos->enPas;
				if (pawn_attacks[side][sq] & enPassant)
					AddEnPassantMove(pos, MOVE(SQ120(sq), SQ120(pos->enPas), EMPTY, EMPTY, MOVE_FLAG_ENPAS), list);
			}
			// Normal captures
			while (attacks) {
				attack = PopLsb(&attacks);
				AddBlackPawnCapMove(pos, SQ120(sq), SQ120(attack), pos->pieces[attack], list);
			}
		}
	}

	// Knights
	while (knights) {

		sq = PopLsb(&knights);

		attacks = knight_attacks[sq] & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// King
	sq = Lsb(king);

	attacks = king_attacks[sq] & enemies;
	while (attacks) {
		attack = PopLsb(&attacks);
		AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
	}

	// Bishops
	while (bishops) {

		sq = PopLsb(&bishops);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// Rooks
	while (rooks) {

		sq = PopLsb(&rooks);

		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
	}

	// Queens
	while (queens) {

		sq = PopLsb(&queens);

		attacks = SliderAttacks(sq, allPieces, mBishopTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
		attacks = SliderAttacks(sq, allPieces, mRookTable) & enemies;
		while (attacks) {
			attack = PopLsb(&attacks);
			AddCaptureMove(pos, MOVE(SQ120(sq), SQ120(attack), pos->pieces[attack], EMPTY, 0), list);
		}
	}

	assert(MoveListOk(list, pos));
}
