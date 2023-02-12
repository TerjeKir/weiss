/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2023 Terje Kirstihagen

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
    Key key;
    Key materialKey;
    Bitboard checkers;
    Move move;
    Square epSquare;
    int rule50;
    int castlingRights;
} History;

typedef struct Position {
    uint8_t board[64];
    Bitboard pieceBB[7];
    Bitboard colorBB[COLOR_NB];
    Bitboard checkers;

    int nonPawnCount[COLOR_NB];
    int material;
    int phaseValue;
    int phase;

    Color stm;
    Square epSquare;
    int rule50;
    int castlingRights;

    uint16_t histPly;
    uint16_t gameMoves;

    Key key;
    Key materialKey;
    Key pawnKey;

    uint64_t nodes;
    int trend;

    History gameHistory[256];
} Position;


extern bool chess960;

extern uint8_t SqDistance[64][64];

extern const int NonPawn[PIECE_NB];

// Zobrist keys
extern uint64_t PieceKeys[PIECE_NB][64];
extern uint64_t CastleKeys[16];
extern uint64_t SideKey;

extern uint8_t CastlePerm[64];
extern Bitboard CastlePath[16];
extern Square RookSquare[16];


void ParseFen(const char *fen, Position *pos);
Key KeyAfter(const Position *pos, Move move);
bool SEE(const Position *pos, const Move move, const int threshold);
bool HasCycle(const Position *pos, int ply);
char *BoardToFen(const Position *pos);
#ifndef NDEBUG
void PrintBoard(const Position *pos);
bool PositionOk(const Position *pos);
#endif
#ifdef DEV
void PrintBoard(const Position *pos);
#endif

static bool ValidPiece(const Piece piece) { return (wP <= piece && piece <= wK) || (bP <= piece && piece <= bK); }
static bool ValidCapture(const Piece capt) { return (wP <= capt && capt <= wQ) || (bP <= capt && capt <= bQ); }
static bool ValidPromotion(const Piece promo) { return (wN <= promo && promo <= wQ) || (bN <= promo && promo <= bQ); }

INLINE Color ColorOf(Piece piece) { return piece >> 3;}
INLINE PieceType PieceTypeOf(Piece piece) { return piece & 7; }
INLINE Piece MakePiece(Color color, PieceType pt) { return (color << 3) + pt; }

INLINE Square MirrorSquare(const Square sq) { return sq ^ 56; } // Mirrors a square horizontally
INLINE Square RelativeSquare(const Color color, const Square sq) { return color == WHITE ? sq : MirrorSquare(sq); }
INLINE Square BlackRelativeSquare(const Color color, const Square sq) { return color == BLACK ? sq : MirrorSquare(sq); }
INLINE int Distance(const Square sq1, const Square sq2) { return SqDistance[sq1][sq2]; }
INLINE int FileOf(Square square) { return square & 7; }
INLINE int RankOf(Square square) { return square >> 3; }
INLINE int RelativeRank(Color color, int rank) { return color == WHITE ? rank : RANK_8 - rank; }
INLINE Square MakeSquare(int rank, int file) { return (rank * FILE_NB) + file; }
INLINE Square StrToSq(const char *str) { return MakeSquare(str[1] - '1', str[0] - 'a'); }

INLINE void SqToStr(Square sq, char *str) {
    str[0] = 'a' + FileOf(sq);
    str[1] = '1' + RankOf(sq);
}

INLINE bool IsRepetition(const Position *pos, int count) {
    int c = 0;
    for (int i = 4; i <= pos->rule50 && i <= pos->histPly; i += 2) {
        if (pos->key == history(-i).key)
            c++;
        if (c == count)
            return true;
    }
    return false;
}
