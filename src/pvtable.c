// pvtable.c

#include <stdio.h>
#include <stdlib.h>

#include "makemove.h"
#include "movegen.h"
#include "pvtable.h"


static int ProbePvMove(const S_BOARD *pos) {

	int index = pos->posKey % pos->HashTable->numEntries;
	assert(index >= 0 && index <= pos->HashTable->numEntries - 1);

	if (pos->HashTable->pTable[index].posKey == pos->posKey)
		return pos->HashTable->pTable[index].move;

	return NOMOVE;
}

int GetPvLine(const int depth, S_BOARD *pos) {

	assert(depth < MAXDEPTH && depth >= 1);

	int move = ProbePvMove(pos);
	int count = 0;

	while (move != NOMOVE && count < depth) {

		assert(count < MAXDEPTH);

		if (MoveExists(pos, move)) {
			MakeMove(pos, move);
			pos->PvArray[count++] = move;
		} else
			break;

		move = ProbePvMove(pos);
	}

	while (pos->ply > 0)
		TakeMove(pos);

	return count;
}

void ClearHashTable(S_HASHTABLE *table) {

	S_HASHENTRY *tableEntry;

	for (tableEntry = table->pTable; tableEntry < table->pTable + table->numEntries; ++tableEntry) {
		tableEntry->posKey = 0ULL;
		tableEntry->move = NOMOVE;
		tableEntry->depth = 0;
		tableEntry->score = 0;
		tableEntry->flags = 0;
	}
	table->newWrite = 0;
}

void InitHashTable(S_HASHTABLE *table, const uint64_t MB) {

	uint64_t HashSize = 0x100000LL * MB;
	table->numEntries = HashSize / sizeof(S_HASHENTRY);

	// Free memory if we have already allocated
	if (table->pTable != NULL)
		free(table->pTable);

	// Allocate memory
	table->pTable = (S_HASHENTRY *)calloc(table->numEntries, sizeof(S_HASHENTRY));

	// If allocation fails, try half the size
	if (table->pTable == NULL) {
		printf("Hash Allocation Failed, trying %I64dMB...\n", MB / 2);
		InitHashTable(table, MB / 2);
	// Success
	} else {
		table->newWrite = 0;
		printf("HashTable init complete with %d entries, using %I64dMB.\n", table->numEntries, MB);
	}
}

int ProbeHashEntry(S_BOARD *pos, int *move, int *score, int alpha, int beta, int depth) {

	int index = pos->posKey % pos->HashTable->numEntries;

	assert(index >= 0 && index <= pos->HashTable->numEntries - 1);
	assert(depth >= 1 && depth < MAXDEPTH);
	assert(alpha < beta);
	assert(alpha >= -INFINITE && alpha <= INFINITE);
	assert(beta >= -INFINITE && beta <= INFINITE);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	if (pos->HashTable->pTable[index].posKey == pos->posKey) {

		*move = pos->HashTable->pTable[index].move;
		if (pos->HashTable->pTable[index].depth >= depth) {

#ifdef PV_STATS
			pos->HashTable->hit++;
#endif
			assert(pos->HashTable->pTable[index].depth >= 1 && pos->HashTable->pTable[index].depth < MAXDEPTH);
			assert(pos->HashTable->pTable[index].flags >= HFALPHA && pos->HashTable->pTable[index].flags <= HFEXACT);

			*score = pos->HashTable->pTable[index].score;
			assert(*score >= -INFINITE);
			assert(*score <= INFINITE);

			if (*score > ISMATE)
				*score -= pos->ply;
			else if (*score < -ISMATE)
				*score += pos->ply;

			assert(*score >= -INFINITE);
			assert(*score <= INFINITE);

			switch (pos->HashTable->pTable[index].flags) {
				case HFALPHA:
					if (*score <= alpha) {
						*score = alpha;
						return TRUE;
					}
					break;
				case HFBETA:
					if (*score >= beta) {
						*score = beta;
						return TRUE;
					}
					break;
				case HFEXACT:
					return TRUE;
					break;
				default:
					assert(FALSE);
					break;
			}
		}
	}

	return FALSE;
}

void StoreHashEntry(S_BOARD *pos, const int move, int score, const int flags, const int depth) {

	int index = pos->posKey % pos->HashTable->numEntries;

	assert(index >= 0 && index <= pos->HashTable->numEntries - 1);
	assert(depth >= 1 && depth < MAXDEPTH);
	assert(flags >= HFALPHA && flags <= HFEXACT);
	assert(score >= -INFINITE && score <= INFINITE);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

#ifdef PV_STATS
	if (pos->HashTable->pTable[index].posKey == 0)
		pos->HashTable->newWrite++;
	else
		pos->HashTable->overWrite++;
#endif

	if (score > ISMATE)
		score += pos->ply;
	else if (score < -ISMATE)
		score -= pos->ply;

	assert(score >= -INFINITE && score <= INFINITE);

	pos->HashTable->pTable[index].posKey = pos->posKey;
	pos->HashTable->pTable[index].move   = move;
	pos->HashTable->pTable[index].depth  = depth;
	pos->HashTable->pTable[index].score  = score;
	pos->HashTable->pTable[index].flags  = flags;
}