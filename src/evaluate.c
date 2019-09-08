// evaluate.c

#include <stdio.h>
#include <stdlib.h>

#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "validate.h"


#define ENDGAME_MAT (1 * PieceValues[wR] + 2 * PieceValues[wN] + 2 * PieceValues[wP] + PieceValues[wK])


bitboard BlackPassedMask[64];
bitboard WhitePassedMask[64];
bitboard IsolatedMask[64];

const int PawnPassed[8] = { 0, 5, 10, 20, 35, 60, 100, 0 };
const int PawnIsolated = -10;

const int  RookOpenFile = 10;
const int QueenOpenFile = 5;
const int  RookSemiOpenFile = 5;
const int QueenSemiOpenFile = 3;

const int BishopPair = 30;

// Piece square tables
const int PawnTable[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	10,  10,   0, -10, -10,   0,  10,  10,
	 5,   0,   0,   5,   5,   0,   0,   5,
	 0,   0,  10,  20,  20,  10,   0,   0,
	 5,   5,   5,  10,  10,   5,   5,   5,
	10,  10,  10,  20,  20,  10,  10,  10,
	20,  20,  20,  30,  30,  20,  20,  20,
	 0,   0,   0,   0,   0,   0,   0,   0};

const int KnightTable[64] = {
	 0, -10,   0,   0,   0,   0, -10,   0,
	 0,   0,   0,   5,   5,   0,   0,   0,
	 0,   0,  10,  10,  10,  10,   0,   0,
	 0,   0,  10,  20,  20,  10,   5,   0,
	 5,  10,  15,  20,  20,  15,  10,   5,
	 5,  10,  10,  20,  20,  10,  10,   5,
	 0,   0,   5,  10,  10,   5,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0};

const int BishopTable[64] = {
	 0,   0, -10,   0,   0, -10,   0,   0,
	 0,   0,   0,  10,  10,   0,   0,   0,
	 0,   0,  10,  15,  15,  10,   0,   0,
	 0,  10,  15,  20,  20,  15,  10,   0,
	 0,  10,  15,  20,  20,  15,  10,   0,
	 0,   0,  10,  15,  15,  10,   0,   0,
	 0,   0,   0,  10,  10,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0};

const int RookTable[64] = {
	 0,   0,   5,  10,  10,   5,   0,   0,
	 0,   0,   5,  10,  10,   5,   0,   0,
	 0,   0,   5,  10,  10,   5,   0,   0,
	 0,   0,   5,  10,  10,   5,   0,   0,
	 0,   0,   5,  10,  10,   5,   0,   0,
	 0,   0,   5,  10,  10,   5,   0,   0,
	25,  25,  25,  25,  25,  25,  25,  25,
	 0,   0,   5,  10,  10,   5,   0,   0};

const int KingEarlygame[64] = {
     0,   5,   5, -10, -10,   0,  10,   5,
   -30, -30, -30, -30, -30, -30, -30, -30,
   -50, -50, -50, -50, -50, -50, -50, -50,
   -70, -70, -70, -70, -70, -70, -70, -70,
   -70, -70, -70, -70, -70, -70, -70, -70,
   -70, -70, -70, -70, -70, -70, -70, -70,
   -70, -70, -70, -70, -70, -70, -70, -70,
   -70, -70, -70, -70, -70, -70, -70, -70};

const int KingEndgame[64] = {
   -50, -10,   0,   0,   0,   0, -10, -50,
   -10,   0,  10,  10,  10,  10,   0, -10,
     0,  10,  20,  20,  20,  20,  10,   0,
     0,  10,  20,  40,  40,  20,  10,   0,
     0,  10,  20,  40,  40,  20,  10,   0,
     0,  10,  20,  20,  20,  20,  10,   0,
   -10,   0,  10,  10,  10,  10,   0, -10,
   -50, -10,   0,   0,   0,   0, -10, -50};


// Initialize bit masks used for evaluations
void InitEvalMasks() {

	int sq, tsq;

	// Start everything at 0
	for (sq = 0; sq < 64; ++sq) {
		IsolatedMask[sq] = 0ULL;
		WhitePassedMask[sq] = 0ULL;
		BlackPassedMask[sq] = 0ULL;
	}

	// For each square a pawn can be on
	for (sq = 8; sq < 56; ++sq) {

		// In front
		for (tsq = sq + 8; tsq < 64; tsq += 8)
			WhitePassedMask[sq] |= (1ULL << tsq);

		for (tsq = sq - 8; tsq >= 0; tsq -= 8)
			BlackPassedMask[sq] |= (1ULL << tsq);

		// Left side
		if (fileOf(sq) > FILE_A) {

			IsolatedMask[sq] |= fileBBs[fileOf(sq) - 1];

			for (tsq = sq + 7; tsq < 64; tsq += 8)
				WhitePassedMask[sq] |= (1ULL << tsq);

			for (tsq = sq - 9; tsq >= 0; tsq -= 8)
				BlackPassedMask[sq] |= (1ULL << tsq);
		}

		// Right side
		if (fileOf(sq) < FILE_H) {

			IsolatedMask[sq] |= fileBBs[fileOf(sq) + 1];

			for (tsq = sq + 9; tsq < 64; tsq += 8)
				WhitePassedMask[sq] |= (1ULL << tsq);

			for (tsq = sq - 7; tsq >= 0; tsq -= 8)
				BlackPassedMask[sq] |= (1ULL << tsq);
		}
	}
}

#ifdef CHECK_MAT_DRAW
// Check if the board is (likely) drawn, logic from sjeng
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

// Calculate a static evaluation of a position
int EvalPosition(const S_BOARD *pos) {

	assert(CheckBoard(pos));

#ifdef CHECK_MAT_DRAW
	if (MaterialDraw(pos)) return 0;
#endif

	int sq, i;
	int score = pos->material[WHITE] - pos->material[BLACK];

	bitboard whitePawns   = pos->colors[WHITE] & pos->pieceBBs[  PAWN];
	// bitboard whiteKnights = pos->colors[WHITE] & pos->pieceBBs[KNIGHT];
	// bitboard whiteBishops = pos->colors[WHITE] & pos->pieceBBs[BISHOP];
	// bitboard whiteRooks   = pos->colors[WHITE] & pos->pieceBBs[  ROOK];
	// bitboard whiteQueens  = pos->colors[WHITE] & pos->pieceBBs[ QUEEN];
	bitboard blackPawns   = pos->colors[BLACK] & pos->pieceBBs[  PAWN];
	// bitboard blackKnights = pos->colors[BLACK] & pos->pieceBBs[KNIGHT];
	// bitboard blackBishops = pos->colors[BLACK] & pos->pieceBBs[BISHOP];
	// bitboard blackRooks   = pos->colors[BLACK] & pos->pieceBBs[  ROOK];
	// bitboard blackQueens  = pos->colors[BLACK] & pos->pieceBBs[ QUEEN];


	// Bishop pair
	if (pos->pieceCounts[wB] >= 2)
		score += BishopPair;
	if (pos->pieceCounts[bB] >= 2)
		score -= BishopPair;

	// White pawns
	for (i = 0; i < pos->pieceCounts[wP]; ++i) {
		sq = pos->pieceList[wP][i];

		// Position score
		score += PawnTable[sq];
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

		// Position score
		score -= PawnTable[Mirror[(sq)]];
		// Isolation penalty
		if (!(IsolatedMask[sq] & blackPawns))
			score -= PawnIsolated;
		// Passed bonus
		if (!(BlackPassedMask[sq] & whitePawns))
			score -= PawnPassed[7 - rankOf(sq)];
	}

	// White knights
	for (i = 0; i < pos->pieceCounts[wN]; ++i) {
		sq = pos->pieceList[wN][i];
		score += KnightTable[sq];
	}

	// Black knights
	for (i = 0; i < pos->pieceCounts[bN]; ++i) {
		sq = pos->pieceList[bN][i];
		score -= KnightTable[Mirror[(sq)]];
	}

	// White bishops
	for (i = 0; i < pos->pieceCounts[wB]; ++i) {
		sq = pos->pieceList[wB][i];
		score += BishopTable[sq];
	}

	// Black bishops
	for (i = 0; i < pos->pieceCounts[bB]; ++i) {
		sq = pos->pieceList[bB][i];
		score -= BishopTable[Mirror[(sq)]];
	}

	// White rooks
	for (i = 0; i < pos->pieceCounts[wR]; ++i) {
		sq = pos->pieceList[wR][i];

		score += RookTable[sq];

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & fileBBs[fileOf(sq)]))
			score += RookOpenFile;
		else if (!(whitePawns & fileBBs[fileOf(sq)]))
			score += RookSemiOpenFile;
	}

	// Black rooks
	for (i = 0; i < pos->pieceCounts[bR]; ++i) {
		sq = pos->pieceList[bR][i];

		score -= RookTable[Mirror[(sq)]];

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

	// White king
	score += (pos->material[BLACK] <= ENDGAME_MAT) ?   KingEndgame[pos->KingSq[WHITE]] 
												   : KingEarlygame[pos->KingSq[WHITE]];

	// Black king
	score -= (pos->material[WHITE] <= ENDGAME_MAT) ?   KingEndgame[Mirror[(pos->KingSq[BLACK])]] 
												   : KingEarlygame[Mirror[(pos->KingSq[BLACK])]];

	assert(score > -INFINITE && score < INFINITE);

	// Return score
	return pos->side ? score : -score;
}