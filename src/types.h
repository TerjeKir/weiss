// types.h

#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

#include "defs.h"


typedef uint64_t bitboard;

enum Limit { MAXGAMEMOVES     = 512,
	         MAXPOSITIONMOVES = 256,
	         MAXDEPTH         = 128 };

enum Score { INFINITE = 30000,
			 ISMATE   = INFINITE - MAXDEPTH };

typedef enum Color {
	BLACK, WHITE, BOTH
} Color;

typedef enum PieceType {
	NO_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, TYPE_NB = 8
} PieceType;

typedef enum Piece {
	EMPTY, PIECE_MIN,
	bP = 1, bN, bB, bR, bQ, bK,
	wP = 9, wN, wB, wR, wQ, wK,
	PIECE_NB = 16
} Piece;

enum PieceValue {
	P_MG =  100, P_EG =  100,
	N_MG =  325, N_EG =  325,
	B_MG =  325, B_EG =  325,
	R_MG =  550, R_EG =  550,
	Q_MG = 1000, Q_EG = 1000
};

enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };

enum Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };

enum Square {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8, NO_SQ = 99
};

enum CastlingRights { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };

/* Structs */

typedef struct {
	int move;
	int score;
} MoveListEntry;

typedef struct {
	MoveListEntry moves[MAXPOSITIONMOVES];
	unsigned int count;
	unsigned int next;
} MoveList;

typedef struct {
	int move;
	int enPas;
	int fiftyMove;
	int castlePerm;
	uint64_t posKey;
} Undo;

typedef struct {
	uint64_t posKey;
	int move;
	int16_t score;
	uint8_t depth;
	uint8_t flag;
} HashEntry;

typedef struct {
	HashEntry *TT;
	int numEntries;
	uint64_t MB;
#ifdef SEARCH_STATS
	int newWrite;
	int overWrite;
	int hit;
	int cut;
#endif
} HashTable;

typedef struct {

	bitboard colorBBs[3];
	bitboard pieceBBs[TYPE_NB];

	int board[64];

	int pieceList[PIECE_NB][10];
	int pieceCounts[PIECE_NB];

	int kingSq[2];
	int bigPieces[2];

	int material;

	int side;
	int enPas;
	int fiftyMove;
	int castlePerm;

	int ply;
	int hisPly;

	uint64_t posKey;

	Undo history[MAXGAMEMOVES];

	HashTable hashTable[1];
	int pvArray[MAXDEPTH];

	int searchHistory[PIECE_NB][64];
	int searchKillers[2][MAXDEPTH];

} Position;

typedef struct {

	int starttime;
	int stoptime;
	unsigned int depth;
	int seldepth;
	int timeset;
	int movestogo;

	uint64_t nodes;
	uint64_t tbhits;

	int stopped;

#ifdef SEARCH_STATS
	float fh;
	float fhf;
	int nullCut;
#endif

	char syzygyPath[256];

} SearchInfo;

typedef struct {

	Position *pos;
	SearchInfo *info;
	char line[4096];

} SearchThread;

/* Functions */

static inline int fileOf(const int square) {
	return square & 7;
}

static inline int rankOf(const int square) {
	return square >> 3;
}

static inline int colorOf(const int piece) {
	return piece >> 3;
}

static inline int pieceTypeOf(const int piece) {
	return (piece & 7);
}

static inline int makePiece(const int color, const int type) {
	return (color << 3) + type;
}