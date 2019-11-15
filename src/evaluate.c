// evaluate.c

#include <stdlib.h>
#include <string.h>

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "psqt.h"
#include "validate.h"


// Eval bit masks
static bitboard BlackPassedMask[64];
static bitboard WhitePassedMask[64];
static bitboard IsolatedMask[64];

// Various bonuses and maluses
static const int PawnPassed[8] = { 0, S(5, 5), S(10, 10), S(20, 20), S(35, 35), S(60, 60), S(100, 100), 0 };
static const int PawnIsolated = S(-10, -10);

static const int  RookOpenFile = S(10, 10);
static const int QueenOpenFile = S(5, 5);
static const int  RookSemiOpenFile = S(5, 5);
static const int QueenSemiOpenFile = S(3, 3);

static const int BishopPair = S(30, 30);

static const int KingLineVulnerability = S(-5, 0);


// Initialize evaluation bit masks
static void InitEvalMasks() __attribute__((constructor));
static void InitEvalMasks() {

	int sq, tsq;

	// For each square a pawn can be on
	for (sq = A2; sq <= H7; ++sq) {

		// In front
		for (tsq = sq + 8; tsq <= H8; tsq += 8)
			WhitePassedMask[sq] |= (1ULL << tsq);

		for (tsq = sq - 8; tsq >= A1; tsq -= 8)
			BlackPassedMask[sq] |= (1ULL << tsq);

		// Left side
		if (fileOf(sq) > FILE_A) {

			IsolatedMask[sq] |= fileBBs[fileOf(sq) - 1];

			for (tsq = sq + 7; tsq <= H8; tsq += 8)
				WhitePassedMask[sq] |= (1ULL << tsq);

			for (tsq = sq - 9; tsq >= A1; tsq -= 8)
				BlackPassedMask[sq] |= (1ULL << tsq);
		}

		// Right side
		if (fileOf(sq) < FILE_H) {

			IsolatedMask[sq] |= fileBBs[fileOf(sq) + 1];

			for (tsq = sq + 9; tsq <= H8; tsq += 8)
				WhitePassedMask[sq] |= (1ULL << tsq);

			for (tsq = sq - 7; tsq >= A1; tsq -= 8)
				BlackPassedMask[sq] |= (1ULL << tsq);
		}
	}
}

#ifdef CHECK_MAT_DRAW
// Check if the board is (likely) drawn, logic from sjeng
static bool MaterialDraw(const Position *pos) {

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

// Calculate a static evaluation of a position
int EvalPosition(const Position *pos) {

#ifdef CHECK_MAT_DRAW
	if (MaterialDraw(pos)) return 0;
#endif

	int sq, i;

	bitboard whitePawns = pos->colorBBs[WHITE] & pos->pieceBBs[PAWN];
	bitboard blackPawns = pos->colorBBs[BLACK] & pos->pieceBBs[PAWN];

	// Material
	int score = pos->material;

	// Bishop pair
	if (pos->pieceCounts[wB] >= 2)
		score += BishopPair;
	if (pos->pieceCounts[bB] >= 2)
		score -= BishopPair;

	// White pawns
	for (i = 0; i < pos->pieceCounts[wP]; ++i) {
		sq = pos->pieceList[wP][i];

		// Isolation penalty
		if (!(IsolatedMask[sq] & whitePawns))
			score += PawnIsolated;
		// Passed bonus
		if (!(WhitePassedMask[sq] & blackPawns))
			score += PawnPassed[rankOf(sq)];
	}

	// Black pawns
	for (i = 0; i < pos->pieceCounts[bP]; ++i) {
		sq = pos->pieceList[bP][i];

		// Isolation penalty
		if (!(IsolatedMask[sq] & blackPawns))
			score -= PawnIsolated;
		// Passed bonus
		if (!(BlackPassedMask[sq] & whitePawns))
			score -= PawnPassed[7 - rankOf(sq)];
	}

	// White knights
	// for (i = 0; i < pos->pieceCounts[wN]; ++i) {
	// 	sq = pos->pieceList[wN][i];
	// }

	// // Black knights
	// for (i = 0; i < pos->pieceCounts[bN]; ++i) {
	// 	sq = pos->pieceList[bN][i];
	// }

	// // White bishops
	// for (i = 0; i < pos->pieceCounts[wB]; ++i) {
	// 	sq = pos->pieceList[wB][i];
	// }

	// // Black bishops
	// for (i = 0; i < pos->pieceCounts[bB]; ++i) {
	// 	sq = pos->pieceList[bB][i];
	// }

	// White rooks
	for (i = 0; i < pos->pieceCounts[wR]; ++i) {
		sq = pos->pieceList[wR][i];

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score += RookOpenFile;
		else if (!(whitePawns & fileBBs[fileOf(sq)]))
			score += RookSemiOpenFile;
	}

	// Black rooks
	for (i = 0; i < pos->pieceCounts[bR]; ++i) {
		sq = pos->pieceList[bR][i];

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score -= RookOpenFile;
		else if (!(blackPawns & fileBBs[fileOf(sq)]))
			score -= RookSemiOpenFile;
	}

	// White queens
	for (i = 0; i < pos->pieceCounts[wQ]; ++i) {
		sq = pos->pieceList[wQ][i];

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score += QueenOpenFile;
		else if (!(whitePawns & fileBBs[fileOf(sq)]))
			score += QueenSemiOpenFile;
	}

	// Black queens
	for (i = 0; i < pos->pieceCounts[bQ]; ++i) {
		sq = pos->pieceList[bQ][i];

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score -= QueenOpenFile;
		else if (!(blackPawns & fileBBs[fileOf(sq)]))
			score -= QueenSemiOpenFile;
	}

	// Kings
	score += KingLineVulnerability * PopCount(  RookAttacks(pos->kingSq[WHITE], pos->colorBBs[WHITE] | pos->pieceBBs[PAWN])
											| BishopAttacks(pos->kingSq[WHITE], pos->colorBBs[WHITE] | pos->pieceBBs[PAWN]));
	score -= KingLineVulnerability * PopCount(  RookAttacks(pos->kingSq[BLACK], pos->colorBBs[BLACK] | pos->pieceBBs[PAWN])
											| BishopAttacks(pos->kingSq[BLACK], pos->colorBBs[BLACK] | pos->pieceBBs[PAWN]));

	// Adjust score by phase
	const int basePhase = 24;
	int phase = pos->phase;
	phase = (phase * 256 + (basePhase / 2)) / basePhase;

	score = ((MgScore(score) * (256 - phase)) + (EgScore(score) * phase)) / 256;

	assert(score > -INFINITE && score < INFINITE);

	// Return score
	return pos->side == WHITE ? score : -score;
}