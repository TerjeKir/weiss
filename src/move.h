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

#include "bitboard.h"
#include "types.h"

/* Move contents - total 27 bits used
0000 0000 0000 0000 0000 0000 0011 1111 -> From       <<  0
0000 0000 0000 0000 0000 1111 1100 0000 -> To         <<  6
0000 0000 0000 0000 1111 0000 0000 0000 -> Piece      << 12
0000 0000 0000 1111 0000 0000 0000 0000 -> Captured   << 16
0000 0000 1111 0000 0000 0000 0000 0000 -> Promotion  << 20
0000 0001 0000 0000 0000 0000 0000 0000 -> En passant << 24
0000 0010 0000 0000 0000 0000 0000 0000 -> Pawn Start << 25
0000 0100 0000 0000 0000 0000 0000 0000 -> Castle     << 26
*/

#define NOMOVE 0

// Fields
#define MOVE_FROM       0x000003F
#define MOVE_TO         0x0000FC0
#define MOVE_PIECE      0x000F000
#define MOVE_CAPT       0x00F0000
#define MOVE_PROMO      0x0F00000
#define MOVE_FLAGS      0x7000000

// Special move flags
#define FLAG_NONE       0
#define FLAG_ENPAS      0x1000000
#define FLAG_PAWNSTART  0x2000000
#define FLAG_CASTLE     0x4000000

// Move constructor
#define MOVE(f, t, pc, ca, pro, fl) ((f) | ((t) << 6) | ((pc) << 12) | ((ca) << 16) | ((pro) << 20) | (fl))

// Extract info from a move
#define fromSq(move)     ((move) & MOVE_FROM)
#define toSq(move)      (((move) & MOVE_TO)    >>  6)
#define piece(move)     (((move) & MOVE_PIECE) >> 12)
#define capturing(move) (((move) & MOVE_CAPT)  >> 16)
#define promotion(move) (((move) & MOVE_PROMO) >> 20)

// Move types
#define moveIsEnPas(move)   ((bool)(move & FLAG_ENPAS))
#define moveIsPStart(move)  ((bool)(move & FLAG_PAWNSTART))
#define moveIsCastle(move)  ((bool)(move & FLAG_CASTLE))
#define moveIsSpecial(move) ((bool)(move & (MOVE_FLAGS | MOVE_PROMO)))
#define moveIsCapture(move) ((bool)(move & MOVE_CAPT))
#define moveIsNoisy(move)   ((bool)(move & (MOVE_CAPT | MOVE_PROMO | FLAG_ENPAS)))
#define moveIsQuiet(move)   ((bool)(!moveIsNoisy(move)))


// Checks legality of a specific castle move given the current position
INLINE bool CastleLegal(const Position *pos, Square to) {

    assert(to == C1 || to == G1 || to == C8 || to == G8);

    Color color = RankOf(to) == RANK_1 ? WHITE : BLACK;
    int side = FileOf(to) == FILE_G ? OO : OOO;
    uint8_t castle = side & (color == WHITE ? WHITE_CASTLE : BLACK_CASTLE);

    if (   !(pos->castlingRights & castle)
        || pos->checkers
        || pieceBB(ALL) & CastlePath[castle])
        return false;

    Bitboard kingPath = BetweenBB[kingSq(color)][to] | BB(to);

    while (kingPath)
        if (SqAttacked(pos, PopLsb(&kingPath), !color))
            return false;

    return !chess960 || !(Attackers(pos, to, pieceBB(ALL) ^ BB(RookSquare[castle])) & colorBB(!color));
}

bool MoveIsPseudoLegal(const Position *pos, Move move);
char *MoveToStr(Move move);
Move ParseMove(const char *ptrChar, const Position *pos);
