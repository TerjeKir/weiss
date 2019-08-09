// hashkeys.c

#include <stdio.h>

#include "defs.h"
#include "validate.h"

uint64_t GeneratePosKey(const S_BOARD *pos) {

	int sq = 0;
	uint64_t finalKey = 0;
	int piece = EMPTY;

	// Pieces
	for (sq = 0; sq < BRD_SQ_NUM; ++sq) {
		piece = pos->pieces[sq];
		if (piece != NO_SQ && piece != EMPTY && piece != OFFBOARD) {
			assert(piece >= wP && piece <= bK);
			finalKey ^= PieceKeys[piece][sq];
		}
	}

	// Side to play
	if (pos->side == WHITE)
		finalKey ^= SideKey;

	// En passant
	if (pos->enPas != NO_SQ) {
		assert(pos->enPas >= 0 && pos->enPas < BRD_SQ_NUM);
		assert(SqOnBoard(pos->enPas));
		assert(RanksBrd[pos->enPas] == RANK_3 || RanksBrd[pos->enPas] == RANK_6);
		finalKey ^= PieceKeys[EMPTY][pos->enPas];
	}

	// Castling rights
	assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);
	finalKey ^= CastleKeys[pos->castlePerm];

	return finalKey;
}
