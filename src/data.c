// data.c

#include "types.h"


const int PieceBig   [PIECE_NB] = { false, false,  true,  true,  true,  true, false, false,  true,  true,  true,  true, false };
const int PieceValues[PIECE_NB] = {     0,   100,   325,   325,   550,  1000, 50000,   100,   325,   325,   550,  1000, 50000 };
const int PieceColor [PIECE_NB] = {  BOTH, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };
const int pieceType  [PIECE_NB] = {    -1,  PAWN,KNIGHT,BISHOP,  ROOK, QUEEN,  KING,  PAWN,KNIGHT,BISHOP,  ROOK, QUEEN,  KING };

const int PiecePawn  [PIECE_NB] = { false,  true, false, false, false, false, false,  true, false, false, false, false, false };
const int PieceKnight[PIECE_NB] = { false, false,  true, false, false, false, false, false,  true, false, false, false, false };
const int PieceBishop[PIECE_NB] = { false, false, false,  true, false, false, false, false, false,  true, false, false, false };
const int PieceRook  [PIECE_NB] = { false, false, false, false,  true, false, false, false, false, false,  true, false, false };
const int PieceQueen [PIECE_NB] = { false, false, false, false, false,  true, false, false, false, false, false,  true, false };
const int PieceKing  [PIECE_NB] = { false, false, false, false, false, false,  true, false, false, false, false, false,  true };

const int Mirror[64] = {
	56, 57, 58, 59, 60, 61, 62, 63,
	48, 49, 50, 51, 52, 53, 54, 55,
	40, 41, 42, 43, 44, 45, 46, 47,
	32, 33, 34, 35, 36, 37, 38, 39,
	24, 25, 26, 27, 28, 29, 30, 31,
	16, 17, 18, 19, 20, 21, 22, 23,
	 8,  9, 10, 11, 12, 13, 14, 15,
	 0,  1,  2,  3,  4,  5,  6,  7};