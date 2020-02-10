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

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "validate.h"


static Bitboard BishopAttacks[0x1480];
static Bitboard RookAttacks[0x19000];

Magic BishopTable[64];
Magic RookTable[64];

Bitboard PseudoAttacks[TYPE_NB][64];
Bitboard PawnAttacks[2][64];


// Helper function that returns a bitboard with the landing square of
// the step, or an empty bitboard if the step would go outside the board
INLINE Bitboard LandingSquare(int sq, int step) {

    const int to = sq + step;
    return (Bitboard)((unsigned)to <= H8 && Distance(sq, to) <= 2) << to;
}

// Helper function that makes slider attack bitboards
static Bitboard MakeSliderAttacks(const int sq, const Bitboard occupied, const int steps[]) {

    Bitboard result = 0;

    for (int dir = 0; dir < 4; ++dir)

        for (int to = sq + steps[dir];
             (A1 <= to) && (to <= H8) && (Distance(to, to - steps[dir]) == 1);
             to += steps[dir]) {

            result |= (1ULL << to);

            if (occupied & (1ULL << to))
                break;
        }

    return result;
}

// Initializes non-slider attack bitboard lookups
static void InitNonSliderAttacks() {

    int KSteps[8] = {  -9, -8, -7, -1,  1,  7,  8,  9 };
    int NSteps[8] = { -17,-15,-10, -6,  6, 10, 15, 17 };
    int PSteps[2][2] = { { -9, -7 }, { 7, 9 } };

    for (int sq = A1; sq <= H8; ++sq) {

        // Kings and knights
        for (int i = 0; i < 8; ++i) {
            PseudoAttacks[KING][sq]   |= LandingSquare(sq, KSteps[i]);
            PseudoAttacks[KNIGHT][sq] |= LandingSquare(sq, NSteps[i]);
        }

        // Pawns
        for (int i = 0; i < 2; ++i) {
            PawnAttacks[WHITE][sq] |= LandingSquare(sq, PSteps[WHITE][i]);
            PawnAttacks[BLACK][sq] |= LandingSquare(sq, PSteps[BLACK][i]);
        }
    }
}

// Initializes rook or bishop attack lookups
#ifdef USE_PEXT
static void InitSliderAttacks(Magic *m, Bitboard *table, const int *dir) {
#else
static void InitSliderAttacks(Magic *m, Bitboard *table, const uint64_t *magics, const int *steps) {
#endif

    for (int sq = A1; sq <= H8; ++sq) {

        m[sq].attacks = table;

        // Construct the mask
        Bitboard edges = ((rank1BB | rank8BB) & ~RankBB[RankOf(sq)])
                       | ((fileABB | fileHBB) & ~FileBB[FileOf(sq)]);

        m[sq].mask  = MakeSliderAttacks(sq, 0, steps) & ~edges;

#ifndef USE_PEXT
        m[sq].magic = magics[sq];
        m[sq].shift = 64 - PopCount(m[sq].mask);
#endif

        Bitboard occupied = 0;
        do {
            m[sq].attacks[AttackIndex(sq, occupied, m)] = MakeSliderAttacks(sq, occupied, steps);
            occupied = (occupied - m[sq].mask) & m[sq].mask; // Carry rippler
            table++;
        } while (occupied);
    }
}

// Initializes all attack lookups
CONSTR InitAttacks() {

    InitNonSliderAttacks();

    const int BSteps[4] = { 7, 9, -7, -9 };
    const int RSteps[4] = { 8, 1, -8, -1 };

#ifdef USE_PEXT
    InitSliderAttacks(BishopTable, BishopAttacks, BSteps);
    InitSliderAttacks(  RookTable,   RookAttacks, RSteps);
#else
    InitSliderAttacks(BishopTable, BishopAttacks, BishopMagics, BSteps);
    InitSliderAttacks(  RookTable,   RookAttacks,   RookMagics, RSteps);
#endif
}

// Returns true if sq is attacked by color
bool SqAttacked(const int sq, const int color, const Position *pos) {

    assert(ValidSquare(sq));
    assert(ValidSide(color));
    assert(CheckBoard(pos));

    const Bitboard bishops = colorBB(color) & (pieceBB(BISHOP) | pieceBB(QUEEN));
    const Bitboard rooks   = colorBB(color) & (pieceBB(ROOK)   | pieceBB(QUEEN));

    if (   PawnAttackBB(!color, sq)          & pieceBB(PAWN)   & colorBB(color)
        || AttackBB(KNIGHT, sq, pieceBB(ALL)) & pieceBB(KNIGHT) & colorBB(color)
        || AttackBB(KING,   sq, pieceBB(ALL)) & pieceBB(KING)   & colorBB(color)
        || AttackBB(BISHOP, sq, pieceBB(ALL)) & bishops
        || AttackBB(ROOK,   sq, pieceBB(ALL)) & rooks)
        return true;

    return false;
}