// attack.c

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "validate.h"


static Bitboard bishop_attacks[0x1480];
static Bitboard rook_attacks[0x19000];

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

// Inits the non-slider attack bitboards
static void InitNonSliderAttacks() {

    int KSteps[8] = {  -9,  -8,  -7, -1, 1,  7,  8,  9 };
    int NSteps[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
    int PSteps[2][2] = { { -7, -9 }, { 7, 9 } };

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
        edges = ((rank1BB | rank8BB) & ~rankBB[rankOf(sq)])
              | ((fileABB | fileHBB) & ~fileBB[fileOf(sq)]);

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

// Returns true if sq is attacked by color
bool SqAttacked(const int sq, const int color, const Position *pos) {

    assert(ValidSquare(sq));
    assert(ValidSide(color));
    assert(CheckBoard(pos));

    const Bitboard bishops = colorBB(color) & (pieceBB(BISHOP) | pieceBB(QUEEN));
    const Bitboard rooks   = colorBB(color) & (pieceBB(ROOK)   | pieceBB(QUEEN));

    if (   PawnAttacks[!color][sq]            & pieceBB(PAWN)   & colorBB(color)
        || AttackBB(KNIGHT, sq, pieceBB(ALL)) & pieceBB(KNIGHT) & colorBB(color)
        || AttackBB(KING,   sq, pieceBB(ALL)) & pieceBB(KING)   & colorBB(color)
        || AttackBB(BISHOP, sq, pieceBB(ALL)) & bishops
        || AttackBB(ROOK,   sq, pieceBB(ALL)) & rooks)
        return true;

    return false;
}