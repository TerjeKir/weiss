// evaluate.c

#include <stdio.h>
#include <stdlib.h>

#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "validate.h"


#define ENDGAME_MAT (1 * PieceValues[wR] + 2 * PieceValues[wN] + 2 * PieceValues[wP] + PieceValues[wK])
#define MIRROR64(sq) (Mirror64[(sq)])


bitboard BlackPassedMask[64];
bitboard WhitePassedMask[64];
bitboard IsolatedMask[64];

const int PawnPassed[8] = {0, 5, 10, 20, 35, 60, 100, 0};
const int PawnIsolated = -10;
const int RookOpenFile = 10;
const int QueenOpenFile = 5;
const int RookSemiOpenFile = 5;
const int QueenSemiOpenFile = 3;
const int BishopPair = 30;

const int PawnTable[64] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	10, 10, 0, -10, -10, 0, 10, 10,
	5, 0, 0, 5, 5, 0, 0, 5,
	0, 0, 10, 20, 20, 10, 0, 0,
	5, 5, 5, 10, 10, 5, 5, 5,
	10, 10, 10, 20, 20, 10, 10, 10,
	20, 20, 20, 30, 30, 20, 20, 20,
	0, 0, 0, 0, 0, 0, 0, 0};

const int KnightTable[64] = {
	0, -10, 0,  0,  0,  0, -10, 0,
	0,   0, 0,  5,  5,  0,   0, 0,
	0,   0, 10, 10, 10, 10,  0, 0,
	0,   0, 10, 20, 20, 10,  5, 0,
	5,  10, 15, 20, 20, 15, 10, 5,
	5,  10, 10, 20, 20, 10, 10, 5,
	0,   0, 5,  10, 10, 5,   0, 0,
	0,   0, 0,  0,  0,  0,   0, 0};

const int BishopTable[64] = {
	0, 0, -10, 0, 0, -10, 0, 0,
	0, 0, 0, 10, 10, 0, 0, 0,
	0, 0, 10, 15, 15, 10, 0, 0,
	0, 10, 15, 20, 20, 15, 10, 0,
	0, 10, 15, 20, 20, 15, 10, 0,
	0, 0, 10, 15, 15, 10, 0, 0,
	0, 0, 0, 10, 10, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0};

const int RookTable[64] = {
	0, 0, 5, 10, 10, 5, 0, 0,
	0, 0, 5, 10, 10, 5, 0, 0,
	0, 0, 5, 10, 10, 5, 0, 0,
	0, 0, 5, 10, 10, 5, 0, 0,
	0, 0, 5, 10, 10, 5, 0, 0,
	0, 0, 5, 10, 10, 5, 0, 0,
	25, 25, 25, 25, 25, 25, 25, 25,
	0, 0, 5, 10, 10, 5, 0, 0};

const int KingO[64] = {
	  0,   5,   5, -10, -10,   0,  10,   5,
	-30, -30, -30, -30, -30, -30, -30, -30,
	-50, -50, -50, -50, -50, -50, -50, -50,
	-70, -70, -70, -70, -70, -70, -70, -70,
	-70, -70, -70, -70, -70, -70, -70, -70,
	-70, -70, -70, -70, -70, -70, -70, -70,
	-70, -70, -70, -70, -70, -70, -70, -70,
	-70, -70, -70, -70, -70, -70, -70, -70};

const int KingE[64] = {
	-50, -10,  0,  0,  0,  0, -10, -50,
	-10,   0, 10, 10, 10, 10,   0, -10,
	  0,  10, 20, 20, 20, 20,  10,   0,
	  0,  10, 20, 40, 40, 20,  10,   0,
	  0,  10, 20, 40, 40, 20,  10,   0,
	  0,  10, 20, 20, 20, 20,  10,   0,
	-10,   0, 10, 10, 10, 10,   0, -10,
	-50, -10, 0,   0,  0, 0,  -10, -50};


void InitEvalMasks() {

	int sq, tsq;

	for (sq = 0; sq < 64; ++sq) {
		IsolatedMask[sq] = 0ULL;
		WhitePassedMask[sq] = 0ULL;
		BlackPassedMask[sq] = 0ULL;
	}

	for (sq = 0; sq < 64; ++sq) {
		tsq = sq + 8;

		while (tsq < 64) {
			WhitePassedMask[sq] |= (1ULL << tsq);
			tsq += 8;
		}

		tsq = sq - 8;
		while (tsq >= 0) {
			BlackPassedMask[sq] |= (1ULL << tsq);
			tsq -= 8;
		}

		if (fileOf(sq) > FILE_A) {
			IsolatedMask[sq] |= fileBBs[fileOf(sq) - 1];

			tsq = sq + 7;
			while (tsq < 64) {
				WhitePassedMask[sq] |= (1ULL << tsq);
				tsq += 8;
			}

			tsq = sq - 9;
			while (tsq >= 0) {
				BlackPassedMask[sq] |= (1ULL << tsq);
				tsq -= 8;
			}
		}

		if (fileOf(sq) < FILE_H) {
			IsolatedMask[sq] |= fileBBs[fileOf(sq) + 1];

			tsq = sq + 9;
			while (tsq < 64) {
				WhitePassedMask[sq] |= (1ULL << tsq);
				tsq += 8;
			}

			tsq = sq - 7;
			while (tsq >= 0) {
				BlackPassedMask[sq] |= (1ULL << tsq);
				tsq -= 8;
			}
		}
	}
}

#ifdef CHECK_MAT_DRAW
static int MaterialDraw(const S_BOARD *pos) {

	assert(CheckBoard(pos));

	// No draw with queens or pawns
	if (pos->pieceCounts[wQ] || pos->pieceCounts[bQ] || pos->pieceCounts[wP] || pos->pieceCounts[bP])
		return false;
	
	// No rooks
	if (!pos->pieceCounts[wR] && !pos->pieceCounts[bR]) {
		// No bishops
		if (!pos->pieceCounts[bB] && !pos->pieceCounts[wB]) {
			// Draw with 1-2 knights (or 0 => KvK)
			if (pos->pieceCounts[wN] < 3 && pos->pieceCounts[bN] < 3)
				return true;
		// No knights
		} else if (!pos->pieceCounts[wN] && !pos->pieceCounts[bN]) {
			// Draw unless one side has 2 extra bishops
			if (abs(pos->pieceCounts[wB] - pos->pieceCounts[bB]) < 2)
				return true;
		// Draw with 1-2 knights vs 1 bishop
		} else if ((pos->pieceCounts[wN] < 3 && !pos->pieceCounts[wB]) || (pos->pieceCounts[wB] == 1 && !pos->pieceCounts[wN]))
			if ((pos->pieceCounts[bN] < 3 && !pos->pieceCounts[bB]) || (pos->pieceCounts[bB] == 1 && !pos->pieceCounts[bN]))
				return true;
	// Draw with 1 rook each + up to 1 N or B each
	} else if (pos->pieceCounts[wR] == 1 && pos->pieceCounts[bR] == 1) {
		if ((pos->pieceCounts[wN] + pos->pieceCounts[wB]) < 2 && (pos->pieceCounts[bN] + pos->pieceCounts[bB]) < 2)
			return true;
	// Draw with white having 1 rook vs 1-2 N/B
	} else if (pos->pieceCounts[wR] == 1 && !pos->pieceCounts[bR]) {
		if ((pos->pieceCounts[wN] + pos->pieceCounts[wB] == 0) && (((pos->pieceCounts[bN] + pos->pieceCounts[bB]) == 1) || ((pos->pieceCounts[bN] + pos->pieceCounts[bB]) == 2)))
			return true;
	// Draw with black having 1 rook vs 1-2 N/B
	} else if (pos->pieceCounts[bR] == 1 && !pos->pieceCounts[wR])
		if ((pos->pieceCounts[bN] + pos->pieceCounts[bB] == 0) && (((pos->pieceCounts[wN] + pos->pieceCounts[wB]) == 1) || ((pos->pieceCounts[wN] + pos->pieceCounts[wB]) == 2)))
			return true;

	return false;
}
#endif

int EvalPosition(const S_BOARD *pos) {

	assert(CheckBoard(pos));

	int sq;
	int score = pos->material[WHITE] - pos->material[BLACK];

	bitboard whitePawns 	= pos->colors[WHITE] & pos->pieceBBs[  PAWN];
	bitboard whiteKnights 	= pos->colors[WHITE] & pos->pieceBBs[KNIGHT];
	bitboard whiteBishops 	= pos->colors[WHITE] & pos->pieceBBs[BISHOP];
	bitboard whiteRooks 	= pos->colors[WHITE] & pos->pieceBBs[  ROOK];
	bitboard whiteQueens 	= pos->colors[WHITE] & pos->pieceBBs[ QUEEN];
	bitboard blackPawns 	= pos->colors[BLACK] & pos->pieceBBs[  PAWN];
	bitboard blackKnights 	= pos->colors[BLACK] & pos->pieceBBs[KNIGHT];
	bitboard blackBishops 	= pos->colors[BLACK] & pos->pieceBBs[BISHOP];
	bitboard blackRooks 	= pos->colors[BLACK] & pos->pieceBBs[  ROOK];
	bitboard blackQueens 	= pos->colors[BLACK] & pos->pieceBBs[ QUEEN];

#ifdef CHECK_MAT_DRAW
	if (MaterialDraw(pos)) return 0;
#endif

	// Bishop pair
	if (PopCount(whiteBishops) >= 2)
		score += BishopPair;
	if (PopCount(blackBishops) >= 2)
		score -= BishopPair;

	// White pawns
	while (whitePawns) {

		sq = PopLsb(&whitePawns);

		// Position score
		score += PawnTable[sq];
		// Isolation penalty
		if ((IsolatedMask[sq] & pos->colors[WHITE] & pos->pieceBBs[PAWN]) == 0)
			score += PawnIsolated;
		// Passed bonus
		if ((WhitePassedMask[sq] & blackPawns) == 0)
			score += PawnPassed[rankOf(sq)];
	}

	// Black pawns
	while (blackPawns) {

		sq = PopLsb(&blackPawns);

		score -= PawnTable[MIRROR64(sq)];

		if ((IsolatedMask[sq] & pos->colors[BLACK] & pos->pieceBBs[PAWN]) == 0)
			score -= PawnIsolated;

		if ((BlackPassedMask[sq] & pos->colors[WHITE] & pos->pieceBBs[PAWN]) == 0)
			score -= PawnPassed[7 - rankOf(sq)];
	}

	// White knights
	while (whiteKnights) {
		sq = PopLsb(&whiteKnights);
		score += KnightTable[sq];
	}

	// Black knights
	while (blackKnights) {
		sq = MIRROR64(PopLsb(&blackKnights));
		score -= KnightTable[sq];
	}

	// White bishops
	while (whiteBishops) {
		sq = PopLsb(&whiteBishops);
		score += BishopTable[sq];
	}

	// Black bishops
	while (blackBishops) {
		sq = MIRROR64(PopLsb(&blackBishops));
		score -= BishopTable[sq];
	}

	// White rooks
	while (whiteRooks) {

		sq = PopLsb(&whiteRooks);

		score += RookTable[sq];

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score += RookOpenFile;
		else if (!(pos->colors[WHITE] & pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score += RookSemiOpenFile;
	}

	// Black rooks
	while (blackRooks) {

		sq = PopLsb(&blackRooks);

		score -= RookTable[MIRROR64(sq)];

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score -= RookOpenFile;
		else if (!(pos->colors[BLACK] & pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score -= RookSemiOpenFile;
	}

	// White queens
	while (whiteQueens) {

		sq = PopLsb(&whiteQueens);

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score += QueenOpenFile;
		else if (!(pos->colors[WHITE] & pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score += QueenSemiOpenFile;
	}

	// Black queens
	while (blackQueens) {

		sq = PopLsb(&blackQueens);

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score -= QueenOpenFile;
		else if (!(pos->colors[BLACK] & pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score -= QueenSemiOpenFile;
	}

	// White king
	sq = pos->KingSq[WHITE];
	score += (pos->material[BLACK] <= ENDGAME_MAT) ? KingE[sq] : KingO[sq];

	// Black king
	sq = pos->KingSq[BLACK];
	score -= (pos->material[WHITE] <= ENDGAME_MAT) ? KingE[MIRROR64(sq)] : KingO[MIRROR64(sq)];

	assert(score > -INFINITE && score < INFINITE);

	// Return score
	return pos->side ? score : -score;
}