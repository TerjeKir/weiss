// pvtable.c

#pragma once

#include "types.h"


// #define PV_STATS


void InitHashTable(S_HASHTABLE *table, const int MB);
void StoreHashEntry(S_BOARD *pos, const int move, int score, const int flags, const int depth);
int ProbeHashEntry(S_BOARD *pos, int *move, int *score, int alpha, int beta, int depth);
int GetPvLine(const int depth, S_BOARD *pos);
void ClearHashTable(S_HASHTABLE *table);