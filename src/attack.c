// attack.c

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "validate.h"

#ifdef USE_PEXT
#include "x86intrin.h"
#endif


typedef struct {
    bitboard *attacks;
    bitboard mask;
#ifndef USE_PEXT
    uint64_t magic;
    int shift;
#endif
} Magic;


static bitboard bishop_attacks[0x1480];
static bitboard rook_attacks[0x19000];

static Magic mBishopTable[64];
static Magic mRookTable[64];

bitboard pawn_attacks[2][64];
bitboard knight_attacks[64];
bitboard king_attacks[64];


// Inits the king attack bitboards
static void InitKingAttacks() {

    for (int sq = A1; sq <= H8; ++sq) {

        if (rankOf(sq) < RANK_8) {
            if (fileOf(sq) < FILE_H)
                SETBIT(king_attacks[sq], sq + 9);
            SETBIT(king_attacks[sq], sq + 8);
            if (fileOf(sq) > FILE_A)
                SETBIT(king_attacks[sq], sq + 7);
        }

        if (fileOf(sq) < FILE_H)
            SETBIT(king_attacks[sq], sq + 1);
        if (fileOf(sq) > FILE_A)
            SETBIT(king_attacks[sq], sq - 1);

        if (rankOf(sq) > RANK_1) {
            if (fileOf(sq) < FILE_H)
                SETBIT(king_attacks[sq], sq - 7);
            SETBIT(king_attacks[sq], sq - 8);
            if (fileOf(sq) > FILE_A)
                SETBIT(king_attacks[sq], sq - 9);
        }
    }
}

// Inits the knight attack bitboards
static void InitKnightAttacks() {

    for (int sq = A1; sq <= H8; ++sq) {

        if (rankOf(sq) < RANK_7) {
            if (fileOf(sq) < FILE_H)
                SETBIT(knight_attacks[sq], sq + 17);
            if (fileOf(sq) > FILE_A)
                SETBIT(knight_attacks[sq], sq + 15);
        }
        if (rankOf(sq) < RANK_8) {
            if (fileOf(sq) < FILE_G)
                SETBIT(knight_attacks[sq], sq + 10);
            if (fileOf(sq) > FILE_B)
                SETBIT(knight_attacks[sq], sq + 6);
        }
        if (rankOf(sq) > RANK_1) {
            if (fileOf(sq) < FILE_G)
                SETBIT(knight_attacks[sq], sq - 6);
            if (fileOf(sq) > FILE_B)
                SETBIT(knight_attacks[sq], sq - 10);
        }
        if (rankOf(sq) > RANK_2) {
            if (fileOf(sq) < FILE_H)
                SETBIT(knight_attacks[sq], sq - 15);
            if (fileOf(sq) > FILE_A)
                SETBIT(knight_attacks[sq], sq - 17);
        }
    }
}

// Inits the pawn attack bitboards
static void InitPawnAttacks() {

    // All squares needed despite pawns never being on 1. or 8. rank
    for (int sq = A1; sq <= H8; ++sq) {

        // White
        if (rankOf(sq) < RANK_8) {
            if (fileOf(sq) < FILE_H)
                SETBIT(pawn_attacks[WHITE][sq], sq + 9);
            if (fileOf(sq) > FILE_A)
                SETBIT(pawn_attacks[WHITE][sq], sq + 7);
        }
        // Black
        if (rankOf(sq) > RANK_1) {
            if (fileOf(sq) < FILE_H)
                SETBIT(pawn_attacks[BLACK][sq], sq - 7);
            if (fileOf(sq) > FILE_A)
                SETBIT(pawn_attacks[BLACK][sq], sq - 9);
        }
    }
}

static bitboard MakeSliderAttacks(const int sq, const bitboard occupied, const int directions[]) {

    bitboard result = 0;

    for (int dir = 0; dir < 4; ++dir)

        for (int tSq = sq + directions[dir];
             (A1 <= tSq) && (tSq <= H8) && (Distance(tSq, tSq - directions[dir]) == 1);
             tSq += directions[dir]) {

            SETBIT(result, tSq);

            if (occupied & (1ULL << tSq))
                break;
        }

    return result;
}

// Inits the magic shit
#ifdef USE_PEXT
static void InitSliderAttacks(Magic *table, bitboard *attackTable, const int *dir) {
#else
static void InitSliderAttacks(Magic *table, bitboard *attackTable, const bitboard *magics, const int *dir) {
#endif
    int size, index;
    bitboard edges, occupied;

    table[0].attacks = attackTable;

    for (int sq = A1; sq <= H8; ++sq) {

        edges = ((rank1BB | rank8BB) & ~rankBBs[rankOf(sq)]) 
              | ((fileABB | fileHBB) & ~fileBBs[fileOf(sq)]);

        table[sq].mask  = MakeSliderAttacks(sq, 0, dir) & ~edges;
#ifndef USE_PEXT
        table[sq].magic = magics[sq];
        table[sq].shift = 64 - PopCount(table[sq].mask);
#endif

        table[sq].attacks = sq == A1 ? attackTable : table[sq - 1].attacks + size + 1;

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

// Initializes all attack bitboards
static void InitAttacks() __attribute__((constructor));
static void InitAttacks() {

    const int bishopDirections[4] = {7, 9, -7, -9};
    const int   rookDirections[4] = {8, 1, -8, -1};

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

// Returns the attack bitboard for a bishop based on what squares are occupied
bitboard BishopAttacks(const int sq, bitboard occupied) {
#ifdef USE_PEXT
    return mBishopTable[sq].attacks[_pext_u64(occupied, mBishopTable[sq].mask)];
#else
    occupied     &= mBishopTable[sq].mask;
    occupied     *= mBishopTable[sq].magic;
    occupied    >>= mBishopTable[sq].shift;
    return mBishopTable[sq].attacks[occupied];
#endif
}

// Returns the attack bitboard for a rook based on what squares are occupied
bitboard RookAttacks(const int sq, bitboard occupied) {
#ifdef USE_PEXT
    return mRookTable[sq].attacks[_pext_u64(occupied, mRookTable[sq].mask)];
#else
    occupied     &= mRookTable[sq].mask;
    occupied     *= mRookTable[sq].magic;
    occupied    >>= mRookTable[sq].shift;
    return mRookTable[sq].attacks[occupied];
#endif
}

// Returns true if sq is attacked by side
bool SqAttacked(const int sq, const int side, const Position *pos) {

    assert(ValidSquare(sq));
    assert(ValidSide(side));
    assert(CheckBoard(pos));

    const bitboard bishops = pos->colorBBs[side] & (pos->pieceBBs[BISHOP] | pos->pieceBBs[QUEEN]);
    const bitboard rooks   = pos->colorBBs[side] & (pos->pieceBBs[  ROOK] | pos->pieceBBs[QUEEN]);

    if (     pawn_attacks[!side][sq] & pos->pieceBBs[PAWN]   & pos->colorBBs[side]
        || knight_attacks[sq]        & pos->pieceBBs[KNIGHT] & pos->colorBBs[side]
        ||   king_attacks[sq]        & pos->pieceBBs[KING]   & pos->colorBBs[side]
        || bishops & BishopAttacks(sq, pos->colorBBs[BOTH])
        || rooks   & RookAttacks(sq, pos->colorBBs[BOTH]))
        return true;

    return false;
}