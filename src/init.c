// init.c

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "attack.h"
#include "bitboards.h"
#include "hashkeys.h"
#include "movegen.h"
#include "validate.h"


int Sq120ToSq64[BRD_SQ_NUM];
int Sq64ToSq120[64];

int FilesBrd[BRD_SQ_NUM];
int RanksBrd[BRD_SQ_NUM];

int FilesBrd64[64];
int RanksBrd64[64];

bitboard SetMask[64];
bitboard ClearMask[64];

bitboard FileBBMask[8];
bitboard RankBBMask[8];

bitboard BlackPassedMask[64];
bitboard WhitePassedMask[64];
bitboard IsolatedMask[64];

S_OPTIONS EngineOptions[1];


static void InitEvalMasks() {

	int sq, tsq, r, f;

	for (sq = 0; sq < 8; ++sq) {
		FileBBMask[sq] = 0ULL;
		RankBBMask[sq] = 0ULL;
	}

	for (r = RANK_8; r >= RANK_1; --r) {
		for (f = FILE_A; f <= FILE_H; ++f) {
			sq = r * 8 + f;
			FileBBMask[f] |= (1ULL << sq);
			RankBBMask[r] |= (1ULL << sq);
		}
	}

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

		if (FilesBrd64[sq] > FILE_A) {
			IsolatedMask[sq] |= FileBBMask[FilesBrd64[sq] - 1];

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

		if (FilesBrd64[sq] < FILE_H) {
			IsolatedMask[sq] |= FileBBMask[FilesBrd64[sq] + 1];

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

static void InitFilesRanksBrd() {

	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;

	for (index = 0; index < BRD_SQ_NUM; ++index) {
		FilesBrd[index] = OFFBOARD;
		RanksBrd[index] = OFFBOARD;
	}

	for (rank = RANK_1; rank <= RANK_8; ++rank) {
		for (file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file, rank);
			FilesBrd[sq] = file;
			RanksBrd[sq] = rank;

			sq = (rank * 8) + file;
			FilesBrd64[sq] = file;
			RanksBrd64[sq] = rank;
		}
	}
}

static void InitBitMasks() {
	int index = 0;

	for (index = 0; index < 64; ++index) {
		SetMask[index] = 0ULL;
		ClearMask[index] = 0ULL;
	}

	for (index = 0; index < 64; ++index) {
		SetMask[index] |= (1ULL << index);
		ClearMask[index] = ~SetMask[index];
	}
}

static void InitSq120To64() {

	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;
	int sq64 = 0;
	for (index = 0; index < BRD_SQ_NUM; ++index)
		Sq120ToSq64[index] = 65;

	for (index = 0; index < 64; ++index)
		Sq64ToSq120[index] = 120;

	for (rank = RANK_1; rank <= RANK_8; ++rank) {
		for (file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file, rank);
			Sq64ToSq120[sq64] = sq;
			Sq120ToSq64[sq] = sq64;
			sq64++;
		}
	}
}

void InitAll() {
	InitDistance();
	InitSq120To64();
	InitBitMasks();
	InitHashKeys();
	InitFilesRanksBrd();
	InitEvalMasks();
	InitMvvLva();
	InitAttacks();
}
