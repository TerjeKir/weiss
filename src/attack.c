// attack.c

#include <stdio.h>

#include "defs.h"
#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "validate.h"


// Simple
bitboard pawn_attacks[2][64];
bitboard knight_attacks[64];
bitboard king_attacks[64];
// Magic
bitboard bishop_attacks[0x1480];
bitboard rook_attacks[0x19000];

MAGIC mBishopTable[64];
MAGIC mRookTable[64];


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

static bitboard MakeSliderAttacks(int sq, bitboard occupied, const int directions[]) {

    bitboard result = 0;

    for (int dir = 0; dir < 4; dir++)

        for (int tSq = sq + directions[dir];
             (tSq >= 0) && (tSq < 64) && (Distance(tSq, tSq - directions[dir]) == 1);
             tSq += directions[dir]) {

            SETBIT(result, tSq);

            if (occupied & (1ULL << tSq))
                break;
        }

    return result;
}

// Inits the magic shit
static void InitSliderAttacks(MAGIC *table, bitboard *attackTable, const bitboard *magics, const int *dir) {

    int size, index;
    bitboard edges, blockers;

    table[0].attacks = attackTable;

    for (int sq = 0; sq < 64; sq++) {

        edges = ((rank1BB | rank8BB) & ~rankBBs[sq / 8]) 
              | ((fileABB | fileHBB) & ~fileBBs[sq % 8]);

        table[sq].magic = magics[sq];
        table[sq].mask  = MakeSliderAttacks(sq, 0, dir) & ~edges;
        table[sq].shift = 64 - PopCount(table[sq].mask);

        table[sq].attacks = sq == 0 ? attackTable : table[sq - 1].attacks + size + 1;

        size = blockers = 0;

        do {
            index = (blockers * table[sq].magic) >> table[sq].shift;
            if (index > size) 
                size = index;
            table[sq].attacks[index] = MakeSliderAttacks(sq, blockers, dir);
            blockers = (blockers - table[sq].mask) & table[sq].mask; // Carry rippler
        } while (blockers);
    }
}

// Returns the attack bitboard for a slider based on what squares are occupied
bitboard SliderAttacks(int sq, bitboard occupied, MAGIC *table) {
    bitboard *ptr = table[sq].attacks;
    occupied     &= table[sq].mask;
    occupied     *= table[sq].magic;
    occupied    >>= table[sq].shift;
    return ptr[occupied];
}


// Initializes all attack bitboards
void InitAttacks() {
    InitKingAttacks();
    InitKnightAttacks();
    InitPawnAttacks();
    const int bishopDirections[4] = {7, 9, -7, -9};
    const int   rookDirections[4] = {8, 1, -8, -1};
    InitSliderAttacks(mBishopTable, bishop_attacks, BishopMagics, bishopDirections);
    InitSliderAttacks(mRookTable,     rook_attacks,   RookMagics,   rookDirections);
}

// Returns TRUE if sq is attacked by side
int SqAttacked(const int sq, const int side, const S_BOARD *pos) {
	
    bitboard bAtks, rAtks;
	int sq64 = SQ64(sq);

	assert(SqOnBoard(sq));
    assert(sq64 >= 0);
    assert(sq64 < 64);
	assert(SideValid(side));
	assert(CheckBoard(pos));

    bitboard allPieces = pos->allBB;
    bitboard bishops = pos->colors[side] & (pos->pieceBBs[BISHOP] | pos->pieceBBs[QUEEN]);
    bitboard rooks = pos->colors[side] & (pos->pieceBBs[ROOK] | pos->pieceBBs[QUEEN]);

	// Pawns
	if (pawn_attacks[!side][sq64] & pos->pieceBBs[PAWN] & pos->colors[side])
		return TRUE;

	// Knights
	if (knight_attacks[sq64] & pos->pieceBBs[KNIGHT] & pos->colors[side])
		return TRUE;

    // Kings
	if (king_attacks[sq64] & pos->pieceBBs[KING] & pos->colors[side])
		return TRUE;

    // Bishops
    bAtks = SliderAttacks(sq64, allPieces, mBishopTable);
    if (bAtks & bishops) return TRUE;

    // Rook
    rAtks = SliderAttacks(sq64, allPieces, mRookTable);
    if (rAtks & rooks) return TRUE;

	return FALSE;
}