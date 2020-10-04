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

#include "bitboard.h"
#include "types.h"

/* Move contents - total 23bits used
0000 0000 0000 0000 0011 1111 -> From       <<  0
0000 0000 0000 1111 1100 0000 -> To         <<  6
0000 0000 1111 0000 0000 0000 -> Captured   << 12
0000 1111 0000 0000 0000 0000 -> Promotion  << 16
0001 0000 0000 0000 0000 0000 -> En passant << 20
0010 0000 0000 0000 0000 0000 -> Pawn Start << 21
0100 0000 0000 0000 0000 0000 -> Castle     << 22
*/

#define NOMOVE 0

// Fields
#define MOVE_FROM       0x00003F
#define MOVE_TO         0x000FC0
#define MOVE_CAPT       0x00F000
#define MOVE_PROMO      0x0F0000
#define MOVE_FLAGS      0x700000

// Special move flags
#define FLAG_NONE       0
#define FLAG_ENPAS      0x100000
#define FLAG_PAWNSTART  0x200000
#define FLAG_CASTLE     0x400000

// Move constructor
#define MOVE(f, t, ca, pro, fl) ((f) | ((t) << 6) | ((ca) << 12) | ((pro) << 16) | (fl))

// Extract info from a move
#define fromSq(move)     ((move) & MOVE_FROM)
#define toSq(move)      (((move) & MOVE_TO)    >>  6)
#define capturing(move) (((move) & MOVE_CAPT)  >> 12)
#define promotion(move) (((move) & MOVE_PROMO) >> 16)

// Move types
#define moveIsEnPas(move)   (move & FLAG_ENPAS)
#define moveIsPStart(move)  (move & FLAG_PAWNSTART)
#define moveIsCastle(move)  (move & FLAG_CASTLE)
#define moveIsSpecial(move) (move & (MOVE_FLAGS | MOVE_PROMO))
#define moveIsCapture(move) (move & MOVE_CAPT)
#define moveIsNoisy(move)   (move & (MOVE_CAPT | MOVE_PROMO | FLAG_ENPAS))
#define moveIsQuiet(move)   (!moveIsNoisy(move))


// Checks legality of a specific castle move given the current position
INLINE bool CastlePseudoLegal(const Position *pos, Color color, int side) {

    uint8_t castle = color == WHITE ? side & WHITE_CASTLE
                                    : side & BLACK_CASTLE;

    Square kingSq = color == WHITE ? E1 : E8;

    Square rookSq = side == OO ? kingSq + 3 * EAST
                               : kingSq + 4 * WEST;

    Square midway = side == OO ? kingSq + EAST
                               : kingSq + WEST;

    Bitboard blocking = BetweenBB[kingSq][rookSq];

    return (pos->castlingRights & castle)
        && !(pieceBB(ALL) & blocking)
        && !SqAttacked(pos, kingSq, !color)
        && !SqAttacked(pos, midway, !color);
}

bool MoveIsPseudoLegal(const Position *pos, Move move);
char *MoveToStr(Move move);
Move ParseMove(const char *ptrChar, const Position *pos);
