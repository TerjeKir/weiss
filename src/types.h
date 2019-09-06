// types.h

#pragma once

#include <stdint.h>

#include "defs.h"


typedef uint64_t bitboard;

typedef enum Color { BLACK, WHITE, BOTH } Color;

enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK };

enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };

enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };

enum { UCIMODE, CONSOLEMODE };

enum {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8, NO_SQ = 99
};

enum { FALSE, TRUE };

enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };

/* STRUCTS */

typedef struct {
	int move;
	int score;
} S_MOVE;

typedef struct {
	S_MOVE moves[MAXPOSITIONMOVES];
	int count;
} S_MOVELIST;

typedef struct {

	int move;
	int castlePerm;
	int enPas;
	int fiftyMove;
	uint64_t posKey;

} S_UNDO;

typedef struct {
	uint64_t posKey;
	int move;
	int16_t score;
	uint8_t depth;
	uint8_t flag;
} S_HASHENTRY;

typedef struct {
	S_HASHENTRY *pTable;
	int numEntries;
#ifdef SEARCH_STATS
	int newWrite;
	int overWrite;
	int hit;
	int cut;
#endif
} S_HASHTABLE;

typedef struct {

	bitboard colors[2];
	bitboard pieceBBs[6]; 	// 0 Pawn  1 Knight 2 Bishop 3 Rook 4 Queen 5 King
	bitboard allBB;			// BB with all pieces

	int pieces[64];			// [square] -> empty/piece on that square

	int pieceList[13][10]; 	// [piece type][#] -> square
	int pieceCounts[13];	// # of each type of piece

	int KingSq[2]; 			// Square king is on
	int bigPieces[2];		// # of non-pawns
	int material[2];		// Total value of pieces

	int side;
	int enPas;
	int fiftyMove;
	int castlePerm;

	int ply;
	int hisPly;

	uint64_t posKey;

	S_UNDO history[MAXGAMEMOVES];

	S_HASHTABLE HashTable[1];
	int PvArray[MAXDEPTH];

	int searchHistory[13][64];
	int searchKillers[2][MAXDEPTH];

} S_BOARD;

typedef struct {

	int starttime;
	int stoptime;
	int depth;
	int seldepth;
	int timeset;
	int movestogo;

	uint64_t nodes;
	uint64_t tbhits;

	int quit;
	int stopped;

#ifdef SEARCH_STATS
	float fh;
	float fhf;
	int nullCut;
#endif

	int GAME_MODE;
	int POST_THINKING;

	char syzygyPath[256];

} S_SEARCHINFO;

inline int fileOf(const int square) {
	return square & 7;
}

inline int rankOf(const int square) {
	return square >> 3;
}