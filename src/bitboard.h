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
#include "board.h"


#ifdef USE_PEXT
// Uses the bmi2 pext instruction in place of magic bitboards
#include "x86intrin.h"
#define AttackIndex(sq, occ, table) (_pext_u64(occ, table[sq].mask))

#else
// Uses magic bitboards as explained on https://www.chessprogramming.org/Magic_Bitboards
#define AttackIndex(sq, occ, table) (((occ & table[sq].mask) * table[sq].magic) >> table[sq].shift)

static const uint64_t RookMagics[64] = {
    0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull,
    0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
    0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull,
    0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
    0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull,
    0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
    0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull,
    0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
    0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull,
    0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
    0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull,
    0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
    0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull,
    0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
    0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull,
    0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull,
};

static const uint64_t BishopMagics[64] = {
    0xFFEDF9FD7CFCFFFFull, 0xFC0962854A77F576ull, 0x5822022042000000ull, 0x2CA804A100200020ull,
    0x0204042200000900ull, 0x2002121024000002ull, 0xFC0A66C64A7EF576ull, 0x7FFDFDFCBD79FFFFull,
    0xFC0846A64A34FFF6ull, 0xFC087A874A3CF7F6ull, 0x1001080204002100ull, 0x1810080489021800ull,
    0x0062040420010A00ull, 0x5028043004300020ull, 0xFC0864AE59B4FF76ull, 0x3C0860AF4B35FF76ull,
    0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, 0x040C0422080A0598ull, 0x4228020082004050ull,
    0x0200800400E00100ull, 0x020B001230021040ull, 0x7C0C028F5B34FF76ull, 0xFC0A028E5AB4DF76ull,
    0x0020208050A42180ull, 0x001004804B280200ull, 0x2048020024040010ull, 0x0102C04004010200ull,
    0x020408204C002010ull, 0x02411100020080C1ull, 0x102A008084042100ull, 0x0941030000A09846ull,
    0x0244100800400200ull, 0x4000901010080696ull, 0x0000280404180020ull, 0x0800042008240100ull,
    0x0220008400088020ull, 0x04020182000904C9ull, 0x0023010400020600ull, 0x0041040020110302ull,
    0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, 0x1401210240484800ull, 0x0022244208010080ull,
    0x1105040104000210ull, 0x2040088800C40081ull, 0x43FF9A5CF4CA0C01ull, 0x4BFFCD8E7C587601ull,
    0xFC0FF2865334F576ull, 0xFC0BF6CE5924F576ull, 0x80000B0401040402ull, 0x0020004821880A00ull,
    0x8200002022440100ull, 0x0009431801010068ull, 0xC3FFB7DC36CA8C89ull, 0xC3FF8A54F4CA2C89ull,
    0xFFFFFCFCFD79EDFFull, 0xFC0863FCCB147576ull, 0x040C000022013020ull, 0x2000104000420600ull,
    0x0400000260142410ull, 0x0800633408100500ull, 0xFC087E8E4BB2F736ull, 0x43FF9E4EF4CA2C89ull,
};
#endif

typedef struct {

    Bitboard *attacks;
    Bitboard mask;

#ifndef USE_PEXT
    uint64_t magic;
    int shift;
#endif

} Magic;

enum {
    fileABB = 0x0101010101010101,
    fileBBB = 0x0202020202020202,
    fileCBB = 0x0404040404040404,
    fileDBB = 0x0808080808080808,
    fileEBB = 0x1010101010101010,
    fileFBB = 0x2020202020202020,
    fileGBB = 0x4040404040404040,
    fileHBB = 0x8080808080808080,

    rank1BB = 0xFF,
    rank2BB = 0xFF00,
    rank3BB = 0xFF0000,
    rank4BB = 0xFF000000,
    rank5BB = 0xFF00000000,
    rank6BB = 0xFF0000000000,
    rank7BB = 0xFF000000000000,
    rank8BB = 0xFF00000000000000,
};

extern const Bitboard FileBB[8];
extern const Bitboard RankBB[8];

extern Bitboard SquareBB[64];
extern Bitboard BetweenBB[64][64];

extern Magic BishopTable[64];
extern Magic RookTable[64];

extern Bitboard PseudoAttacks[8][64];
extern Bitboard PawnAttacks[2][64];

// Eval bit masks
extern Bitboard PassedMask[2][64];
extern Bitboard IsolatedMask[64];


// Shifts a bitboard (protonspring version)
// Doesn't work for shifting more than one step horizontally
INLINE Bitboard ShiftBB(const int dir, Bitboard bb) {

    // Horizontal shifts should not wrap around
    const int h = dir & 7;
    bb = (h == 1) ? bb & ~fileHBB
       : (h == 7) ? bb & ~fileABB
                  : bb;

    // Can only shift by positive numbers
    return dir > 0 ? bb <<  dir
                   : bb >> -dir;
}

INLINE Bitboard AdjacentFilesBB(const Square sq) {

    return ShiftBB(WEST, FileBB[FileOf(sq)])
         | ShiftBB(EAST, FileBB[FileOf(sq)]);
}

// Population count/Hamming weight
INLINE int PopCount(const Bitboard bb) {

    return __builtin_popcountll(bb);
}

// Returns the index of the least significant bit
INLINE int Lsb(const Bitboard bb) {

    assert(bb);
    return __builtin_ctzll(bb);
}

// Returns the index of the least significant bit and unsets it
INLINE int PopLsb(Bitboard *bb) {

    int lsb = Lsb(*bb);
    *bb &= (*bb - 1);

    return lsb;
}

// Checks whether or not a bitboard has multiple set bits
INLINE bool Multiple(Bitboard bb) {

    return bb & (bb - 1);
}

// Checks whether or not a bitboard has a single set bit
INLINE bool Single(Bitboard bb) {

    return bb && !Multiple(bb);
}

// Returns the attack bitboard for a piece of piecetype on square sq
INLINE Bitboard AttackBB(PieceType piecetype, Square sq, Bitboard occupied) {

    assert(piecetype != PAWN);

    switch(piecetype) {
        case BISHOP: return BishopTable[sq].attacks[AttackIndex(sq, occupied, BishopTable)];
        case ROOK  : return   RookTable[sq].attacks[AttackIndex(sq, occupied, RookTable)];
        case QUEEN : return AttackBB(ROOK, sq, occupied) | AttackBB(BISHOP, sq, occupied);
        default    : return PseudoAttacks[piecetype][sq];
    }
}

// Returns the attack bitboard for a pawn
INLINE Bitboard PawnAttackBB(Color color, Square sq) {

    return PawnAttacks[color][sq];
}

// Returns the combined attack bitboard of all pawns in the given bitboard
INLINE Bitboard PawnBBAttackBB(Bitboard pawns, Color color) {

    const Direction up = (color == WHITE ? NORTH : SOUTH);

    return ShiftBB(up+WEST, pawns) | ShiftBB(up+EAST, pawns);
}

bool SqAttacked(const Position *pos, Square sq, Color color);
bool KingAttacked(const Position *pos, Color color);
