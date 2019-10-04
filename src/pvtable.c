// pvtable.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "pvtable.h"


// Returns principal variation move in the position (if any)
static int ProbePvMove(const S_BOARD *pos) {

	int index = pos->posKey % pos->hashTable->numEntries;

	if (pos->hashTable->pTable[index].posKey == pos->posKey)
		return pos->hashTable->pTable[index].move;

	return NOMOVE;
}

// Fills the pvArray of the position with the PV
int GetPvLine(const int depth, S_BOARD *pos) {

	assert(depth < MAXDEPTH && depth >= 1);

	int move = ProbePvMove(pos);
	int count = 0;

	while (move != NOMOVE && count < depth) {

		assert(count < MAXDEPTH);

		if (MoveExists(pos, move)) {
			MakeMove(pos, move);
			pos->pvArray[count++] = move;
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

	memset(table->pTable, 0, table->numEntries * sizeof(S_HASHENTRY));

#ifdef SEARCH_STATS
	table->newWrite = 0;
#endif
}

// Initializes the hash table
int InitHashTable(S_HASHTABLE *table, uint64_t MB) {

	// Ignore if already initialized with this size
	if (table->MB == MB) {
		printf("HashTable already initialized to %I64u.\n", MB);
		return MB;
	}

	uint64_t HashSize = 0x100000LL * MB;
	table->numEntries = HashSize / sizeof(S_HASHENTRY);

	MB = MB < MINHASH ? MINHASH : MB; // Don't go under minhash
	MB = MB > MAXHASH ? MAXHASH : MB; // Don't go over maxhash

	// Free memory if we have already allocated
	if (table->pTable != NULL)
		free(table->pTable);

	// Allocate memory
	table->pTable = (S_HASHENTRY *)calloc(table->numEntries, sizeof(S_HASHENTRY));

	// If allocation fails, try half the size
	if (table->pTable == NULL) {
		printf("Hash Allocation Failed, trying %I64uMB...\n", MB / 2);
		return InitHashTable(table, MB / 2);

	// Success
	} else {
#ifdef SEARCH_STATS
		table->newWrite = 0;
#endif
		table->MB = MB;
		printf("HashTable init complete with %d entries, using %I64uMB.\n", table->numEntries, MB);
		return MB;
	}
}

// Mate scores are stored as mate in 0 as they depend on the current ply
static inline int ScoreToTT (int score, const int ply) {
	return score >=  ISMATE ? score + ply
		 : score <= -ISMATE ? score - ply
		 				    : score;
}

// Translates from mate in 0 to the proper mate score at current ply
static inline int ScoreFromTT (int score, const int ply) {
	return score >=  ISMATE ? score - ply
		 : score <= -ISMATE ? score + ply
		 					: score;
}

// Probe the hash table
#ifdef SEARCH_STATS
int ProbeHashEntry(S_BOARD *pos, int *move, int *score, const int alpha, const int beta, const int depth) {
#else
int ProbeHashEntry(const S_BOARD *pos, int *move, int *score, const int alpha, const int beta, const int depth) {
#endif

	assert(alpha < beta);
	assert(alpha >= -INFINITE && alpha <= INFINITE);
	assert(beta  >= -INFINITE && beta  <= INFINITE);
	assert(depth >= 1 && depth < MAXDEPTH);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	int index = pos->posKey % pos->hashTable->numEntries;

	assert(index >= 0 && index < pos->hashTable->numEntries);

	// Look for an entry at the index
	if (pos->hashTable->pTable[index].posKey == pos->posKey) {

		// Use the move as PV regardless of depth
		*move = pos->hashTable->pTable[index].move;

		// The score is only usable if the depth is equal or greater than current
		if (pos->hashTable->pTable[index].depth >= depth) {

			assert(pos->hashTable->pTable[index].depth >= 1 && pos->hashTable->pTable[index].depth < MAXDEPTH);
			assert(pos->hashTable->pTable[index].flag >= BOUND_UPPER && pos->hashTable->pTable[index].flag <= BOUND_EXACT);

			*score = ScoreFromTT(pos->hashTable->pTable[index].score, pos->ply);

			assert(-INFINITE <= *score && *score <= INFINITE);

#ifdef SEARCH_STATS
			pos->hashTable->hit++;
#endif

			// Return true if the score is usable
			uint8_t flag = pos->hashTable->pTable[index].flag;
			if (   (flag == BOUND_UPPER && *score <= alpha)
				|| (flag == BOUND_LOWER && *score >= beta)
				||  flag == BOUND_EXACT)
				return true;
		}
	}
	return false;
}

// Store an entry in the hash table
void StoreHashEntry(S_BOARD *pos, const int move, const int score, const int flag, const int depth) {

	assert(-INFINITE <= score && score <= INFINITE);
	assert(flag >= BOUND_UPPER && flag <= BOUND_EXACT);
	assert(depth >= 1 && depth < MAXDEPTH);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	int index = pos->posKey % pos->hashTable->numEntries;

	assert(index >= 0 && index < pos->hashTable->numEntries);

#ifdef SEARCH_STATS
	if (pos->hashTable->pTable[index].posKey == 0)
		pos->hashTable->newWrite++;
	else
		pos->hashTable->overWrite++;
#endif

	pos->hashTable->pTable[index].posKey = pos->posKey;
	pos->hashTable->pTable[index].move   = move;
	pos->hashTable->pTable[index].score  = ScoreToTT(score, pos->ply);
	pos->hashTable->pTable[index].depth  = depth;
	pos->hashTable->pTable[index].flag   = flag;

	assert(pos->hashTable->pTable[index].score >= -INFINITE);
	assert(pos->hashTable->pTable[index].score <=  INFINITE);
}

// Estimates the load factor of the transposition table (1 = 0.1%)
int HashFull(const S_BOARD *pos) {

	uint64_t used = 0;
	int samples = 1000;

	for (int i = 0; i < samples; ++i)
		if (pos->hashTable->pTable[i].move != NOMOVE)
			used++;

	return used / (samples / 1000);
}