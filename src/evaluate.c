// evaluate.c

#include "stdio.h"
#include "defs.h"

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

// sjeng 11.2
//8/6R1/2k5/6P1/8/8/4nP2/6K1 w - - 1 41
int MaterialDraw(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	if (!pos->pceNum[wR] && !pos->pceNum[bR] && !pos->pceNum[wQ] && !pos->pceNum[bQ]) {
		if (!pos->pceNum[bB] && !pos->pceNum[wB]) {
			if (pos->pceNum[wN] < 3 && pos->pceNum[bN] < 3) {
				return TRUE;
			}
		} else if (!pos->pceNum[wN] && !pos->pceNum[bN]) {
			if (abs(pos->pceNum[wB] - pos->pceNum[bB]) < 2) {
				return TRUE;
			}
		} else if ((pos->pceNum[wN] < 3 && !pos->pceNum[wB]) || (pos->pceNum[wB] == 1 && !pos->pceNum[wN])) {
			if ((pos->pceNum[bN] < 3 && !pos->pceNum[bB]) || (pos->pceNum[bB] == 1 && !pos->pceNum[bN])) {
				return TRUE;
			}
		}
	} else if (!pos->pceNum[wQ] && !pos->pceNum[bQ]) {
		if (pos->pceNum[wR] == 1 && pos->pceNum[bR] == 1) {
			if ((pos->pceNum[wN] + pos->pceNum[wB]) < 2 && (pos->pceNum[bN] + pos->pceNum[bB]) < 2) {
				return TRUE;
			}
		} else if (pos->pceNum[wR] == 1 && !pos->pceNum[bR]) {
			if ((pos->pceNum[wN] + pos->pceNum[wB] == 0) && (((pos->pceNum[bN] + pos->pceNum[bB]) == 1) || ((pos->pceNum[bN] + pos->pceNum[bB]) == 2))) {
				return TRUE;
			}
		} else if (pos->pceNum[bR] == 1 && !pos->pceNum[wR]) {
			if ((pos->pceNum[bN] + pos->pceNum[bB] == 0) && (((pos->pceNum[wN] + pos->pceNum[wB]) == 1) || ((pos->pceNum[wN] + pos->pceNum[wB]) == 2))) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

#define ENDGAME_MAT (1 * PieceVal[wR] + 2 * PieceVal[wN] + 2 * PieceVal[wP] + PieceVal[wK])

int EvalPosition(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	int pce;
	int pceNum;
	int sq;
	int score = pos->material[WHITE] - pos->material[BLACK];

	if (!pos->pceNum[wP] && !pos->pceNum[bP] && MaterialDraw(pos)) {
		return 0;
	}

	pce = wP;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		score += PawnTable[SQ64(sq)];

		if ((IsolatedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("wP Iso:%s\n", PrSq(sq));
			score += PawnIsolated;
		}

		if ((WhitePassedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("wP Passed:%s\n", PrSq(sq));
			score += PawnPassed[RanksBrd[sq]];
		}
	}

	pce = bP;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);
		score -= PawnTable[MIRROR64(SQ64(sq))];

		if ((IsolatedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("bP Iso:%s\n", PrSq(sq));
			score -= PawnIsolated;
		}

		if ((BlackPassedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("bP Passed:%s\n", PrSq(sq));
			score -= PawnPassed[7 - RanksBrd[sq]];
		}
	}

	pce = wN;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		score += KnightTable[SQ64(sq)];
	}

	pce = bN;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);
		score -= KnightTable[MIRROR64(SQ64(sq))];
	}

	pce = wB;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		score += BishopTable[SQ64(sq)];
	}

	pce = bB;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);
		score -= BishopTable[MIRROR64(SQ64(sq))];
	}

	pce = wR;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		score += RookTable[SQ64(sq)];

		ASSERT(FileRankValid(FilesBrd[sq]));

		if (!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += RookOpenFile;
		} else if (!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += RookSemiOpenFile;
		}
	}

	pce = bR;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);
		score -= RookTable[MIRROR64(SQ64(sq))];
		ASSERT(FileRankValid(FilesBrd[sq]));
		if (!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= RookOpenFile;
		} else if (!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= RookSemiOpenFile;
		}
	}

	pce = wQ;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if (!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += QueenOpenFile;
		} else if (!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += QueenSemiOpenFile;
		}
	}

	pce = bQ;
	for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq) >= 0 && SQ64(sq) <= 63);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if (!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= QueenOpenFile;
		} else if (!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= QueenSemiOpenFile;
		}
	}
	//8/p6k/6p1/5p2/P4K2/8/5pB1/8 b - - 2 62
	pce = wK;
	sq = pos->pList[pce][0];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq) >= 0 && SQ64(sq) <= 63);

	if ((pos->material[BLACK] <= ENDGAME_MAT)) {
		score += KingE[SQ64(sq)];
	} else {
		score += KingO[SQ64(sq)];
	}

	pce = bK;
	sq = pos->pList[pce][0];
	ASSERT(SqOnBoard(sq));
	ASSERT(MIRROR64(SQ64(sq)) >= 0 && MIRROR64(SQ64(sq)) <= 63);

	if ((pos->material[WHITE] <= ENDGAME_MAT)) {
		score -= KingE[MIRROR64(SQ64(sq))];
	} else {
		score -= KingO[MIRROR64(SQ64(sq))];
	}

	if (pos->pceNum[wB] >= 2)
		score += BishopPair;
	if (pos->pceNum[bB] >= 2)
		score -= BishopPair;

	if (pos->side == WHITE) {
		return score;
	} else {
		return -score;
	}
}
