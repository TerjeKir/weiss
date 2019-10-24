// data.c

#include "types.h"
#include "evaluate.h"

const int pieceValue[PIECE_NB] = { 0, S(P_MG, P_EG), S(N_MG, N_EG), S(B_MG, B_EG), S(R_MG, R_EG), S(Q_MG, Q_EG), 0, 0,
								   0, S(P_MG, P_EG), S(N_MG, N_EG), S(B_MG, B_EG), S(R_MG, R_EG), S(Q_MG, Q_EG), 0, 0 };

//                                EMPTY,    bP,    bN,    bB,    bR,    bQ,    bK, EMPTY, EMPTY,    wP,    wN,    wB,    wR,    wQ,    wK, EMPTY
const int pieceBig [PIECE_NB] = { false, false,  true,  true,  true,  true, false, false, false, false,  true,  true,  true,  true, false, false };
const int piecePawn[PIECE_NB] = { false,  true, false, false, false, false, false, false, false,  true, false, false, false, false, false, false };
const int pieceKing[PIECE_NB] = { false, false, false, false, false, false,  true, false, false, false, false, false, false, false,  true, false };

const int mirror[64] = {
	56, 57, 58, 59, 60, 61, 62, 63,
	48, 49, 50, 51, 52, 53, 54, 55,
	40, 41, 42, 43, 44, 45, 46, 47,
	32, 33, 34, 35, 36, 37, 38, 39,
	24, 25, 26, 27, 28, 29, 30, 31,
	16, 17, 18, 19, 20, 21, 22, 23,
	 8,  9, 10, 11, 12, 13, 14, 15,
	 0,  1,  2,  3,  4,  5,  6,  7};