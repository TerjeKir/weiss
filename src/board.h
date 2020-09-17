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

#include "types.h"


typedef struct {
    Key posKey;
    Move move;
    uint8_t epSquare;
    uint8_t rule50;
    uint8_t castlingRights;
    uint8_t padding; // not used
    int eval;
} History;

typedef struct Position {

    uint8_t board[64];
    Bitboard pieceBB[TYPE_NB];
    Bitboard colorBB[2];

    int nonPawnCount[2];

    int material;
    int phaseValue;
    int phase;

    Color stm;
    uint8_t epSquare;
    uint8_t rule50;
    uint8_t castlingRights;

    uint8_t ply;
    uint16_t histPly;
    uint16_t gameMoves;

    Key key;

    History gameHistory[MAXGAMEMOVES];

} Position;


extern uint8_t SqDistance[64][64];

extern const int NonPawn[PIECE_NB];

// Zobrist keys
extern uint64_t PieceKeys[PIECE_NB][64];
extern uint64_t CastleKeys[16];
extern uint64_t SideKey;


void InitDistance();
void ParseFen(const char *fen, Position *pos);
Key KeyAfter(const Position *pos, Move move);
bool SEE(const Position *pos, const Move move, const int threshold);
char *BoardToFen(const Position *pos);
#ifndef NDEBUG
void PrintBoard(const Position *pos);
bool PositionOk(const Position *pos);
#endif
#ifdef DEV
void PrintBoard(const Position *pos);
void MirrorBoard(Position *pos);
#endif

// Mirrors a square horizontally
INLINE Square MirrorSquare(const Square sq) {
    return sq ^ 56;
}

INLINE Square RelativeSquare(const Color color, const Square sq) {
    return color == WHITE ? sq : MirrorSquare(sq);
}

// Returns the same piece of the opposite color
INLINE Piece MirrorPiece(Piece piece) {
    return piece == EMPTY ? EMPTY : piece ^ 8;
}

// Returns the distance between two squares
INLINE int Distance(const Square sq1, const Square sq2) {
    return SqDistance[sq1][sq2];
}

INLINE bool ValidPiece(const Piece piece) {
    return (wP <= piece && piece <= wK)
        || (bP <= piece && piece <= bK);
}

INLINE bool ValidCapture(const Piece capt) {
    return (wP <= capt && capt <= wQ)
        || (bP <= capt && capt <= bQ);
}

INLINE bool ValidPromotion(const Piece promo) {
    return (wN <= promo && promo <= wQ)
        || (bN <= promo && promo <= bQ);
}

INLINE int FileOf(const Square square) {
    return square & 7;
}

INLINE int RankOf(const Square square) {
    return square >> 3;
}

// Converts a rank into the rank relative to the given color
INLINE int RelativeRank(const Color color, const int rank) {
    return color == WHITE ? rank : RANK_8 - rank;
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

INLINE Square AlgebraicToSq(const char file, const char rank) {
    return (file - 'a') + 8 * (rank - '1');
}
