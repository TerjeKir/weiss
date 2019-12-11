// transposition.h

#pragma once

#include "types.h"


#define MINHASH 4
#define MAXHASH 16384
#define DEFAULTHASH 32


enum { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };


void ClearHashTable(HashTable *table);
void  InitHashTable(HashTable *table, uint64_t MB);
bool  ProbeHashEntry(const Position *pos, int *move, int *score, const int alpha, const int beta, const int depth);
void StoreHashEntry(Position *pos, const int move, const int score, const int flag, const int depth);
int HashFull(const Position *pos);