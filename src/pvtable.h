// pvtable.h

#pragma once

#include "types.h"


#define MINHASH 4
#define MAXHASH 16384
#define DEFAULTHASH 1024


enum { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };


int  GetPvLine(const int depth, S_BOARD *pos);
void ClearhashTable(S_HASHTABLE *table);
int   InithashTable(S_HASHTABLE *table, uint64_t MB);
int  ProbeHashEntry(const S_BOARD *pos, int *move, int *score, const int alpha, const int beta, const int depth);
void StoreHashEntry(S_BOARD *pos, const int move, const int score, const int flag, const int depth);