// defs.h

#pragma once


#define NDEBUG
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>

#include "types.h"

/* DEFINITIONS */

#define NAME "weiss 0.2"

#define BRD_SQ_NUM 120

#define MAXGAMEMOVES 512
#define MAXPOSITIONMOVES 256
#define MAXDEPTH 128
#define MAXHASH 1024

#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define INFINITE 30000
#define ISMATE (INFINITE - MAXDEPTH)

#define MOVE_FLAG_ENPAS 0x40000
#define MOVE_FLAG_PAWNSTART 0x80000
#define MOVE_FLAG_CASTLE 0x1000000
#define MOVE_FLAG_CAP 0x7C000
#define MFLAGPROM 0xF00000

#define NOMOVE 0

/* MACROS */

#define FR2SQ(f, r) ( (21 + (f)) + ( (r) * 10))
#define SQ64(sq120) (Sq120ToSq64[(sq120)])
#define SQ120(sq64) (Sq64ToSq120[(sq64)])

#define CLRBIT(bb, sq) ((bb) &= ClearMask[(sq)])
#define SETBIT(bb, sq) ((bb) |= SetMask[(sq)])

#define IsBQ(p) (PieceBishopQueen[(p)])
#define IsRQ(p) (PieceRookQueen[(p)])
#define IsKn(p) (PieceKnight[(p)])
#define IsKi(p) (PieceKing[(p)])

#define MIRROR64(sq) (Mirror64[(sq)])

#define FROMSQ(m) ((m) & 0x7F)
#define TOSQ(m) (((m)>>7) & 0x7F)
#define CAPTURED(m) (((m)>>14) & 0xF)
#define PROMOTED(m) (((m)>>20) & 0xF)

/* GAME MOVE 
0000 0000 0000 0000 0000 0111 1111 -> From 0x7F
0000 0000 0000 0011 1111 1000 0000 -> To >> 7, 0x7F
0000 0000 0011 1100 0000 0000 0000 -> Captured >> 14, 0xF
0000 0000 0100 0000 0000 0000 0000 -> EP 0x40000
0000 0000 1000 0000 0000 0000 0000 -> Pawn Start 0x80000
0000 1111 0000 0000 0000 0000 0000 -> Promoted Piece >> 20, 0xF
0001 0000 0000 0000 0000 0000 0000 -> Castle 0x1000000
*/

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
	int score;
	int depth;
	int flags;
} S_HASHENTRY;

typedef struct {
	S_HASHENTRY *pTable;
	int numEntries;
	int newWrite;
	int overWrite;
	int hit;
	int cut;
} S_HASHTABLE;

typedef struct {

	bitboard colors[2];		// 0 White 1 Black
	bitboard pieceBBs[6]; 	// 0 Pawn  1 Knight 2 Bishop 3 Rook 4 Queen 5 King
	bitboard allBB;

	int pieces[BRD_SQ_NUM]; // (120)

	int pceNum[13];			// # of each type of piece
	int bigPce[2];			// # of non-pawns
	int material[2];		// Total value of pieces (from PieceVals)

	int pList[13][10]; 		// [piece type][#] -> square (120)

	int KingSq[2]; 			// 0 White 1 Black (120)

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

	int searchHistory[13][BRD_SQ_NUM];
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

	float fh;
	float fhf;
	int nullCut;

	int GAME_MODE;
	int POST_THINKING;

} S_SEARCHINFO;

typedef struct {
	char syzygyPath[256];
} S_OPTIONS;

/* GLOBALS */

extern int Sq120ToSq64[BRD_SQ_NUM];
extern int Sq64ToSq120[64];
extern bitboard SetMask[64];
extern bitboard ClearMask[64];
extern char PceChar[];
extern char SideChar[];
extern char RankChar[];
extern char FileChar[];

extern int PieceBig[13];
extern int PieceVal[13];
extern int PieceCol[13];
extern int PiecePawn[13];

extern int FilesBrd[BRD_SQ_NUM];
extern int RanksBrd[BRD_SQ_NUM];

extern int PieceKnight[13];
extern int PieceKing[13];
extern int PieceRookQueen[13];
extern int PieceBishopQueen[13];
extern int PieceSlides[13];

extern int Mirror64[64];

extern bitboard FileBBMask[8];
extern bitboard RankBBMask[8];

extern bitboard BlackPassedMask[64];
extern bitboard WhitePassedMask[64];
extern bitboard IsolatedMask[64];

extern S_OPTIONS EngineOptions[1];