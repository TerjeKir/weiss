// types.h

#pragma once

#include <inttypes.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include "defs.h"


#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define INLINE static inline __attribute__((always_inline))
#define CONSTR static __attribute__((constructor)) void

#define history(offset) (pos->history[pos->hisPly + offset])
#define killer1 (pos->searchKillers[pos->ply][0])
#define killer2 (pos->searchKillers[pos->ply][1])

#define pieceBB(type) (pos->pieceBB[(type)])
#define colorBB(color) (pos->colorBB[(color)])
#define sideToMove() (pos->side)
#define pieceOn(sq) (pos->board[sq])


typedef uint64_t Bitboard;

typedef int64_t TimePoint;

enum Limit { MAXGAMEMOVES     = 512,
             MAXPOSITIONMOVES = 256,
             MAXDEPTH         = 128 };

enum Score { INFINITE = 32500,
             ISMATE   = INFINITE - MAXDEPTH,
             NOSCORE  = INFINITE + 1 };

typedef enum Color {
    BLACK, WHITE
} Color;

typedef enum PieceType {
    NO_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, TYPE_NB = 8
} PieceType;

typedef enum Piece {
    EMPTY = 0, ALL = 0, PIECE_MIN,
    bP = 1, bN, bB, bR, bQ, bK,
    wP = 9, wN, wB, wR, wQ, wK,
    PIECE_NB = 16
} Piece;

enum PieceValue {
    P_MG =  128, P_EG =  140,
    N_MG =  430, N_EG =  450,
    B_MG =  450, B_EG =  475,
    R_MG =  700, R_EG =  740,
    Q_MG = 1280, Q_EG = 1400
};

enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };

enum Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };

typedef enum Square {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8, NO_SQ = 99
} Square;

enum CastlingRights { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };

/* Structs */

typedef struct PV {
    int length;
    int line[MAXDEPTH];
} PV;

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
    int eval;
} History;

typedef struct {
    uint64_t posKey;
    int move;
    int16_t score;
    uint8_t depth;
    uint8_t flag;
} TTEntry;

typedef struct {

    int board[64];
    Bitboard pieceBB[TYPE_NB];
    Bitboard colorBB[2];

    int pieceCounts[PIECE_NB];
    int pieceList[PIECE_NB][10];
    int index[64];

    int nonPawns[2];

    int material;
    int basePhase;
    int phase;

    int side;
    int enPas;
    int fiftyMove;
    int castlePerm;

    int ply;
    int hisPly;

    uint64_t posKey;

    History history[MAXGAMEMOVES];

    int searchHistory[PIECE_NB][64];
    int searchKillers[MAXDEPTH][2];

} Position;

typedef struct {

    uint64_t nodes;
    uint64_t tbhits;

    int score;
    int depth;
    int bestMove;
    int ponderMove;
    int seldepth;

    PV pv;

    jmp_buf jumpBuffer;

} SearchInfo;

typedef struct {

    Position *pos;
    SearchInfo *info;
    char line[4096];

} SearchThread;

typedef struct {

    TimePoint start;
    int time, inc, movestogo, movetime, depth, maxUsage;
    bool timelimit;

} SearchLimits;

/* Functions */

INLINE int FileOf(const int square) {
    return square & 7;
}

INLINE int RankOf(const int square) {
    return square >> 3;
}

INLINE int ColorOf(const int piece) {
    return piece >> 3;
}

INLINE int PieceTypeOf(const int piece) {
    return (piece & 7);
}

INLINE int MakePiece(const int color, const int type) {
    return (color << 3) + type;
}