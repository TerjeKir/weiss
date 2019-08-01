// attack.c

#include <stdio.h>
#include "defs.h"
#include "attack.h"

const int KnDir[8] = {-8, -19, -21, -12, 8, 19, 21, 12};
const int RkDir[4] = {-1, -10, 1, 10};
const int BiDir[4] = {-9, -11, 11, 9};
const int KiDir[8] = {-1, -10, 1, 10, -9, -11, 11, 9};

uint64_t king_attacks[64];
uint64_t knight_attacks[64];
uint64_t pawn_attacks[2][64];

// Initializes all attack bitboards
void InitAttacks() {
    InitKingAttacks();
    InitKnightAttacks();
    InitPawnAttacks();
}

// Inits the king attack bitboards
void InitKingAttacks() {

    uint64_t bitmask = 1ULL;
    int square;

    for (square = 0; square < 64; square++, bitmask = bitmask << 1) {

        if (square <= 55) {
            if ((square % 8) != 7)
                king_attacks[square] ^= (bitmask << 9);
            king_attacks[square] ^= (bitmask << 8);
            if ((square % 8) != 0)
                king_attacks[square] ^= (bitmask << 7);
        }

        if ((square % 8) != 7)
            king_attacks[square] ^= (bitmask << 1);
        if ((square % 8) != 0)
            king_attacks[square] ^= (bitmask >> 1);

        if (square >= 8) {
            if ((square % 8) != 7)
                king_attacks[square] ^= (bitmask >> 7);
            king_attacks[square] ^= (bitmask >> 8);
            if ((square % 8) != 0)
                king_attacks[square] ^= (bitmask >> 9);
        }
    }
}

// Inits the knight attack bitboards
void InitKnightAttacks() {

    uint64_t bitmask = 1ULL;
    int square;

    for (square = 0; square < 64; square++, bitmask = bitmask << 1) {

        if (square <= 47) {
            if ((square % 8) < 7)
                knight_attacks[square] ^= (bitmask << 17);
            if ((square % 8) > 0)
                knight_attacks[square] ^= (bitmask << 15);
        }
        if (square <= 55) {
            if ((square % 8) < 6)
                knight_attacks[square] ^= (bitmask << 10);
            if ((square % 8) > 1)
                knight_attacks[square] ^= (bitmask << 6);
        }
        if (square >= 8) {
            if ((square % 8) < 6)
                knight_attacks[square] ^= (bitmask >> 6);
            if ((square % 8) > 1)
                knight_attacks[square] ^= (bitmask >> 10);
        }
        if (square >= 16) {
            if ((square % 8) < 7)
                knight_attacks[square] ^= (bitmask >> 15);
            if ((square % 8) > 0)
                knight_attacks[square] ^= (bitmask >> 17);
        }
    }
}

// Inits the pawn attack bitboards
void InitPawnAttacks() {

    uint64_t bitmask = 1ULL;
    int square;

    // No point going further as all their attacks would be off board
    for (square = 0; square < 56; square++, bitmask = bitmask << 1) {

        if ((square % 8) != 0)
            pawn_attacks[WHITE][square] |= bitmask << 7;

        if ((square % 8) != 7)
            pawn_attacks[WHITE][square] |= bitmask << 9;

        if ((square % 8) != 7)
            pawn_attacks[BLACK][square] |= bitmask >> 7;

        if ((square % 8) != 0)
            pawn_attacks[BLACK][square] |= bitmask >> 9;
    }
}

// Returns TRUE if sq is attacked by side
int SqAttacked(const int sq, const int side, const S_BOARD *pos) {
	
	int pce, index, t_sq, dir;
	int sq64 = SQ64(sq);

	ASSERT(SqOnBoard(sq));
	ASSERT(SideValid(side));
	ASSERT(CheckBoard(pos));

	// pawns
	if (pawn_attacks[side][sq64] & pos->pieceBBs[PAWN] & pos->colors[side])
		return TRUE;

	// knights
	if (pawn_attacks[side][sq64] & pos->pieceBBs[KNIGHT] & pos->colors[side])
		return TRUE;

	// rooks, queens
	for (index = 0; index < 4; ++index) {

		dir = RkDir[index];
		t_sq = sq + dir;
		ASSERT(SqIs120(t_sq));

		pce = pos->pieces[t_sq];
		ASSERT(PceValidEmptyOffbrd(pce));

		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsRQ(pce) && PieceCol[pce] == side) {
					return TRUE;
				}
				break;
			}
			t_sq += dir;
			ASSERT(SqIs120(t_sq));
			pce = pos->pieces[t_sq];
		}
	}

	// bishops, queens
	for (index = 0; index < 4; ++index) {

		dir = BiDir[index];
		t_sq = sq + dir;
		ASSERT(SqIs120(t_sq));

		pce = pos->pieces[t_sq];
		ASSERT(PceValidEmptyOffbrd(pce));

		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsBQ(pce) && PieceCol[pce] == side)
					return TRUE;
				break;
			}
			t_sq += dir;
			ASSERT(SqIs120(t_sq));
			pce = pos->pieces[t_sq];
		}
	}
	
	// kings
	if (pawn_attacks[side][sq64] & pos->pieceBBs[KING] & pos->colors[side])
		return TRUE;

	return FALSE;
}