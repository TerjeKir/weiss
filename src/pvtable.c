// pvtable.c

#include <stdio.h>
#include <stdlib.h>

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "pvtable.h"


// Returns principal variation move in the position (if any)
static int ProbePvMove(const S_BOARD *pos) {

	int index = pos->posKey % pos->HashTable->numEntries;

	if (pos->HashTable->pTable[index].posKey == pos->posKey)
		return pos->HashTable->pTable[index].move;

	return NOMOVE;
}

// Fills the PvArray of the position with the PV
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

// Clears the hash table
void ClearHashTable(S_HASHTABLE *table) {

	S_HASHENTRY *tableEntry;

	for (tableEntry = table->pTable; tableEntry < table->pTable + table->numEntries; ++tableEntry) {
		tableEntry->posKey = 0ULL;
		tableEntry->move = NOMOVE;
		tableEntry->depth = 0;
		tableEntry->score = 0;
		tableEntry->flag = 0;
	}
#ifdef SEARCH_STATS
	table->newWrite = 0;
#endif
}

// Initializes the hash table
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
#ifdef SEARCH_STATS
		table->newWrite = 0;
#endif
		printf("HashTable init complete with %d entries, using %I64dMB.\n", table->numEntries, MB);
	}
}

// Mate scores are stored as mate in 0 as they depend on the current ply
inline int ScoreToTT (int score, int ply) {
	return score >=  ISMATE ? score + ply
		 : score <= -ISMATE ? score - ply
		 				    : score;
}

// Translates from mate in 0 to the proper mate score at current ply
inline int ScoreFromTT (int score, int ply) {
	return score >=  ISMATE ? score - ply
		 : score <= -ISMATE ? score + ply
		 					: score;
}

// Probe the hash table
int ProbeHashEntry(S_BOARD *pos, int *move, int *score, int alpha, int beta, int depth) {

	assert(alpha < beta);
	assert(alpha >= -INFINITE && alpha <= INFINITE);
	assert(beta  >= -INFINITE && beta  <= INFINITE);
	assert(depth >= 1 && depth < MAXDEPTH);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	int index = pos->posKey % pos->HashTable->numEntries;

	assert(index >= 0 && index < pos->HashTable->numEntries);

	// Look for an entry at the index
	if (pos->HashTable->pTable[index].posKey == pos->posKey) {

		// Use the move as PV regardless of depth
		*move = pos->HashTable->pTable[index].move;

		// The score is only usable if the depth is equal or greater than current
		if (pos->HashTable->pTable[index].depth >= depth) {

			assert(pos->HashTable->pTable[index].depth >= 1 && pos->HashTable->pTable[index].depth < MAXDEPTH);
			assert(pos->HashTable->pTable[index].flag >= HFALPHA && pos->HashTable->pTable[index].flag <= HFEXACT);

			*score = ScoreFromTT(pos->HashTable->pTable[index].score, pos->ply);

			assert(-INFINITE <= *score && *score <= INFINITE);

#ifdef SEARCH_STATS
			pos->HashTable->hit++;
#endif

			// Return true if the score is usable
			switch (pos->HashTable->pTable[index].flag) {
				case HFALPHA:
					if (*score <= alpha) return TRUE;
					break;
				case HFBETA:
					if (*score >= beta) return TRUE;
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

// Store an entry in the hash table
void StoreHashEntry(S_BOARD *pos, const int move, int score, const int flag, const int depth) {

	assert(-INFINITE <= score && score <= INFINITE);
	assert(flag >= HFALPHA && flag <= HFEXACT);
	assert(depth >= 1 && depth < MAXDEPTH);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	int index = pos->posKey % pos->HashTable->numEntries;

	assert(index >= 0 && index < pos->HashTable->numEntries);

	pos->HashTable->pTable[index].posKey = pos->posKey;
	pos->HashTable->pTable[index].move   = move;
	pos->HashTable->pTable[index].depth  = depth;
	pos->HashTable->pTable[index].score  = ScoreToTT(score, pos->ply);
	pos->HashTable->pTable[index].flag   = flag;

	assert(pos->HashTable->pTable[index].score >= -INFINITE);
	assert(pos->HashTable->pTable[index].score <=  INFINITE);

#ifdef SEARCH_STATS
	if (pos->HashTable->pTable[index].posKey == 0)
		pos->HashTable->newWrite++;
	else
		pos->HashTable->overWrite++;
#endif
}