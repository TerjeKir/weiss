// hashkeys.c

#include <stdio.h>

#include "defs.h"
#include "bitboards.h"
#include "validate.h"


#define RAND_64 ((uint64_t)rand() |	   \
				 (uint64_t)rand() << 15 | \
				 (uint64_t)rand() << 30 | \
				 (uint64_t)rand() << 45 | \
				 ((uint64_t)rand() & 0xf) << 60)

// Zobrist key tables
uint64_t PieceKeys[13][120]; // 0 En passant 1-12 White pawn - Black king
uint64_t SideKey;
uint64_t CastleKeys[16];

// Inits zobrist key tables
void InitHashKeys() {

	int index, index2;

	SideKey = RAND_64;

	for (index = 0; index < 13; ++index)
		for (index2 = 0; index2 < 120; ++index2)
			PieceKeys[index][index2] = RAND_64;

	for (index = 0; index < 16; ++index)
		CastleKeys[index] = RAND_64;
}

uint64_t GeneratePosKey(const S_BOARD *pos) {

	uint64_t posKey = 0;
	bitboard allPieces = pos->colors[WHITE] | pos->colors[BLACK];
	int sq, piece;

	// Pieces
	while (allPieces) {

		sq = PopLsb(&allPieces);
		piece = pos->pieces[SQ120(sq)];

		assert(piece >= wP && piece <= bK);
		posKey ^= PieceKeys[piece][sq];
	}

	// Side to play
	if (pos->side == WHITE)
		posKey ^= SideKey;

	// En passant
	if (pos->enPas != NO_SQ) {
		assert(pos->enPas >= 0 && pos->enPas < BRD_SQ_NUM);
		assert(SqOnBoard(pos->enPas));
		assert(RanksBrd[pos->enPas] == RANK_3 || RanksBrd[pos->enPas] == RANK_6);
		posKey ^= PieceKeys[EMPTY][pos->enPas];
	}

	// Castling rights
	assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);
	posKey ^= CastleKeys[pos->castlePerm];

	return posKey;
}
