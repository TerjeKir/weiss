// transposition.h

#pragma once

#include "types.h"


#define MINHASH 4
#define MAXHASH 16384
#define DEFAULTHASH 1024


enum { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };


int  GetPvLine(const int depth, Position *pos);
void ClearHashTable(HashTable *table);
void  InitHashTable(HashTable *table, uint64_t MB);
#ifdef SEARCH_STATS
bool  ProbeHashEntry(Position *pos, int *move, int *score, const int alpha, const int beta, const int depth);
#else
bool  ProbeHashEntry(const Position *pos, int *move, int *score, const int alpha, const int beta, const int depth);
#endif
void StoreHashEntry(Position *pos, const int move, const int score, const int flag, const int depth);
int HashFull(const Position *pos);