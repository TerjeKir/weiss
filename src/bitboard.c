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

#include "bitboard.h"
#include "board.h"


const Bitboard FileBB[FILE_NB] = {
    fileABB, fileBBB, fileCBB, fileDBB, fileEBB, fileFBB, fileGBB, fileHBB
};

const Bitboard RankBB[RANK_NB] = {
    rank1BB, rank2BB, rank3BB, rank4BB, rank5BB, rank6BB, rank7BB, rank8BB
};

Bitboard BetweenBB[64][64];

static Bitboard BishopAttacks[5248];
static Bitboard RookAttacks[102400];

Magic Magics[64][2];

Bitboard PseudoAttacks[TYPE_NB][64];
Bitboard PawnAttacks[COLOR_NB][64];

Bitboard PassedMask[COLOR_NB][64];
Bitboard IsolatedMask[64];


// Returns a bitboard with the landing square of the step,
// or an empty bitboard if the step would go outside the board
INLINE Bitboard LandingSquareBB(const Square sq, const int step) {
    const Square to = sq + step;
    const bool inside = to <= H8 && Distance(sq, to) <= 2;
    return inside ? BB(to) : 0;
}

// Makes a slider attack bitboard
static Bitboard MakeSliderAttackBB(const Square sq, const PieceType pt, const Bitboard occupied) {

    Bitboard attacks = 0;

    const int BSteps[] = { 7, 9, -7, -9 };
    const int RSteps[] = { 8, 1, -8, -1 };
    const int *steps = pt == BISHOP ? BSteps : RSteps;

    for (int dir = 0; dir < 4; ++dir) {
        Square s = sq;
        // Step in the direction until hitting a piece or the edge of the board
        while(!(occupied & BB(s)) && LandingSquareBB(s, steps[dir]))
            attacks |= BB(s += steps[dir]);
    }

    return attacks;
}

// Initializes non-slider attack lookups
static void InitNonSliderAttacks() {

    int KSteps[8] = {  -9, -8, -7, -1,  1,  7,  8,  9 };
    int NSteps[8] = { -17,-15,-10, -6,  6, 10, 15, 17 };

    for (Square sq = A1; sq <= H8; ++sq) {

        // Kings and knights
        for (int i = 0; i < 8; ++i) {
            PseudoAttacks[KING][sq]   |= LandingSquareBB(sq, KSteps[i]);
            PseudoAttacks[KNIGHT][sq] |= LandingSquareBB(sq, NSteps[i]);
        }

        // Pawns
        PawnAttacks[WHITE][sq] =  LandingSquareBB(sq, NORTH + WEST)
                                | LandingSquareBB(sq, NORTH + EAST);
        PawnAttacks[BLACK][sq] =  LandingSquareBB(sq, SOUTH + WEST)
                                | LandingSquareBB(sq, SOUTH + EAST);
    }
}

// Initializes slider attack lookups
static void InitSliderAttacks(PieceType pt, Bitboard table[]) {

#ifndef USE_PEXT
    const uint64_t *magics = pt == ROOK ? RookMagics : BishopMagics;
#endif

    for (Square sq = A1; sq <= H8; ++sq) {

        Magic *m = &Magics[sq][pt - BISHOP];
        (*m).attacks = table;

        // Construct the mask
        Bitboard edges = ((rank1BB | rank8BB) & ~RankBB[RankOf(sq)])
                       | ((fileABB | fileHBB) & ~FileBB[FileOf(sq)]);

        (*m).mask = MakeSliderAttackBB(sq, pt, 0) & ~edges;

#ifndef USE_PEXT
        m.magic = magics[sq];
        m.shift = 64 - PopCount(m.mask);
#endif

        // Loop through all possible combinations of occupied squares, filling the table
        Bitboard occupied = 0;
        do {
            MagicAttack(sq, pt, occupied) = MakeSliderAttackBB(sq, pt, occupied);
            occupied = (occupied - (*m).mask) & (*m).mask; // Carry rippler
            table++;
        } while (occupied);
    }
}

// Initializes all bitboard lookups
CONSTR(2) InitBitboards() {

    InitNonSliderAttacks();

    InitSliderAttacks(BISHOP, BishopAttacks);
    InitSliderAttacks(ROOK, RookAttacks);

    for (Square sq1 = A1; sq1 <= H8; sq1++)
        for (Square sq2 = A1; sq2 <= H8; sq2++)
            for (PieceType pt = BISHOP; pt <= ROOK; pt++)
                if (AttackBB(pt, sq1, BB(sq2)) & BB(sq2))
                    BetweenBB[sq1][sq2] = AttackBB(pt, sq1, BB(sq2)) & AttackBB(pt, sq2, BB(sq1));

    for (Square sq = A1; sq <= H8; ++sq) {

        IsolatedMask[sq] = AdjacentFilesBB(sq);

        PassedMask[WHITE][sq] = ShiftBB(~rank1BB, NORTH * RelativeRank(WHITE, RankOf(sq)))
                              & (FileBB[FileOf(sq)] | AdjacentFilesBB(sq));

        PassedMask[BLACK][sq] = ShiftBB(~rank8BB, SOUTH * RelativeRank(BLACK, RankOf(sq)))
                              & (FileBB[FileOf(sq)] | AdjacentFilesBB(sq));
    }
}

// Returns a bitboard with all attackers of a square
Bitboard Attackers(const Position *pos, const Square sq, const Bitboard occ) {

    const Bitboard bishops = pieceBB(BISHOP) | pieceBB(QUEEN);
    const Bitboard rooks   = pieceBB(ROOK)   | pieceBB(QUEEN);

    return (PawnAttackBB(WHITE, sq) & colorPieceBB(BLACK, PAWN))
         | (PawnAttackBB(BLACK, sq) & colorPieceBB(WHITE, PAWN))
         | (AttackBB(KNIGHT, sq, occ) & pieceBB(KNIGHT))
         | (AttackBB(KING,   sq, occ) & pieceBB(KING))
         | (AttackBB(BISHOP, sq, occ) & bishops)
         | (AttackBB(ROOK,   sq, occ) & rooks);
}

// Checks whether a square is attacked by the given color
bool SqAttacked(const Position *pos, const Square sq, const Color color) {

    const Bitboard bishops = colorBB(color) & (pieceBB(BISHOP) | pieceBB(QUEEN));
    const Bitboard rooks   = colorBB(color) & (pieceBB(ROOK)   | pieceBB(QUEEN));

    return (   PawnAttackBB(!color, sq) & colorPieceBB(color, PAWN)
            || AttackBB(KNIGHT, sq, 0)  & colorPieceBB(color, KNIGHT)
            || AttackBB(KING,   sq, 0)  & colorPieceBB(color, KING)
            || AttackBB(BISHOP, sq, pieceBB(ALL)) & bishops
            || AttackBB(ROOK,   sq, pieceBB(ALL)) & rooks);
}

// Checks whether a king is attacked
bool KingAttacked(const Position *pos, const Color color) {
    return SqAttacked(pos, kingSq(color), !color);
}
