// syzygy.c

#include "defs.h"
#include "fathom/tbprobe.h"

#define TB_PROBE_DEPTH 4
#define TB_LARGEST 6

const uint64_t m1  = 0x5555555555555555;
const uint64_t m2  = 0x3333333333333333;
const uint64_t m4  = 0x0f0f0f0f0f0f0f0f;
const uint64_t h01 = 0x0101010101010101;

int countPieces(S_BOARD *pos) {

    long long unsigned int x = pos->colors[WHITE] | pos->colors[BLACK];

    //// Best for slow multiplication
    x -= (x >> 1) & m1;              //put count of each 2 bits into those 2 bits
    x = (x & m2) + ((x >> 2) & m2);  //put count of each 4 bits into those 4 bits 
    x = (x + (x >> 4)) & m4;         //put count of each 8 bits into those 8 bits 
    x += x >>  8;                    //put count of each 16 bits into their lowest 8 bits
    x += x >> 16;                    //put count of each 32 bits into their lowest 8 bits
    x += x >> 32;                    //put count of each 64 bits into their lowest 8 bits
    
    return x & 0x7f;

    //// Best for fast multiplication
    // x -= (x >> 1) & m1;                 //put count of each 2 bits into those 2 bits
    // x = (x & m2) + ((x >> 2) & m2);     //put count of each 4 bits into those 4 bits 
    // x = (x + (x >> 4)) & m4;            //put count of each 8 bits into those 8 bits 

    // return (x * h01) >> 56;             //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 

    //// Built-in
    // return __builtin_popcount(x);
}

unsigned int probeWDL(S_BOARD *pos, int depth) {
    
    int cardinality = countPieces(pos);

    ASSERT(CheckBoard(pos));

    if (    (pos->ply == 0)
        ||  (pos->enPas != NO_SQ)
        ||  (pos->castlePerm != 0)
        ||  (pos->fiftyMove != 0)
        ||  (cardinality > (int)TB_LARGEST)
        ||  (cardinality == (int)TB_LARGEST && depth < (int)TB_PROBE_DEPTH))
        return TB_RESULT_FAILED;

    return tb_probe_wdl(
        pos->colors[WHITE],
        pos->colors[BLACK],
        pos->pieces[KING],
        pos->pieces[QUEEN],
        pos->pieces[ROOK],
        pos->pieces[BISHOP],
        pos->pieces[KNIGHT],
        pos->pieces[PAWN],
        pos->fiftyMove,
        pos->castlePerm,
        pos->enPas == NO_SQ ? 0 : pos->enPas,
        pos->side == WHITE ? 1 : 0);
}

