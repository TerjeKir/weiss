// attack.c

#include <stdio.h>

#include "defs.h"
#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "validate.h"

const int RkDir[4] = {-1, -10, 1, 10};
const int BiDir[4] = {-9, -11, 11, 9};

bitboard king_attacks[64];
bitboard knight_attacks[64];
bitboard pawn_attacks[2][64];


// Inits the king attack bitboards
static void InitKingAttacks() {

    bitboard bitmask = 1ULL;
    int square;

    for (square = 0; square < 64; square++, bitmask <<= 1) {

        if (square <= 55) {
            if ((square % 8) != 7)
                king_attacks[square] |= (bitmask << 9);
            king_attacks[square] |= (bitmask << 8);
            if ((square % 8) != 0)
                king_attacks[square] |= (bitmask << 7);
        }

        if ((square % 8) != 7)
            king_attacks[square] |= (bitmask << 1);
        if ((square % 8) != 0)
            king_attacks[square] |= (bitmask >> 1);

        if (square >= 8) {
            if ((square % 8) != 7)
                king_attacks[square] |= (bitmask >> 7);
            king_attacks[square] |= (bitmask >> 8);
            if ((square % 8) != 0)
                king_attacks[square] |= (bitmask >> 9);
        }
    }
}

// Inits the knight attack bitboards
static void InitKnightAttacks() {

    bitboard bitmask = 1ULL;
    int square;

    for (square = 0; square < 64; square++, bitmask <<= 1) {

        if (square <= 47) {
            if ((square % 8) < 7)
                knight_attacks[square] |= (bitmask << 17);
            if ((square % 8) > 0)
                knight_attacks[square] |= (bitmask << 15);
        }
        if (square <= 55) {
            if ((square % 8) < 6)
                knight_attacks[square] |= (bitmask << 10);
            if ((square % 8) > 1)
                knight_attacks[square] |= (bitmask << 6);
        }
        if (square >= 8) {
            if ((square % 8) < 6)
                knight_attacks[square] |= (bitmask >> 6);
            if ((square % 8) > 1)
                knight_attacks[square] |= (bitmask >> 10);
        }
        if (square >= 16) {
            if ((square % 8) < 7)
                knight_attacks[square] |= (bitmask >> 15);
            if ((square % 8) > 0)
                knight_attacks[square] |= (bitmask >> 17);
        }
    }
}

// Inits the pawn attack bitboards
static void InitPawnAttacks() {

    bitboard bitmask = 1ULL;
    int square;

    // No point going further as all their attacks would be off board
    for (square = 0; square < 64; square++, bitmask <<= 1) {

        // White
        if ((square % 8) != 0)
            pawn_attacks[WHITE][square] |= bitmask << 7;
        if ((square % 8) != 7)
            pawn_attacks[WHITE][square] |= bitmask << 9;
        // Black
        if ((square % 8) != 7)
            pawn_attacks[BLACK][square] |= bitmask >> 7;
        if ((square % 8) != 0)
            pawn_attacks[BLACK][square] |= bitmask >> 9;
    }
}

// Initializes all attack bitboards
void InitAttacks() {
    InitKingAttacks();
    InitKnightAttacks();
    InitPawnAttacks();
}

// Returns TRUE if sq is attacked by side
int SqAttacked(const int sq, const int side, const S_BOARD *pos) {
	
	int pce, index, t_sq, dir;
	int sq64 = SQ64(sq);

	assert(SqOnBoard(sq));
	assert(SideValid(side));
	assert(CheckBoard(pos));

	// pawns
	if (pawn_attacks[!side][sq64] & pos->pieceBBs[PAWN] & pos->colors[side])
		return TRUE;

	// knights
	if (knight_attacks[sq64] & pos->pieceBBs[KNIGHT] & pos->colors[side])
		return TRUE;

    // kings
	if (king_attacks[sq64] & pos->pieceBBs[KING] & pos->colors[side])
		return TRUE;

	// rooks, queens
	for (index = 0; index < 4; ++index) {

		dir = RkDir[index];
		t_sq = sq + dir;
		assert(SqIs120(t_sq));

		pce = pos->pieces[t_sq];
		assert(PceValidEmptyOffbrd(pce));

		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsRQ(pce) && PieceCol[pce] == side)
					return TRUE;
				break;
			}
			t_sq += dir;
			assert(SqIs120(t_sq));
			pce = pos->pieces[t_sq];
		}
	}

	// bishops, queens
	for (index = 0; index < 4; ++index) {

		dir = BiDir[index];
		t_sq = sq + dir;
		assert(SqIs120(t_sq));

		pce = pos->pieces[t_sq];
		assert(PceValidEmptyOffbrd(pce));

		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsBQ(pce) && PieceCol[pce] == side)
					return TRUE;
				break;
			}
			t_sq += dir;
			assert(SqIs120(t_sq));
			pce = pos->pieces[t_sq];
		}
	}

	return FALSE;
}