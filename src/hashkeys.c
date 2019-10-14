// hashkeys.c

#include <stdlib.h>

#include "types.h"


#define RAND_64 ((uint64_t)rand() |	   \
				 (uint64_t)rand() << 15 | \
				 (uint64_t)rand() << 30 | \
				 (uint64_t)rand() << 45 | \
				 ((uint64_t)rand() & 0xf) << 60)


// Zobrist key tables
uint64_t PieceKeys[PIECE_NB][64];
uint64_t CastleKeys[16];
uint64_t SideKey;


// Inits zobrist key tables
static void InitHashKeys() __attribute__((constructor));
static void InitHashKeys() {

	// Side to play
	SideKey = RAND_64;

	// En passant
	for (int sq = A1; sq <= H8; ++sq)
			PieceKeys[0][sq] = RAND_64;

	// White pieces
	for (int piece = wP; piece <= wK; ++piece)
		for (int sq = A1; sq <= H8; ++sq)
			PieceKeys[piece][sq] = RAND_64;

	// Black pieces
	for (int piece = bP; piece <= bK; ++piece)
		for (int sq = A1; sq <= H8; ++sq)
			PieceKeys[piece][sq] = RAND_64;

	// Castling rights
	for (int i = 0; i < 16; ++i)
		CastleKeys[i] = RAND_64;
}

uint64_t GeneratePosKey(const Position *pos) {

	uint64_t posKey = 0;

	// Pieces
	for (int sq = A1; sq <= H8; ++sq) {
		int piece = pos->board[sq];
		if (piece != EMPTY)
			posKey ^= PieceKeys[piece][sq];
	}

	// Side to play
	if (pos->side == WHITE)
		posKey ^= SideKey;

	// En passant
	if (pos->enPas != NO_SQ)
		posKey ^= PieceKeys[EMPTY][pos->enPas];

	// Castling rights
	posKey ^= CastleKeys[pos->castlePerm];

	return posKey;
}