// evaluate.c

#include <stdio.h>

#include "defs.h"
#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "validate.h"


#define ENDGAME_MAT (1 * PieceVal[wR] + 2 * PieceVal[wN] + 2 * PieceVal[wP] + PieceVal[wK])
#define MIRROR64(sq) (Mirror64[(sq)])


extern bitboard BlackPassedMask[64];
extern bitboard WhitePassedMask[64];
extern bitboard IsolatedMask[64];

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


	// Bishop pair
	if (PopCount(whiteBishops) >= 2)
		score += BishopPair;
	if (PopCount(blackBishops) >= 2)
		score -= BishopPair;

	// White pawns
	while (whitePawns) {

		sq = PopLsb(&whitePawns);
		assert(sq >= 0 && sq < 64);

		// Position score
		score += PawnTable[sq];
		// Isolation penalty
		if ((IsolatedMask[sq] & pos->colors[WHITE] & pos->pieceBBs[PAWN]) == 0)
			score += PawnIsolated;
		// Passed bonus
		if ((WhitePassedMask[sq] & blackPawns) == 0)
			score += PawnPassed[RanksBrd64[sq]];
	}

	// Black pawns
	while (blackPawns) {

		sq = PopLsb(&blackPawns);
		assert(MIRROR64(sq) >= 0 && MIRROR64(sq) < 64);

		score -= PawnTable[MIRROR64(sq)];

		if ((IsolatedMask[sq] & pos->colors[BLACK] & pos->pieceBBs[PAWN]) == 0)
			score -= PawnIsolated;

		if ((BlackPassedMask[sq] & pos->colors[WHITE] & pos->pieceBBs[PAWN]) == 0)
			score -= PawnPassed[7 - RanksBrd64[sq]];
	}

	// White knights
	while (whiteKnights) {
		sq = PopLsb(&whiteKnights);
		assert(sq >= 0 && sq < 64);
		score += KnightTable[sq];
	}

	// Black knights
	while (blackKnights) {
		sq = MIRROR64(PopLsb(&blackKnights));
		assert(sq >= 0 && sq < 64);
		score -= KnightTable[sq];
	}

	// White bishops
	while (whiteBishops) {
		sq = PopLsb(&whiteBishops);
		assert(sq >= 0 && sq < 64);
		score += BishopTable[sq];
	}

	// Black bishops
	while (blackBishops) {
		sq = MIRROR64(PopLsb(&blackBishops));
		assert(sq >= 0 && sq < 64);
		score -= BishopTable[sq];
	}

	// White rooks
	while (whiteRooks) {

		sq = PopLsb(&whiteRooks);
		assert(sq >= 0 && sq < 64);

		score += RookTable[sq];

		assert(FileRankValid(FilesBrd64[sq]));

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & FileBBMask[FilesBrd64[sq]]))
			score += RookOpenFile;
		else if (!(pos->colors[WHITE] & pos->pieceBBs[PAWN] & FileBBMask[FilesBrd64[sq]]))
			score += RookSemiOpenFile;
	}

	// Black rooks
	while (blackRooks) {

		sq = PopLsb(&blackRooks);
		assert(sq >= 0 && sq < 64);

		score -= RookTable[MIRROR64(sq)];

		assert(FileRankValid(FilesBrd64[sq]));

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & FileBBMask[FilesBrd64[sq]]))
			score -= RookOpenFile;
		else if (!(pos->colors[BLACK] & pos->pieceBBs[PAWN] & FileBBMask[FilesBrd64[sq]]))
			score -= RookSemiOpenFile;
	}

	// White queens
	while (whiteQueens) {

		sq = PopLsb(&whiteQueens);
		assert(sq >= 0 && sq < 64);
		assert(FileRankValid(FilesBrd64[sq]));

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & FileBBMask[FilesBrd64[sq]]))
			score += QueenOpenFile;
		else if (!(pos->colors[WHITE] & pos->pieceBBs[PAWN] & FileBBMask[FilesBrd64[sq]]))
			score += QueenSemiOpenFile;
	}

	// Black queens
	while (blackQueens) {

		sq = PopLsb(&blackQueens);
		assert(sq >= 0 && sq < 64);
		assert(FileRankValid(FilesBrd64[sq]));

		// Open/Semi-open file bonus
		if (!(pos->pieceBBs[PAWN] & FileBBMask[FilesBrd64[sq]]))
			score -= QueenOpenFile;
		else if (!(pos->colors[BLACK] & pos->pieceBBs[PAWN] & FileBBMask[FilesBrd64[sq]]))
			score -= QueenSemiOpenFile;
	}

	// White king
	sq = SQ64(pos->KingSq[WHITE]);
	assert(sq >= 0 && sq < 64);
	if (pos->material[BLACK] <= ENDGAME_MAT)
		score += KingE[sq];
	else
		score += KingO[sq];

	// Black king
	sq = SQ64(pos->KingSq[BLACK]);
	assert(sq >= 0 && sq < 64);
	if (pos->material[WHITE] <= ENDGAME_MAT)
		score -= KingE[MIRROR64(sq)];
	else
		score -= KingO[MIRROR64(sq)];

	// Return score
	if (pos->side == WHITE)
		return score;
	else
		return -score;
}