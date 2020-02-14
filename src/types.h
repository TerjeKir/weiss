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

#include <inttypes.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

// #define NDEBUG
#include <assert.h>


#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define INLINE static inline __attribute__((always_inline))
#define CONSTR static __attribute__((constructor)) void

#define history(offset) (pos->history[pos->gamePly + offset])
#define killer1 (pos->searchKillers[pos->ply][0])
#define killer2 (pos->searchKillers[pos->ply][1])

#define pieceBB(type) (pos->pieceBB[(type)])
#define colorBB(color) (pos->colorBB[(color)])
#define colorPieceBB(color, type) (colorBB(color) & pieceBB(type))
#define sideToMove() (pos->sideToMove)
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
    MAXGAMEMOVES     = 512,
    MAXPOSITIONMOVES = 256,
    MAXDEPTH         = 128
};

enum Score {
    INFINITE = 32500,
    ISMATE   = INFINITE - MAXDEPTH,
    NOSCORE  = INFINITE + 1
};

enum Color {
    BLACK, WHITE
};

enum PieceType {
    NO_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, TYPE_NB = 8
};

enum Piece {
    EMPTY = 0, ALL = 0, PIECE_MIN,
    bP = 1, bN, bB, bR, bQ, bK,
    wP = 9, wN, wB, wR, wQ, wK,
    PIECE_NB = 16
};

enum PieceValue {
    P_MG =  128, P_EG =  140,
    N_MG =  430, N_EG =  450,
    B_MG =  450, B_EG =  475,
    R_MG =  700, R_EG =  740,
    Q_MG = 1280, Q_EG = 1400
};

enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE
};

enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE
};

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

typedef enum Direction {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST
} Direction;

enum CastlingRights { WHITE_OO = 1, WHITE_OOO = 2, BLACK_OO = 4, BLACK_OOO = 8 };

/* Structs */

typedef struct PV {
    int length;
    Move line[MAXDEPTH];
} PV;

typedef struct {
    Move move;
    int score;
} MoveListEntry;

typedef struct {
    unsigned count;
    unsigned next;
    MoveListEntry moves[MAXPOSITIONMOVES];
} MoveList;

typedef struct {
    Key posKey;
    Move move;
    uint8_t epSquare;
    uint8_t rule50;
    uint8_t castlingRights;
    uint8_t padding; // not used
    int eval;
} History;

typedef struct {
    Key posKey;
    Move move;
    int16_t score;
    uint8_t depth;
    uint8_t flag;
} TTEntry;

typedef struct {

    uint8_t board[64];
    Bitboard pieceBB[TYPE_NB];
    Bitboard colorBB[2];

    int nonPawnCount[2];

    int material;
    int basePhase;
    int phase;

    Color sideToMove;
    uint8_t epSquare;
    uint8_t rule50;
    uint8_t castlingRights;

    uint8_t ply;
    uint16_t gamePly;

    Key key;

    History history[MAXGAMEMOVES];

    int searchHistory[PIECE_NB][64];
    Move searchKillers[MAXDEPTH][2];

} Position;

typedef struct {

    uint64_t nodes;
    uint64_t tbhits;

    int score;
    Depth depth;
    Move bestMove;
    Move ponderMove;
    Depth seldepth;

    PV pv;

    jmp_buf jumpBuffer;

} SearchInfo;

typedef struct {

    Position *pos;
    SearchInfo *info;
    char line[4096];

} ThreadInfo;

typedef struct {

    TimePoint start;
    int time, inc, movestogo, movetime, depth, maxUsage;
    bool timelimit;

} SearchLimits;

/* Functions */

INLINE int FileOf(const Square square) {
    return square & 7;
}

INLINE int RankOf(const Square square) {
    return square >> 3;
}

INLINE Color ColorOf(const Piece piece) {
    return piece >> 3;
}

INLINE PieceType PieceTypeOf(const Piece piece) {
    return (piece & 7);
}

INLINE Piece MakePiece(const Color color, const PieceType pt) {
    return (color << 3) + pt;
}

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