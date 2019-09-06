// attack.c

#include "attack.h"
#include "board.h"
#include "validate.h"
#include "bitboards.h"

#ifdef USE_PEXT
#include "x86intrin.h"
#endif


const int bishopDirections[4] = {7, 9, -7, -9};
const int   rookDirections[4] = {8, 1, -8, -1};


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

    for (int square = 0; square < 64; ++square, bitmask <<= 1) {

        if (square <= 55) {
            if ((fileOf(square)) != 7)
                king_attacks[square] |= (bitmask << 9);
            king_attacks[square] |= (bitmask << 8);
            if ((fileOf(square)) != 0)
                king_attacks[square] |= (bitmask << 7);
        }

        if ((fileOf(square)) != 7)
            king_attacks[square] |= (bitmask << 1);
        if ((fileOf(square)) != 0)
            king_attacks[square] |= (bitmask >> 1);

        if (square >= 8) {
            if ((fileOf(square)) != 7)
                king_attacks[square] |= (bitmask >> 7);
            king_attacks[square] |= (bitmask >> 8);
            if ((fileOf(square)) != 0)
                king_attacks[square] |= (bitmask >> 9);
        }
    }
}

// Inits the knight attack bitboards
static void InitKnightAttacks() {

    bitboard bitmask = 1ULL;

    for (int square = 0; square < 64; ++square, bitmask <<= 1) {

        if (square <= 47) {
            if ((fileOf(square)) < 7)
                knight_attacks[square] |= (bitmask << 17);
            if ((fileOf(square)) > 0)
                knight_attacks[square] |= (bitmask << 15);
        }
        if (square <= 55) {
            if ((fileOf(square)) < 6)
                knight_attacks[square] |= (bitmask << 10);
            if ((fileOf(square)) > 1)
                knight_attacks[square] |= (bitmask << 6);
        }
        if (square >= 8) {
            if ((fileOf(square)) < 6)
                knight_attacks[square] |= (bitmask >> 6);
            if ((fileOf(square)) > 1)
                knight_attacks[square] |= (bitmask >> 10);
        }
        if (square >= 16) {
            if ((fileOf(square)) < 7)
                knight_attacks[square] |= (bitmask >> 15);
            if ((fileOf(square)) > 0)
                knight_attacks[square] |= (bitmask >> 17);
        }
    }
}

// Inits the pawn attack bitboards
static void InitPawnAttacks() {

    bitboard bitmask = 1ULL;

    // No point going further as all their attacks would be off board
    for (int square = 0; square < 64; ++square, bitmask <<= 1) {

        // White
        if ((fileOf(square)) != 0)
            pawn_attacks[WHITE][square] |= bitmask << 7;
        if ((fileOf(square)) != 7)
            pawn_attacks[WHITE][square] |= bitmask << 9;
        // Black
        if ((fileOf(square)) != 7)
            pawn_attacks[BLACK][square] |= bitmask >> 7;
        if ((fileOf(square)) != 0)
            pawn_attacks[BLACK][square] |= bitmask >> 9;
    }
}

static bitboard MakeSliderAttacks(const int sq, const bitboard occupied, const int directions[]) {

    bitboard result = 0;

    for (int dir = 0; dir < 4; ++dir)

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
#ifdef USE_PEXT
static void InitSliderAttacks(MAGIC *table, bitboard *attackTable, const int *dir) {
#else
static void InitSliderAttacks(MAGIC *table, bitboard *attackTable, const bitboard *magics, const int *dir) {
#endif
    int size, index;
    bitboard edges, occupied;

    table[0].attacks = attackTable;

    for (int sq = 0; sq < 64; ++sq) {

        edges = ((rank1BB | rank8BB) & ~rankBBs[rankOf(sq)]) 
              | ((fileABB | fileHBB) & ~fileBBs[fileOf(sq)]);

        table[sq].mask  = MakeSliderAttacks(sq, 0, dir) & ~edges;
#ifndef USE_PEXT
        table[sq].magic = magics[sq];
        table[sq].shift = 64 - PopCount(table[sq].mask);
#endif

        table[sq].attacks = sq == 0 ? attackTable : table[sq - 1].attacks + size + 1;

        size = occupied = 0;

        do {
#ifdef USE_PEXT
            index = _pext_u64(occupied, table[sq].mask);
#else
            index = (occupied * table[sq].magic) >> table[sq].shift;
#endif
            if (index > size) 
                size = index;
            table[sq].attacks[index] = MakeSliderAttacks(sq, occupied, dir);
            occupied = (occupied - table[sq].mask) & table[sq].mask; // Carry rippler
        } while (occupied);
    }
}

// Returns the attack bitboard for a slider based on what squares are occupied
bitboard SliderAttacks(const int sq, bitboard occupied, const MAGIC *table) {
    bitboard *ptr = table[sq].attacks;
#ifdef USE_PEXT
    return ptr[_pext_u64(occupied, table[sq].mask)];
#else
    occupied     &= table[sq].mask;
    occupied     *= table[sq].magic;
    occupied    >>= table[sq].shift;
    return ptr[occupied];
#endif
}

// Initializes all attack bitboards
void InitAttacks() {

    // Simple
    InitKingAttacks();
    InitKnightAttacks();
    InitPawnAttacks();

    // Magic
#ifdef USE_PEXT
    InitSliderAttacks(mBishopTable, bishop_attacks, bishopDirections);
    InitSliderAttacks(  mRookTable,   rook_attacks,   rookDirections);
#else
    InitSliderAttacks(mBishopTable, bishop_attacks, BishopMagics, bishopDirections);
    InitSliderAttacks(  mRookTable,   rook_attacks,   RookMagics,   rookDirections);
#endif
}

// Returns TRUE if sq is attacked by side
int SqAttacked(const int sq, const int side, const S_BOARD *pos) {
	
    bitboard bAtks, rAtks;

    assert(ValidSquare(sq));
	assert(SideValid(side));
	assert(CheckBoard(pos));

    bitboard allPieces = pos->allBB;
    bitboard bishops   = pos->colors[side] & (pos->pieceBBs[BISHOP] | pos->pieceBBs[QUEEN]);
    bitboard rooks     = pos->colors[side] & (pos->pieceBBs[ROOK] | pos->pieceBBs[QUEEN]);

	// Pawns
	if (pawn_attacks[!side][sq] & pos->pieceBBs[PAWN] & pos->colors[side])
		return TRUE;

	// Knights
	if (knight_attacks[sq] & pos->pieceBBs[KNIGHT] & pos->colors[side])
		return TRUE;

    // Kings
	if (king_attacks[sq] & pos->pieceBBs[KING] & pos->colors[side])
		return TRUE;

    // Bishops
    bAtks = SliderAttacks(sq, allPieces, mBishopTable);
    if (bAtks & bishops) return TRUE;

    // Rook
    rAtks = SliderAttacks(sq, allPieces, mRookTable);
    if (rAtks & rooks) return TRUE;

	return FALSE;
}