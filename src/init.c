// init.c

#include "attack.h"
#include "board.h"
#include "hashkeys.h"
#include "movegen.h"


bitboard   SetMask[64];
bitboard ClearMask[64];

bitboard FileBBMask[8]; // TODO
bitboard RankBBMask[8];

bitboard BlackPassedMask[64];
bitboard WhitePassedMask[64];
bitboard IsolatedMask[64];


static void InitEvalMasks() {

	int sq, tsq;

	for (int i = 0; i < 8; ++i) {
		FileBBMask[i] = 0ULL;
		RankBBMask[i] = 0ULL;
	}

	for (    int rank = RANK_8; rank >= RANK_1; --rank) {
		for (int file = FILE_A; file <= FILE_H; ++file) {
			sq = rank * 8 + file;
			FileBBMask[file] |= (1ULL << sq);
			RankBBMask[rank] |= (1ULL << sq);
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

		if (fileOf(sq) > FILE_A) {
			IsolatedMask[sq] |= FileBBMask[fileOf(sq) - 1];

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
			IsolatedMask[sq] |= FileBBMask[fileOf(sq) + 1];

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

void InitAll() {
	InitDistance();
	InitBitMasks();
	InitHashKeys();
	InitEvalMasks();
	InitMvvLva();
	InitAttacks();
}
