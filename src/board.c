// board.c

#include <stdio.h>

#include "defs.h"
#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "hashkeys.h"
#include "validate.h"


// Update material lists to match pos->pieces
static void UpdateListsMaterial(S_BOARD *pos) {

	int piece, sq, color;

	for (sq = 0; sq < 64; sq++) {

		piece = pos->pieces[SQ120(sq)];
		assert(PceValidEmptyOffbrd(piece));

		if (piece != EMPTY) {

			color = PieceColor[piece];
			assert(SideValid(color));

			if (PieceBig[piece]) pos->bigPieces[color]++;
			pos->material[color] += PieceValues[piece];

			assert(pos->pieceCounts[piece] < 10 && pos->pieceCounts[piece] >= 0);

			pos->pieceList[piece][pos->pieceCounts[piece]] = SQ120(sq);
			pos->pieceCounts[piece]++;

			if (piece == wK) pos->KingSq[WHITE] = sq;
			if (piece == bK) pos->KingSq[BLACK] = sq;
		}
	}
}

// Update bitboards to match pos->pieces
static void UpdateBitboards(S_BOARD *pos) {

	int piece, sq;

	for (sq = 0; sq < 64; sq++) {

		piece = pos->pieces[SQ120(sq)];
		assert(PceValidEmptyOffbrd(piece));

		if (piece != OFFBOARD && piece != EMPTY) {

			SETBIT(pos->allBB,  sq);

			// Pawns
			if (piece == wP) {
				SETBIT(pos->colors[WHITE], sq);
				SETBIT(pos->pieceBBs[PAWN],  sq);
			} else if (piece == bP) {
				SETBIT(pos->colors[BLACK], sq);
				SETBIT(pos->pieceBBs[PAWN],  sq);
			// Knights
			} else if (piece == wN) {
				SETBIT(pos->colors[WHITE],  sq);
				SETBIT(pos->pieceBBs[KNIGHT], sq);
			} else if (piece == bN) {
				SETBIT(pos->colors[BLACK],  sq);
				SETBIT(pos->pieceBBs[KNIGHT], sq);
			// Bishops
			} else if (piece == wB) {
				SETBIT(pos->colors[WHITE],  sq);
				SETBIT(pos->pieceBBs[BISHOP], sq);
			} else if (piece == bB) {
				SETBIT(pos->colors[BLACK],  sq);
				SETBIT(pos->pieceBBs[BISHOP], sq);
			// Rooks
			} else if (piece == wR) {
				SETBIT(pos->colors[WHITE], sq);
				SETBIT(pos->pieceBBs[ROOK],  sq);
			} else if (piece == bR) {
				SETBIT(pos->colors[BLACK], sq);
				SETBIT(pos->pieceBBs[ROOK],  sq);
			// Queens
			} else if (piece == wQ) {
				SETBIT(pos->colors[WHITE], sq);
				SETBIT(pos->pieceBBs[QUEEN], sq);
			} else if (piece == bQ) {
				SETBIT(pos->colors[BLACK], sq);
				SETBIT(pos->pieceBBs[QUEEN], sq);
			// Kings
			} else if (piece == wK) {
				SETBIT(pos->colors[WHITE], sq);
				SETBIT(pos->pieceBBs[KING],  sq);
			} else if (piece == bK) {
				SETBIT(pos->colors[BLACK], sq);
				SETBIT(pos->pieceBBs[KING],  sq);
			}
		}
	}
}

// Resets the board
static void ResetBoard(S_BOARD *pos) {

	int index = 0;

	// All squares to offboard
	for (index = 0; index < BRD_SQ_NUM; ++index)
		pos->pieces[index] = OFFBOARD;

	// All squares on the board to empty instead of offboard
	for (index = 0; index < 64; ++index)
		pos->pieces[SQ120(index)] = EMPTY;

	// Piece counts and material
	for (index = 0; index < 2; ++index) {
		pos->bigPieces[index] = 0;
		pos->material[index] = 0;
	}
	for (index = 0; index < 13; ++index)
		pos->pieceCounts[index] = 0;

	// Bitboards
	for (index = 0; index < 2; index++)
		pos->colors[index] = 0ULL;
	
	for (index = PAWN; index <= KING; index++)
		pos->pieceBBs[index] = 0ULL;

	pos->allBB = 0ULL;

	// King squares
	pos->KingSq[WHITE] = pos->KingSq[BLACK] = NO_SQ;

	// Misc
	pos->side = BOTH;
	pos->enPas = NO_SQ;
	pos->fiftyMove = 0;
	pos->castlePerm = 0;

	// Ply
	pos->ply = 0;
	pos->hisPly = 0;

	// Position key
	pos->posKey = 0ULL;
}

#ifndef NDEBUG
// Check board state makes sense
int CheckBoard(const S_BOARD *pos) {

	int t_pieceCounts[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int t_bigPieces[2] = {0, 0};
	int t_material[2] = {0, 0};

	int sq, t_piece, t_pce_num, color;

	// Bitboards
	assert(PopCount(pos->pieceBBs[KING]) == 2);

	assert(PopCount(pos->pieceBBs[  PAWN] & pos->colors[WHITE]) <= 8);
	assert(PopCount(pos->pieceBBs[KNIGHT] & pos->colors[WHITE]) <= 10);
	assert(PopCount(pos->pieceBBs[BISHOP] & pos->colors[WHITE]) <= 10);
	assert(PopCount(pos->pieceBBs[  ROOK] & pos->colors[WHITE]) <= 10);
	assert(PopCount(pos->pieceBBs[ QUEEN] & pos->colors[WHITE]) <= 9);
	assert(PopCount(pos->pieceBBs[  KING] & pos->colors[WHITE]) == 1);

	assert(PopCount(pos->pieceBBs[  PAWN] & pos->colors[BLACK]) <= 8);
	assert(PopCount(pos->pieceBBs[KNIGHT] & pos->colors[BLACK]) <= 10);
	assert(PopCount(pos->pieceBBs[BISHOP] & pos->colors[BLACK]) <= 10);
	assert(PopCount(pos->pieceBBs[  ROOK] & pos->colors[BLACK]) <= 10);
	assert(PopCount(pos->pieceBBs[ QUEEN] & pos->colors[BLACK]) <= 9);
	assert(PopCount(pos->pieceBBs[  KING] & pos->colors[BLACK]) == 1);

	assert(pos->allBB == (pos->colors[WHITE] | pos->colors[BLACK]));

	// check piece lists
	for (t_piece = wP; t_piece <= bK; t_piece++)
		for (t_pce_num = 0; t_pce_num < pos->pieceCounts[t_piece]; ++t_pce_num) {
			sq = pos->pieceList[t_piece][t_pce_num];
			assert(pos->pieces[sq] == t_piece);
		}

	// check piece count and other counters
	for (sq = 0; sq < 64; sq++) {
		t_piece = pos->pieces[SQ120(sq)];
		t_pieceCounts[t_piece]++;
		color = PieceColor[t_piece];

		if (PieceBig[t_piece]) t_bigPieces[color]++;

		t_material[color] += PieceValues[t_piece];
	}

	for (t_piece = wP; t_piece <= bK; ++t_piece)
		assert(t_pieceCounts[t_piece] == pos->pieceCounts[t_piece]);

	assert(t_material[WHITE] == pos->material[WHITE] && t_material[BLACK] == pos->material[BLACK]);
	assert(t_bigPieces[WHITE] == pos->bigPieces[WHITE] && t_bigPieces[BLACK] == pos->bigPieces[BLACK]);

	assert(pos->side == WHITE || pos->side == BLACK);

	assert(pos->enPas == NO_SQ 
	   || (pos->enPas >= 40 && pos->enPas < 48 && pos->side == WHITE) 
	   || (pos->enPas >= 16 && pos->enPas < 24 && pos->side == BLACK));

	assert(pos->pieces[SQ120(pos->KingSq[WHITE])] == wK);
	assert(pos->pieces[SQ120(pos->KingSq[BLACK])] == bK);

	assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	assert(GeneratePosKey(pos) == pos->posKey);

	return TRUE;
}
#endif

// Parse FEN and set up board in the position described
int ParseFen(char *fen, S_BOARD *pos) {

	assert(fen != NULL);
	assert(pos != NULL);

	int rank = RANK_8;
	int file = FILE_A;
	int piece, count, i, sq;

	ResetBoard(pos);

	// Piece locations
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
			sq = rank * 8 + file;
			if (piece != EMPTY)
				pos->pieces[SQ120(sq)] = piece;

			file++;
		}
		fen++;
	}

	// Side to move
	assert(*fen == 'w' || *fen == 'b');

	pos->side = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	// Castling rights
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

	// En passant square
	if (*fen != '-') {
		file = fen[0] - 'a';
		rank = fen[1] - '1';

		assert(file >= FILE_A && file <= FILE_H);
		assert(rank == RANK_3 || rank == RANK_6);

		pos->enPas = (8 * rank) + file;
	}

	// Position Key
	pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);
	UpdateBitboards(pos);

	return 0;
}

// Print the board with misc info
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

// Reverse the colors
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
		tempEnPas = Mirror64[pos->enPas];

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
