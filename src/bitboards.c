// bitboards.c

#include <stdio.h>

#include "types.h"
#include "bitboards.h"

const int BitTable[64] = {
	63, 30,  3, 32, 25, 41, 22, 33,
	15, 50, 42, 13, 11, 53, 19, 34,
	61, 29,  2, 51, 21, 43, 45, 10,
	18, 47,  1, 54,  9, 57,  0, 35,
	62, 31, 40,  4, 49,  5, 52, 26,
	60,  6, 23, 44, 46, 27, 56, 16,
	 7, 39, 48, 24, 59, 14, 12, 55,
	38, 28, 58, 20, 37, 17, 36,  8};

const uint64_t m1  = 0x5555555555555555;
const uint64_t m2  = 0x3333333333333333;
const uint64_t m4  = 0x0f0f0f0f0f0f0f0f;
const uint64_t h01 = 0x0101010101010101;


int PopCount(uint64_t x) {

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

    //// Built-in fastest?
    // return __builtin_popcount(x); ?
    // return __builtin_popcountll(x); ?
}

// Returns the index of the least significant bit
inline int Lsb(uint64_t *bb) {

	uint64_t b = *bb ^ (*bb - 1);

	unsigned int fold = (unsigned)((b & 0xffffffff) ^ (b >> 32));

	return BitTable[(fold * 0x783a9b23) >> 26];
}

// Returns the index of the least significant bit and unsets it
int PopLsb(uint64_t *bb) {

	int lsb = Lsb(bb);
	*bb &= (*bb - 1);

	return lsb;
}

// Prints a bitboard
void PrintBB(uint64_t bb) {
    
    uint64_t bitmask = 1;

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file <= 7; file++) {
            if (bb & (bitmask << ((rank * 8) + file)))
                printf("1");
            else
                printf("0");
        }
        printf("\n");
    }
}
