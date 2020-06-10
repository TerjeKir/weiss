/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2020  Terje Kirstihagen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#define NDEBUG
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>


// Macro for printing size_t
#ifdef _WIN32
#  ifdef _WIN64
#    define PRI_SIZET PRIu64
#  else
#    define PRI_SIZET PRIu32
#  endif
#else
#  define PRI_SIZET "zu"
#endif

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define INLINE static inline __attribute__((always_inline))
#define CONSTR static __attribute__((constructor)) void

#define lastMoveNullMove (!root && history(-1).move == NOMOVE)
#define history(offset) (pos->gameHistory[pos->histPly + offset])
#define killer1 (thread->killers[pos->ply][0])
#define killer2 (thread->killers[pos->ply][1])

#define pieceBB(type) (pos->pieceBB[(type)])
#define colorBB(color) (pos->colorBB[(color)])
#define colorPieceBB(color, type) (colorBB(color) & pieceBB(type))
#define sideToMove (pos->stm)
#define pieceOn(sq) (pos->board[sq])


typedef uint64_t Bitboard;
typedef uint64_t Key;

typedef uint32_t Move;
typedef uint32_t Square;

typedef int64_t TimePoint;

typedef int32_t Depth;
typedef int32_t Color;
typedef int32_t Piece;
typedef int32_t PieceType;


enum Limit {
    MAXGAMEMOVES     = 256,
    MAXPOSITIONMOVES = 256,
    MAXDEPTH         = 128
};

enum Score {
    TBWIN        = 30000,
    TBWIN_IN_MAX = TBWIN - 999,

    MATE        = 31000,
    MATE_IN_MAX = MATE - 999,

    INFINITE = MATE + 1,
    NOSCORE  = MATE + 2,
};

enum Color {
    BLACK, WHITE, COLOR_NB
};

enum {
    ALL, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, TYPE_NB = 8
};

enum {
    EMPTY,
    bP = 1, bN, bB, bR, bQ, bK,
    wP = 9, wN, wB, wR, wQ, wK,
    PIECE_NB = 16
};

enum PieceValue {
    P_MG =   90, P_EG =  154,
    N_MG =  398, N_EG =  478,
    B_MG =  416, B_EG =  488,
    R_MG =  565, R_EG =  854,
    Q_MG = 1391, Q_EG = 1567
};

enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE
};

enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE
};

enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

typedef enum Direction {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST
} Direction;

enum CastlingRights {
    WHITE_OO  = 1,
    WHITE_OOO = 2,
    BLACK_OO  = 4,
    BLACK_OOO = 8,

    OO  = WHITE_OO  | BLACK_OO,
    OOO = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLE = WHITE_OO | WHITE_OOO,
    BLACK_CASTLE = BLACK_OO | BLACK_OOO
};

typedef struct PV {
    int length;
    Move line[MAXDEPTH];
} PV;
