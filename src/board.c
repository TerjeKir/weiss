// board.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "hashkeys.h"
#include "validate.h"

// Initialize distance lookup table
void InitDistance() {

    int vertical, horizontal;

    for (int sq1 = 0; sq1 < 64; ++sq1)
        for (int sq2 = 0; sq2 < 64; ++sq2) {
            vertical   = abs(rankOf(sq1) - rankOf(sq2));
            horizontal = abs(fileOf(sq1) - fileOf(sq2));
            distance[sq1][sq2] = ((vertical > horizontal) ? vertical : horizontal);
        }
}

// Update material lists to match pos->pieces
static void UpdateListsMaterial(S_BOARD *pos) {

	// Loop through each square on the board
	for (int sq = 0; sq < 64; ++sq) {

		int piece = pos->pieces[sq];
		assert(PieceValidEmpty(piece));

		// If it isn't empty we update the relevant lists
		if (piece != EMPTY) {

			int color = PieceColor[piece];
			assert(SideValid(color));

			// Non pawn piece
			if (PieceBig[piece]) 
				pos->bigPieces[color]++;

			// Total material value for that side
			pos->material[color] += PieceValues[piece];

			assert(pos->pieceCounts[piece] < 10 && pos->pieceCounts[piece] >= 0);

			// Piece list / count
			pos->pieceList[piece][pos->pieceCounts[piece]] = sq;
			pos->pieceCounts[piece]++;

			// King square
			if      (piece == wK) pos->KingSq[WHITE] = sq;
			else if (piece == bK) pos->KingSq[BLACK] = sq;
		}
	}
}

// Update bitboards to match pos->pieces
static void UpdateBitboards(S_BOARD *pos) {

	// Loop through each square and update bitboards if there's a piece there
	for (int sq = 0; sq < 64; ++sq) {

		int piece = pos->pieces[sq];
		assert(PieceValidEmpty(piece));

		if (piece != EMPTY) {
			SETBIT(pos->allBB,  sq);
			SETBIT(pos->colors[PieceColor[piece]], sq);
			SETBIT(pos->pieceBBs[pieceType[piece]], sq);
		}
	}
}

// Resets the board
static void ResetBoard(S_BOARD *pos) {

	int index = 0;

	// Bitboard representations
	for (index = 0; index < 2; ++index)
		pos->colors[index] = 0ULL;

	for (index = PAWN; index <= KING; ++index)
		pos->pieceBBs[index] = 0ULL;

	pos->allBB = 0ULL;

	// Array representation
	for (index = 0; index < 64; ++index)
		pos->pieces[index] = EMPTY;

	// Piece lists and counts
	for (index = 0; index < 13; ++index) {
		pos->pieceCounts[index] = 0;
		for (int index2 = 0; index2 < 10; ++index2)
			pos->pieceList[index][index2] = 0;
	}

	// King squares
	pos->KingSq[WHITE] = pos->KingSq[BLACK] = NO_SQ;

	// Piece counts and material
	for (index = 0; index < 2; ++index) {
		pos->bigPieces[index] = 0;
		pos->material[index] = 0;
	}

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

	memset(pos->PvArray, 0, sizeof(pos->PvArray)); // TODO does this make sense???
}



// Parse FEN and set up board in the position described
int ParseFen(const char *fen, S_BOARD *pos) {

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

		for (i = 0; i < count; ++i) {
			sq = rank * 8 + file;
			if (piece != EMPTY)
				pos->pieces[sq] = piece;

			file++;
		}
		fen++;
	}

	// Side to move
	assert(*fen == 'w' || *fen == 'b');

	pos->side = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	// Castling rights
	for (i = 0; i < 4; ++i) {

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

	// Update lists and bitboards
	UpdateListsMaterial(pos);
	UpdateBitboards(pos);

	// Generate position Key
	pos->posKey = GeneratePosKey(pos);

	return 0;
}

// Print the board with misc info
void PrintBoard(const S_BOARD *pos) {

	int sq, file, rank, piece;

	printf("\nGame Board:\n\n");

	for (rank = RANK_8; rank >= RANK_1; --rank) {
		printf("%d  ", rank + 1);
		for (file = FILE_A; file <= FILE_H; ++file) {
			sq = (rank * 8) + file;
			piece = pos->pieces[sq];
			printf("%3c", PceChar[piece]);
		}
		printf("\n");
	}

	printf("\n   ");
	for (file = FILE_A; file <= FILE_H; ++file)
		printf("%3c", 'a' + file);

	printf("\n");
	printf("side: %c\n", SideChar[pos->side]);
	printf("enPas: %d\n", pos->enPas);
	printf("castle: %c%c%c%c\n",
		   pos->castlePerm & WKCA ? 'K' : '-',
		   pos->castlePerm & WQCA ? 'Q' : '-',
		   pos->castlePerm & BKCA ? 'k' : '-',
		   pos->castlePerm & BQCA ? 'q' : '-');
	printf("PosKey: %I64u\n", pos->posKey);
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

	assert(PopCount(pos->pieceBBs[  PAWN] & pos->colors[WHITE]) == pos->pieceCounts[wP]);
	assert(PopCount(pos->pieceBBs[KNIGHT] & pos->colors[WHITE]) == pos->pieceCounts[wN]);
	assert(PopCount(pos->pieceBBs[BISHOP] & pos->colors[WHITE]) == pos->pieceCounts[wB]);
	assert(PopCount(pos->pieceBBs[  ROOK] & pos->colors[WHITE]) == pos->pieceCounts[wR]);
	assert(PopCount(pos->pieceBBs[ QUEEN] & pos->colors[WHITE]) == pos->pieceCounts[wQ]);

	assert(PopCount(pos->pieceBBs[  PAWN] & pos->colors[BLACK]) == pos->pieceCounts[bP]);
	assert(PopCount(pos->pieceBBs[KNIGHT] & pos->colors[BLACK]) == pos->pieceCounts[bN]);
	assert(PopCount(pos->pieceBBs[BISHOP] & pos->colors[BLACK]) == pos->pieceCounts[bB]);
	assert(PopCount(pos->pieceBBs[  ROOK] & pos->colors[BLACK]) == pos->pieceCounts[bR]);
	assert(PopCount(pos->pieceBBs[ QUEEN] & pos->colors[BLACK]) == pos->pieceCounts[bQ]);

	assert(pos->allBB == (pos->colors[WHITE] | pos->colors[BLACK]));

	// check piece lists
	for (t_piece = wP; t_piece <= bK; ++t_piece)
		for (t_pce_num = 0; t_pce_num < pos->pieceCounts[t_piece]; ++t_pce_num) {
			sq = pos->pieceList[t_piece][t_pce_num];
			assert(pos->pieces[sq] == t_piece);
		}

	// check piece count and other counters
	for (sq = 0; sq < 64; ++sq) {

		t_piece = pos->pieces[sq];
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

	assert(pos->pieces[pos->KingSq[WHITE]] == wK);
	assert(pos->pieces[pos->KingSq[BLACK]] == bK);

	assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	assert(GeneratePosKey(pos) == pos->posKey);

	return true;
}
#endif

#ifdef CLI
// Reverse the colors
void MirrorBoard(S_BOARD *pos) {

	int SwapPiece[13] = {EMPTY, bP, bN, bB, bR, bQ, bK, wP, wN, wB, wR, wQ, wK};
	int tempPiecesArray[64];
	int tempEnPas, sq;

	// Save info about current position, but mirrored

	// Side to play
	int tempSide = pos->side ^ 1;

	// Castling rights
	int tempCastlePerm = 0;
	if (pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
	if (pos->castlePerm & WQCA) tempCastlePerm |= BQCA;
	if (pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
	if (pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

	// En passant
	if (pos->enPas != NO_SQ)
		tempEnPas = Mirror[pos->enPas];
	else
		tempEnPas = NO_SQ;

	// Array representation
	for (sq = 0; sq < 64; ++sq)
		tempPiecesArray[sq] = SwapPiece[pos->pieces[Mirror[sq]]];

	// Clear the board
	ResetBoard(pos);

	// Fill the board with the stored mirrored data
	for (sq = 0; sq < 64; ++sq) {
		pos->pieces[sq] = tempPiecesArray[sq];
	}
	pos->side = tempSide;
	pos->enPas = tempEnPas;
	pos->castlePerm = tempCastlePerm;

	// Update lists and bitboards based on array rep
	UpdateListsMaterial(pos);
	UpdateBitboards(pos);

	// Generate new position key
	pos->posKey = GeneratePosKey(pos);

	assert(CheckBoard(pos));
}
#endif