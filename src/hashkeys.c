// hashkeys.c
#include "stdio.h"
#include "defs.h"

uint64_t GeneratePosKey(const S_BOARD *pos) {

	int sq = 0;
	uint64_t finalKey = 0;
	int piece = EMPTY;

	// pieces
	for (sq = 0; sq < BRD_SQ_NUM; ++sq) {
		piece = pos->pieces[sq];
		if (piece != NO_SQ && piece != EMPTY && piece != OFFBOARD) {
			ASSERT(piece >= wP && piece <= bK);
			finalKey ^= PieceKeys[piece][sq];
		}
	}

	if (pos->side == WHITE) {
		finalKey ^= SideKey;
	}

	if (pos->enPas != NO_SQ) {
		ASSERT(pos->enPas >= 0 && pos->enPas < BRD_SQ_NUM);
		ASSERT(SqOnBoard(pos->enPas));
		ASSERT(RanksBrd[pos->enPas] == RANK_3 || RanksBrd[pos->enPas] == RANK_6);
		finalKey ^= PieceKeys[EMPTY][pos->enPas];
	}

	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	finalKey ^= CastleKeys[pos->castlePerm];

	return finalKey;
}
