// attack.c

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "validate.h"

#ifdef USE_PEXT
#include "x86intrin.h"
#endif


typedef struct {
    Bitboard *attacks;
    Bitboard mask;
#ifndef USE_PEXT
    uint64_t magic;
    int shift;
#endif
} Magic;


static Bitboard bishop_attacks[0x1480];
static Bitboard rook_attacks[0x19000];

static Magic BishopTable[64];
static Magic RookTable[64];

Bitboard pawn_attacks[2][64];
Bitboard knight_attacks[64];
Bitboard king_attacks[64];


// Helper function that returns a bitboard with the landing square of
// the step, or an empty bitboard of the step would go outside the board
INLINE Bitboard LandingSquare(int sq, int step) {

    const int to = sq + step;
    return (Bitboard)((unsigned)to <= H8 && Distance(sq, to) <= 2) << to;
}

// Inits the non-slider attack bitboards
static void InitNonSliderAttacks() {

    int KSteps[8] = {  -9,  -8,  -7, -1, 1,  7,  8,  9 };
    int NSteps[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
    int PSteps[2][2] = { { -7, -9 }, { 7, 9 } };

    for (int sq = A1; sq <= H8; ++sq) {

        // Kings and knights
        for (int i = 0; i < 8; ++i) {
            king_attacks[sq]   |= LandingSquare(sq, KSteps[i]);
            knight_attacks[sq] |= LandingSquare(sq, NSteps[i]);
        }

        // Pawns
        for (int i = 0; i < 2; ++i) {
            pawn_attacks[WHITE][sq] |= LandingSquare(sq, PSteps[WHITE][i]);
            pawn_attacks[BLACK][sq] |= LandingSquare(sq, PSteps[BLACK][i]);
        }
    }
}

// Makes slider attack bitboards
static Bitboard MakeSliderAttacks(const int sq, const Bitboard occupied, const int directions[]) {

    Bitboard result = 0;

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
static void InitSliderAttacks(Magic *m, Bitboard *table, const int *dir) {
#else
static void InitSliderAttacks(Magic *m, Bitboard *table, const uint64_t *magics, const int *dir) {
#endif
    int size = 0, index;
    Bitboard edges, occupied;

    m[0].attacks = table;

    for (int sq = A1; sq <= H8; ++sq) {

        // Construct the mask
        edges = ((rank1BB | rank8BB) & ~rankBBs[rankOf(sq)])
              | ((fileABB | fileHBB) & ~fileBBs[fileOf(sq)]);

        m[sq].mask  = MakeSliderAttacks(sq, 0, dir) & ~edges;

#ifndef USE_PEXT
        m[sq].magic = magics[sq];
        m[sq].shift = 64 - PopCount(m[sq].mask);
#endif

        m[sq].attacks = sq == A1 ? table : m[sq - 1].attacks + size;

        size = occupied = 0;

        do {
#ifdef USE_PEXT
            index = _pext_u64(occupied, m[sq].mask);
#else
            index = (occupied * m[sq].magic) >> m[sq].shift;
#endif
            m[sq].attacks[index] = MakeSliderAttacks(sq, occupied, dir);

            size++;
            occupied = (occupied - m[sq].mask) & m[sq].mask; // Carry rippler
        } while (occupied);
    }
}

// Initializes all attack bitboards
CONSTR InitAttacks() {

    // Non-sliders
    InitNonSliderAttacks();

    // Sliders
    const int bishopDirections[4] = {7, 9, -7, -9};
    const int   rookDirections[4] = {8, 1, -8, -1};

#ifdef USE_PEXT
    InitSliderAttacks(BishopTable, bishop_attacks, bishopDirections);
    InitSliderAttacks(  RookTable,   rook_attacks,   rookDirections);
#else
    InitSliderAttacks(BishopTable, bishop_attacks, BishopMagics, bishopDirections);
    InitSliderAttacks(  RookTable,   rook_attacks,   RookMagics,   rookDirections);
#endif
}

// Returns the attack bitboard for a bishop based on what squares are occupied
Bitboard BishopAttacks(const int sq, Bitboard occupied) {
#ifdef USE_PEXT
    return BishopTable[sq].attacks[_pext_u64(occupied, BishopTable[sq].mask)];
#else
    occupied  &= BishopTable[sq].mask;
    occupied  *= BishopTable[sq].magic;
    occupied >>= BishopTable[sq].shift;
    return BishopTable[sq].attacks[occupied];
#endif
}

// Returns the attack bitboard for a rook based on what squares are occupied
Bitboard RookAttacks(const int sq, Bitboard occupied) {
#ifdef USE_PEXT
    return RookTable[sq].attacks[_pext_u64(occupied, RookTable[sq].mask)];
#else
    occupied  &= RookTable[sq].mask;
    occupied  *= RookTable[sq].magic;
    occupied >>= RookTable[sq].shift;
    return RookTable[sq].attacks[occupied];
#endif
}

// Returns true if sq is attacked by side
bool SqAttacked(const int sq, const int side, const Position *pos) {

    assert(ValidSquare(sq));
    assert(ValidSide(side));
    assert(CheckBoard(pos));

    const Bitboard bishops = pos->colorBB[side] & (pos->pieceBB[BISHOP] | pos->pieceBB[QUEEN]);
    const Bitboard rooks   = pos->colorBB[side] & (pos->pieceBB[  ROOK] | pos->pieceBB[QUEEN]);

    if (     pawn_attacks[!side][sq] & pos->pieceBB[PAWN]   & pos->colorBB[side]
        || knight_attacks[sq]        & pos->pieceBB[KNIGHT] & pos->colorBB[side]
        ||   king_attacks[sq]        & pos->pieceBB[KING]   & pos->colorBB[side]
        || bishops & BishopAttacks(sq, pos->pieceBB[ALL])
        || rooks   &   RookAttacks(sq, pos->pieceBB[ALL]))
        return true;

    return false;
}