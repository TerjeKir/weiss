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

#include "bitboard.h"
#include "board.h"


const Bitboard FileBB[8] = {
    fileABB, fileBBB, fileCBB, fileDBB, fileEBB, fileFBB, fileGBB, fileHBB
};

const Bitboard RankBB[8] = {
    rank1BB, rank2BB, rank3BB, rank4BB, rank5BB, rank6BB, rank7BB, rank8BB
};

Bitboard SquareBB[64];
Bitboard BetweenBB[64][64];

static Bitboard BishopAttacks[0x1480];
static Bitboard RookAttacks[0x19000];

Magic BishopTable[64];
Magic RookTable[64];

Bitboard PseudoAttacks[TYPE_NB][64];
Bitboard PawnAttacks[2][64];

Bitboard PassedMask[2][64];
Bitboard IsolatedMask[64];


// Helper function that returns a bitboard with the landing square of
// the step, or an empty bitboard if the step would go outside the board
INLINE Bitboard LandingSquareBB(const Square sq, const int step) {

    const Square to = sq + step;
    return (Bitboard)(to <= H8 && Distance(sq, to) <= 2) << to;
}

// Helper function that makes a slider attack bitboard
static Bitboard MakeSliderAttackBB(const Square sq, const Bitboard occupied, const int steps[]) {

    Bitboard attacks = 0;

    for (int dir = 0; dir < 4; ++dir) {

        Square s = sq;
        while(!(occupied & SquareBB[s]) && LandingSquareBB(s, steps[dir]))
            attacks |= SquareBB[s += steps[dir]];
    }

    return attacks;
}

// Initializes non-slider attack lookups
static void InitNonSliderAttacks() {

    int KSteps[8] = {  -9, -8, -7, -1,  1,  7,  8,  9 };
    int NSteps[8] = { -17,-15,-10, -6,  6, 10, 15, 17 };
    int PSteps[2][2] = { { -9, -7 }, { 7, 9 } };

    for (Square sq = A1; sq <= H8; ++sq) {

        // Kings and knights
        for (int i = 0; i < 8; ++i) {
            PseudoAttacks[KING][sq]   |= LandingSquareBB(sq, KSteps[i]);
            PseudoAttacks[KNIGHT][sq] |= LandingSquareBB(sq, NSteps[i]);
        }

        // Pawns
        for (int i = 0; i < 2; ++i) {
            PawnAttacks[WHITE][sq] |= LandingSquareBB(sq, PSteps[WHITE][i]);
            PawnAttacks[BLACK][sq] |= LandingSquareBB(sq, PSteps[BLACK][i]);
        }
    }
}

// Initializes slider attack lookups
static void InitSliderAttacks(Magic *m, Bitboard *table, const int *steps) {

#ifndef USE_PEXT
    const uint64_t *magics = steps[0] == 8 ? RookMagics : BishopMagics;
#endif

    for (Square sq = A1; sq <= H8; ++sq) {

        m[sq].attacks = table;

        // Construct the mask
        Bitboard edges = ((rank1BB | rank8BB) & ~RankBB[RankOf(sq)])
                       | ((fileABB | fileHBB) & ~FileBB[FileOf(sq)]);

        m[sq].mask = MakeSliderAttackBB(sq, 0, steps) & ~edges;

#ifndef USE_PEXT
        m[sq].magic = magics[sq];
        m[sq].shift = 64 - PopCount(m[sq].mask);
#endif

        Bitboard occupied = 0;
        do {
            m[sq].attacks[AttackIndex(sq, occupied, m)] = MakeSliderAttackBB(sq, occupied, steps);
            occupied = (occupied - m[sq].mask) & m[sq].mask; // Carry rippler
            table++;
        } while (occupied);
    }
}

// Initializes evaluation bit masks
static void InitEvalMasks() {

    // For each square a pawn can be on
    for (Square sq = A2; sq <= H7; ++sq) {

        IsolatedMask[sq] = AdjacentFilesBB(sq);

        PassedMask[WHITE][sq] = ShiftBB(NORTH * RelativeRank(WHITE, RankOf(sq)), ~rank1BB)
                              & (FileBB[FileOf(sq)] | AdjacentFilesBB(sq));

        PassedMask[BLACK][sq] = ShiftBB(SOUTH * RelativeRank(BLACK, RankOf(sq)), ~rank8BB)
                              & (FileBB[FileOf(sq)] | AdjacentFilesBB(sq));
    }
}

// Initializes all bitboard lookups
CONSTR InitBitMasks() {

    for (Square sq = A1; sq <= H8; ++sq)
        SquareBB[sq] = (1ULL << sq);

    InitDistance();

    InitEvalMasks();

    InitNonSliderAttacks();

    const int BSteps[4] = { 7, 9, -7, -9 };
    const int RSteps[4] = { 8, 1, -8, -1 };

    InitSliderAttacks(BishopTable, BishopAttacks, BSteps);
    InitSliderAttacks(  RookTable,   RookAttacks, RSteps);

    for (Square sq1 = A1; sq1 <= H8; sq1++)
        for (Square sq2 = A1; sq2 <= H8; sq2++)
            for (PieceType pt = BISHOP; pt <= ROOK; pt++)
                if (AttackBB(pt, sq1, SquareBB[sq2]) & SquareBB[sq2])
                    BetweenBB[sq1][sq2] = AttackBB(pt, sq1, SquareBB[sq2]) & AttackBB(pt, sq2, SquareBB[sq1]);
}

// Checks whether a square is attacked by the given color
bool SqAttacked(const Position *pos, const Square sq, const Color color) {

    const Bitboard bishops = colorBB(color) & (pieceBB(BISHOP) | pieceBB(QUEEN));
    const Bitboard rooks   = colorBB(color) & (pieceBB(ROOK)   | pieceBB(QUEEN));

    return (   PawnAttackBB(!color, sq)           & colorPieceBB(color, PAWN)
            || AttackBB(KNIGHT, sq, pieceBB(ALL)) & colorPieceBB(color, KNIGHT)
            || AttackBB(KING,   sq, pieceBB(ALL)) & colorPieceBB(color, KING)
            || AttackBB(BISHOP, sq, pieceBB(ALL)) & bishops
            || AttackBB(ROOK,   sq, pieceBB(ALL)) & rooks);
}

// Checks whether a king is attacked
bool KingAttacked(const Position *pos, const Color color) {

    return SqAttacked(pos, Lsb(colorPieceBB(color, KING)), !color);
}
