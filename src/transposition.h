// transposition.h

#pragma once

#include "types.h"


#define MINHASH 4
#define MAXHASH 16384
#define DEFAULTHASH 1024


enum { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };


int  GetPvLine(const int depth, S_BOARD *pos);
void ClearHashTable(S_HASHTABLE *table);
void  InitHashTable(S_HASHTABLE *table, uint64_t MB);
#ifdef SEARCH_STATS
bool  ProbeHashEntry(S_BOARD *pos, int *move, int *score, const int alpha, const int beta, const int depth);
#else
bool  ProbeHashEntry(const S_BOARD *pos, int *move, int *score, const int alpha, const int beta, const int depth);
#endif
void StoreHashEntry(S_BOARD *pos, const int move, const int score, const int flag, const int depth);
int HashFull(const S_BOARD *pos);