// hashkeys.c

#include "defs.h"
#include "validate.h"


#define RAND_64 ((uint64_t)rand() |	   \
				 (uint64_t)rand() << 15 | \
				 (uint64_t)rand() << 30 | \
				 (uint64_t)rand() << 45 | \
				 ((uint64_t)rand() & 0xf) << 60)


// Zobrist key tables
uint64_t PieceKeys[13][120]; // 0 En passant, 1-12 White pawn - Black king
uint64_t SideKey;
uint64_t CastleKeys[16];


// Inits zobrist key tables
void InitHashKeys() {

	SideKey = RAND_64;

	for (int piece = 0; piece < 13; ++piece)
		for (int sq = 0; sq < 120; ++sq)
			PieceKeys[piece][sq] = RAND_64;

	for (int index = 0; index < 16; ++index)
		CastleKeys[index] = RAND_64;
}

uint64_t GeneratePosKey(const S_BOARD *pos) {

	uint64_t posKey = 0;
	int piece;

	// Pieces
	for (int sq = 0; sq < 64; ++sq) {
		
		piece = pos->pieces[sq];
		
		if (piece != NO_SQ && piece != EMPTY && piece != OFFBOARD) {
			assert(piece >= wP && piece <= bK);
			posKey ^= PieceKeys[piece][SQ120(sq)];
		}
	}

	// Side to play
	if (pos->side == WHITE)
		posKey ^= SideKey;

	// En passant
	if (pos->enPas != NO_SQ) {
		assert((pos->enPas >= 16 && pos->enPas < 24 && pos->side == BLACK) 
			|| (pos->enPas >= 40 && pos->enPas < 48 && pos->side == WHITE));
		posKey ^= PieceKeys[EMPTY][pos->enPas];
	}

	// Castling rights
	assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);
	posKey ^= CastleKeys[pos->castlePerm];

	return posKey;
}
