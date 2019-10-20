// hashkeys.c

#include "types.h"

uint64_t rand64() {

    // http://vigna.di.unimi.it/ftp/papers/xorshift.pdf

    static uint64_t seed = 1070372ull;

    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;

    return seed * 2685821657736338717ull;
}

// Zobrist key tables
uint64_t PieceKeys[PIECE_NB][64];
uint64_t CastleKeys[16];
uint64_t SideKey;


// Inits zobrist key tables
static void InitHashKeys() __attribute__((constructor));
static void InitHashKeys() {

	// Side to play
	SideKey = rand64();

	// En passant
	for (int sq = A1; sq <= H8; ++sq)
		PieceKeys[0][sq] = rand64();

	// White pieces
	for (int piece = wP; piece <= wK; ++piece)
		for (int sq = A1; sq <= H8; ++sq)
			PieceKeys[piece][sq] = rand64();

	// Black pieces
	for (int piece = bP; piece <= bK; ++piece)
		for (int sq = A1; sq <= H8; ++sq)
			PieceKeys[piece][sq] = rand64();

	// Castling rights
	for (int i = 0; i < 16; ++i)
		CastleKeys[i] = rand64();
}

uint64_t GeneratePosKey(const Position *pos) {

	uint64_t posKey = 0;

	// Pieces
	for (int sq = A1; sq <= H8; ++sq) {
		int piece = pos->board[sq];
		if (piece != EMPTY)
			posKey ^= PieceKeys[piece][sq];
	}

	// Side to play
	if (pos->side == WHITE)
		posKey ^= SideKey;

	// En passant
	if (pos->enPas != NO_SQ)
		posKey ^= PieceKeys[EMPTY][pos->enPas];

	// Castling rights
	posKey ^= CastleKeys[pos->castlePerm];

	return posKey;
}