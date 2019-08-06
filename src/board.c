// board.c

#include <stdio.h>

#include "defs.h"
#include "bitboards.h"
#include "board.h"
#include "hashkeys.h"
#include "validate.h"

// Update material lists to match pos->pieces[]
static void UpdateListsMaterial(S_BOARD *pos) {

	int piece, sq, index, colour;

	for (index = 0; index < BRD_SQ_NUM; ++index) {
		sq = index;
		piece = pos->pieces[index];
		assert(PceValidEmptyOffbrd(piece));
		if (piece != OFFBOARD && piece != EMPTY) {
			colour = PieceCol[piece];
			assert(SideValid(colour));

			if (PieceBig[piece]) pos->bigPce[colour]++;
			if (PieceMin[piece]) pos->minPce[colour]++;
			if (PieceMaj[piece]) pos->majPce[colour]++;

			pos->material[colour] += PieceVal[piece];

			assert(pos->pceNum[piece] < 10 && pos->pceNum[piece] >= 0);

			pos->pList[piece][pos->pceNum[piece]] = sq;
			pos->pceNum[piece]++;

			if (piece == wK) pos->KingSq[WHITE] = sq;
			if (piece == bK) pos->KingSq[BLACK] = sq;

			if (piece == wP) {
				SETBIT(pos->pawns[WHITE], SQ64(sq));
				SETBIT(pos->pawns[BOTH], SQ64(sq));
			} else if (piece == bP) {
				SETBIT(pos->pawns[BLACK], SQ64(sq));
				SETBIT(pos->pawns[BOTH], SQ64(sq));
			}
		}
	}
}

// Update bitboards
static void UpdateBitboards(S_BOARD *pos) {

	int piece, sq, index;

	for (index = 0; index < BRD_SQ_NUM; ++index) {

		sq = index;
		piece = pos->pieces[index];
		assert(PceValidEmptyOffbrd(piece));

		if (piece != OFFBOARD && piece != EMPTY) {

			// Pawns
			if (piece == wP) {
				SETBIT(pos->colors[WHITE], SQ64(sq));
				SETBIT(pos->pieceBBs[PAWN],  SQ64(sq));
			} else if (piece == bP) {
				SETBIT(pos->colors[BLACK], SQ64(sq));
				SETBIT(pos->pieceBBs[PAWN],  SQ64(sq));
			// Knights
			} else if (piece == wN) {
				SETBIT(pos->colors[WHITE],  SQ64(sq));
				SETBIT(pos->pieceBBs[KNIGHT], SQ64(sq));
			} else if (piece == bN) {
				SETBIT(pos->colors[BLACK],  SQ64(sq));
				SETBIT(pos->pieceBBs[KNIGHT], SQ64(sq));
			// Bishops
			} else if (piece == wB) {
				SETBIT(pos->colors[WHITE],  SQ64(sq));
				SETBIT(pos->pieceBBs[BISHOP], SQ64(sq));
			} else if (piece == bB) {
				SETBIT(pos->colors[BLACK],  SQ64(sq));
				SETBIT(pos->pieceBBs[BISHOP], SQ64(sq));
			// Rooks
			} else if (piece == wR) {
				SETBIT(pos->colors[WHITE], SQ64(sq));
				SETBIT(pos->pieceBBs[ROOK],  SQ64(sq));
			} else if (piece == bR) {
				SETBIT(pos->colors[BLACK], SQ64(sq));
				SETBIT(pos->pieceBBs[ROOK],  SQ64(sq));
			// Queens
			} else if (piece == wQ) {
				SETBIT(pos->colors[WHITE], SQ64(sq));
				SETBIT(pos->pieceBBs[QUEEN], SQ64(sq));
			} else if (piece == bQ) {
				SETBIT(pos->colors[BLACK], SQ64(sq));
				SETBIT(pos->pieceBBs[QUEEN], SQ64(sq));
			// Kings
			} else if (piece == wK) {
				SETBIT(pos->colors[WHITE], SQ64(sq));
				SETBIT(pos->pieceBBs[KING],  SQ64(sq));
			} else if (piece == bK) {
				SETBIT(pos->colors[BLACK], SQ64(sq));
				SETBIT(pos->pieceBBs[KING],  SQ64(sq));
			}
		}
	}
}

// Resets the board
static void ResetBoard(S_BOARD *pos) {

	int index = 0;

	for (index = 0; index < BRD_SQ_NUM; ++index)
		pos->pieces[index] = OFFBOARD;

	for (index = 0; index < 64; ++index)
		pos->pieces[SQ120(index)] = EMPTY;

	for (index = 0; index < 2; ++index) {
		pos->bigPce[index] = 0;
		pos->majPce[index] = 0;
		pos->minPce[index] = 0;
		pos->material[index] = 0;
	}

	for (index = 0; index < 3; ++index)
		pos->pawns[index] = 0ULL;

	for (index = 0; index < 13; ++index)
		pos->pceNum[index] = 0;

	// Bitboards
	for (index = 0; index < 2; index++)
		pos->colors[index] = 0ULL;
	
	for (index = 0; index < 6; index++)
		pos->pieceBBs[index] = 0ULL;

	pos->KingSq[WHITE] = pos->KingSq[BLACK] = NO_SQ;

	pos->side = BOTH;
	pos->enPas = NO_SQ;
	pos->fiftyMove = 0;

	pos->ply = 0;
	pos->hisPly = 0;

	pos->castlePerm = 0;

	pos->posKey = 0ULL;
}

// Check piece list is ok
int PceListOk(const S_BOARD *pos) {

	int pce, sq, num;

	for (pce = wP; pce <= bK; ++pce)
		if (pos->pceNum[pce] < 0 || pos->pceNum[pce] >= 10)
			return FALSE;

	if (pos->pceNum[wK] != 1 || pos->pceNum[bK] != 1)
		return FALSE;

	for (pce = wP; pce <= bK; ++pce) {
		for (num = 0; num < pos->pceNum[pce]; ++num) {
			sq = pos->pList[pce][num];
			if (!SqOnBoard(sq))
				return FALSE;
		}
	}
	return TRUE;
}

// Check board state makes sense
int CheckBoard(const S_BOARD *pos) {

	int t_pceNum[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int t_bigPce[2] = {0, 0};
	int t_majPce[2] = {0, 0};
	int t_minPce[2] = {0, 0};
	int t_material[2] = {0, 0};

	int sq64, t_piece, t_pce_num, sq120, colour;

	uint64_t t_pawns[3] = {0ULL, 0ULL, 0ULL};

	t_pawns[WHITE] = pos->pawns[WHITE];
	t_pawns[BLACK] = pos->pawns[BLACK];
	t_pawns[BOTH] = pos->pawns[BOTH];

	// check piece lists
	for (t_piece = wP; t_piece <= bK; ++t_piece)
		for (t_pce_num = 0; t_pce_num < pos->pceNum[t_piece]; ++t_pce_num) {
			sq120 = pos->pList[t_piece][t_pce_num];
			assert(pos->pieces[sq120] == t_piece);
		}

	// check piece count and other counters
	for (sq64 = 0; sq64 < 64; ++sq64) {
		sq120 = SQ120(sq64);
		t_piece = pos->pieces[sq120];
		t_pceNum[t_piece]++;
		colour = PieceCol[t_piece];

		if (PieceBig[t_piece]) t_bigPce[colour]++;
		if (PieceMin[t_piece]) t_minPce[colour]++;
		if (PieceMaj[t_piece]) t_majPce[colour]++;

		t_material[colour] += PieceVal[t_piece];
	}

	for (t_piece = wP; t_piece <= bK; ++t_piece)
		assert(t_pceNum[t_piece] == pos->pceNum[t_piece]);

	// check bitboard counts
#ifndef NDEBUG
	int pcount;
	pcount = CNT(t_pawns[WHITE]);
	assert(pcount == pos->pceNum[wP]);
	pcount = CNT(t_pawns[BLACK]);
	assert(pcount == pos->pceNum[bP]);
	pcount = CNT(t_pawns[BOTH]);
	assert(pcount == (pos->pceNum[bP] + pos->pceNum[wP]));
#endif

	// check bitboard squares
	while (t_pawns[WHITE]) {
		sq64 = POP(&t_pawns[WHITE]);
		assert(pos->pieces[SQ120(sq64)] == wP);
	}

	while (t_pawns[BLACK]) {
		sq64 = POP(&t_pawns[BLACK]);
		assert(pos->pieces[SQ120(sq64)] == bP);
	}

	while (t_pawns[BOTH]) {
		sq64 = POP(&t_pawns[BOTH]);
		assert((pos->pieces[SQ120(sq64)] == bP) || (pos->pieces[SQ120(sq64)] == wP));
	}

	assert(t_material[WHITE] == pos->material[WHITE] && t_material[BLACK] == pos->material[BLACK]);
	assert(t_minPce[WHITE] == pos->minPce[WHITE] && t_minPce[BLACK] == pos->minPce[BLACK]);
	assert(t_majPce[WHITE] == pos->majPce[WHITE] && t_majPce[BLACK] == pos->majPce[BLACK]);
	assert(t_bigPce[WHITE] == pos->bigPce[WHITE] && t_bigPce[BLACK] == pos->bigPce[BLACK]);

	assert(pos->side == WHITE || pos->side == BLACK);
	assert(GeneratePosKey(pos) == pos->posKey);

	assert(pos->enPas == NO_SQ || (RanksBrd[pos->enPas] == RANK_6 && pos->side == WHITE) || (RanksBrd[pos->enPas] == RANK_3 && pos->side == BLACK));

	assert(pos->pieces[pos->KingSq[WHITE]] == wK);
	assert(pos->pieces[pos->KingSq[BLACK]] == bK);

	assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	assert(PceListOk(pos));

	return TRUE;
}

int ParseFen(char *fen, S_BOARD *pos) {

	assert(fen != NULL);
	assert(pos != NULL);

	int rank = RANK_8;
	int file = FILE_A;
	int piece = 0;
	int count = 0;
	int i = 0;
	int sq64 = 0;
	int sq120 = 0;

	ResetBoard(pos);

	while ((rank >= RANK_1) && *fen) {
		count = 1;
		switch (*fen) {
			case 'p': piece = bP; break;
			case 'r': piece = bR; break;
			case 'n': piece = bN; break;
			case 'b': piece = bB; break;
			case 'k': piece = bK; break;
			case 'q': piece = bQ; break;
			case 'P': piece = wP; break;
			case 'R': piece = wR; break;
			case 'N': piece = wN; break;
			case 'B': piece = wB; break;
			case 'K': piece = wK; break;
			case 'Q': piece = wQ; break;

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
				piece = EMPTY;
				count = *fen - '0';
				break;

			case '/':
			case ' ':
				rank--;
				file = FILE_A;
				fen++;
				continue;

			default:
				printf("FEN error \n");
				return -1;
		}

		for (i = 0; i < count; i++) {
			sq64 = rank * 8 + file;
			sq120 = SQ120(sq64);
			if (piece != EMPTY)
				pos->pieces[sq120] = piece;

			file++;
		}
		fen++;
	}

	assert(*fen == 'w' || *fen == 'b');

	pos->side = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	for (i = 0; i < 4; i++) {

		if (*fen == ' ')
			break;

		switch (*fen) {
			case 'K': pos->castlePerm |= WKCA; break;
			case 'Q': pos->castlePerm |= WQCA; break;
			case 'k': pos->castlePerm |= BKCA; break;
			case 'q': pos->castlePerm |= BQCA; break;
			default: break;
		}
		fen++;
	}
	fen++;

	assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	if (*fen != '-') {
		file = fen[0] - 'a';
		rank = fen[1] - '1';

		assert(file >= FILE_A && file <= FILE_H);
		assert(rank >= RANK_1 && rank <= RANK_8);

		pos->enPas = FR2SQ(file, rank);
	}

	pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);
	UpdateBitboards(pos);

	return 0;
}

void PrintBoard(const S_BOARD *pos) {

	int sq, file, rank, piece;

	printf("\nGame Board:\n\n");

	for (rank = RANK_8; rank >= RANK_1; rank--) {
		printf("%d  ", rank + 1);
		for (file = FILE_A; file <= FILE_H; file++) {
			sq = FR2SQ(file, rank);
			piece = pos->pieces[sq];
			printf("%3c", PceChar[piece]);
		}
		printf("\n");
	}

	printf("\n   ");
	for (file = FILE_A; file <= FILE_H; file++)
		printf("%3c", 'a' + file);

	printf("\n");
	printf("side: %c\n", SideChar[pos->side]);
	printf("enPas: %d\n", pos->enPas);
	printf("castle: %c%c%c%c\n",
		   pos->castlePerm & WKCA ? 'K' : '-',
		   pos->castlePerm & WQCA ? 'Q' : '-',
		   pos->castlePerm & BKCA ? 'k' : '-',
		   pos->castlePerm & BQCA ? 'q' : '-');
	printf("PosKey: %I64d\n", pos->posKey);
}

void MirrorBoard(S_BOARD *pos) {

	int tempPiecesArray[64];
	int tempSide = pos->side ^ 1;
	int SwapPiece[13] = {EMPTY, bP, bN, bB, bR, bQ, bK, wP, wN, wB, wR, wQ, wK};
	int tempCastlePerm = 0;
	int tempEnPas = NO_SQ;

	int sq;
	int tp;

	if (pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
	if (pos->castlePerm & WQCA) tempCastlePerm |= BQCA;

	if (pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
	if (pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

	if (pos->enPas != NO_SQ)
		tempEnPas = SQ120(Mirror64[SQ64(pos->enPas)]);

	for (sq = 0; sq < 64; sq++)
		tempPiecesArray[sq] = pos->pieces[SQ120(Mirror64[sq])];

	ResetBoard(pos);

	for (sq = 0; sq < 64; sq++) {
		tp = SwapPiece[tempPiecesArray[sq]];
		pos->pieces[SQ120(sq)] = tp;
	}

	pos->side = tempSide;
	pos->castlePerm = tempCastlePerm;
	pos->enPas = tempEnPas;

	pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);
	UpdateBitboards(pos);

	assert(CheckBoard(pos));
}
