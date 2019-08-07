// evaluate.c

#include <stdio.h>

#include "defs.h"
#include "validate.h"
#include "board.h"

#define ENDGAME_MAT (1 * PieceVal[wR] + 2 * PieceVal[wN] + 2 * PieceVal[wP] + PieceVal[wK])

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

	int pce;
	int pceNum;
	int sq;
	int score = pos->material[WHITE] - pos->material[BLACK];

	// White pawns
	pce = wP;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		score += PawnTable[SQ64(sq)];

		if ((IsolatedMask[SQ64(sq)] & pos->colors[WHITE] & pos->pieceBBs[0]) == 0)
			score += PawnIsolated;

		if ((WhitePassedMask[SQ64(sq)] & pos->colors[BLACK] & pos->pieceBBs[0]) == 0)
			score += PawnPassed[RanksBrd[sq]];
	}

	// Black pawns
	pce = bP;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);
		score -= PawnTable[MIRROR64(SQ64(sq))];

		if ((IsolatedMask[SQ64(sq)] & pos->colors[BLACK] & pos->pieceBBs[0]) == 0)
			score -= PawnIsolated;

		if ((BlackPassedMask[SQ64(sq)] & pos->colors[WHITE] & pos->pieceBBs[0]) == 0)
			score -= PawnPassed[7 - RanksBrd[sq]];
	}

	// White knights
	pce = wN;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		score += KnightTable[SQ64(sq)];
	}

	// Black knights
	pce = bN;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);
		score -= KnightTable[MIRROR64(SQ64(sq))];
	}

	// White bishops
	pce = wB;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		score += BishopTable[SQ64(sq)];
	}

	// Black bishops
	pce = bB;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);
		score -= BishopTable[MIRROR64(SQ64(sq))];
	}

	// White rooks
	pce = wR;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		score += RookTable[SQ64(sq)];

		assert(FileRankValid(FilesBrd[sq]));

		if (!(pos->pieceBBs[0] & FileBBMask[FilesBrd[sq]]))
			score += RookOpenFile;
		else if (!(pos->colors[WHITE] & pos->pieceBBs[0] & FileBBMask[FilesBrd[sq]]))
			score += RookSemiOpenFile;
	}

	// Black rooks
	pce = bR;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);
		score -= RookTable[MIRROR64(SQ64(sq))];
		assert(FileRankValid(FilesBrd[sq]));
		if (!(pos->pieceBBs[0] & FileBBMask[FilesBrd[sq]]))
			score -= RookOpenFile;
		else if (!(pos->colors[BLACK] & pos->pieceBBs[0] & FileBBMask[FilesBrd[sq]]))
			score -= RookSemiOpenFile;
	}

	// White queens
	pce = wQ;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		assert(FileRankValid(FilesBrd[sq]));
		if (!(pos->pieceBBs[0] & FileBBMask[FilesBrd[sq]]))
			score += QueenOpenFile;
		else if (!(pos->colors[WHITE] & pos->pieceBBs[0] & FileBBMask[FilesBrd[sq]]))
			score += QueenSemiOpenFile;
	}

	// Black queens
	pce = bQ;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		assert(SqOnBoard(sq));
		assert(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		assert(FileRankValid(FilesBrd[sq]));
		if (!(pos->pieceBBs[0] & FileBBMask[FilesBrd[sq]]))
			score -= QueenOpenFile;
		else if (!(pos->colors[BLACK] & pos->pieceBBs[0] & FileBBMask[FilesBrd[sq]]))
			score -= QueenSemiOpenFile;
	}

	// White king
	pce = wK;
	sq = pos->pList[pce][0];
	assert(SqOnBoard(sq));
	assert(SQ64(sq) >= 0 && SQ64(sq) <= 63);

	if (pos->material[BLACK] <= ENDGAME_MAT)
		score += KingE[SQ64(sq)];
	else
		score += KingO[SQ64(sq)];

	// Black king
	pce = bK;
	sq = pos->pList[pce][0];
	assert(SqOnBoard(sq));
	assert(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);

	if (pos->material[WHITE] <= ENDGAME_MAT)
		score -= KingE[MIRROR64(SQ64(sq))];
	else
		score -= KingO[MIRROR64(SQ64(sq))];

	// Bishop pair
	if (pos->pceNum[wB] >= 2)
		score += BishopPair;
	if (pos->pceNum[bB] >= 2)
		score -= BishopPair;

	if (pos->side == WHITE)
		return score;
	else
		return -score;
}
