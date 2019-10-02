// hashkeys.c

#include <stdlib.h>

#include "types.h"


#define RAND_64 ((uint64_t)rand() |	   \
				 (uint64_t)rand() << 15 | \
				 (uint64_t)rand() << 30 | \
				 (uint64_t)rand() << 45 | \
				 ((uint64_t)rand() & 0xf) << 60)


// Zobrist key tables
uint64_t PieceKeys[PIECE_NB][64]; // 0 En passant, 1-12 White pawn - Black king
uint64_t SideKey;
uint64_t CastleKeys[16];


// Inits zobrist key tables
void InitHashKeys() {

	SideKey = RAND_64;

	for (int piece = 0; piece < PIECE_NB; ++piece)
		for (int sq = A1; sq <= H8; ++sq)
			PieceKeys[piece][sq] = RAND_64;

	for (int i = 0; i < 16; ++i)
		CastleKeys[i] = RAND_64;
}

uint64_t GeneratePosKey(const S_BOARD *pos) {

	uint64_t posKey = 0;

	// Pieces
	for (int sq = A1; sq <= H8; ++sq) {
		int piece = pos->pieces[sq];
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