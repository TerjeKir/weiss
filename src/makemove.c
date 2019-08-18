// makemove.c

#include <stdio.h>

#include "defs.h"
#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "hashkeys.h"
#include "makemove.h"
#include "validate.h"


#define HASH_PCE(piece, sq) (pos->posKey ^= (PieceKeys[(piece)][(sq)]))
#define HASH_CA             (pos->posKey ^= (CastleKeys[(pos->castlePerm)]))
#define HASH_SIDE           (pos->posKey ^= (SideKey))
#define HASH_EP             (pos->posKey ^= (PieceKeys[EMPTY][(pos->enPas)]))


const int CastlePerm[64] = {
	13, 15, 15, 15, 12, 15, 15, 14,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	 7, 15, 15, 15,  3, 15, 15, 11
};


// Remove a piece from a square sq
static void ClearPiece(const int sq, S_BOARD *pos) {

	assert(ValidSquare(sq));
	assert(CheckBoard(pos));

	int piece = pos->pieces[sq];

	assert(PieceValid(piece));

	int color = PieceColor[piece];
	int t_pieceCounts = -1;

	assert(SideValid(color));

	HASH_PCE(piece, sq);

	pos->pieces[sq] = EMPTY;
	pos->material[color] -= PieceValues[piece];

	// Piece lists
	if (PieceBig[piece])
		pos->bigPieces[color]--;

	for (int index = 0; index < pos->pieceCounts[piece]; ++index) {
		if (pos->pieceList[piece][index] == sq) {
			t_pieceCounts = index;
			break;
		}
	}

	assert(t_pieceCounts != -1);
	assert(t_pieceCounts >= 0 && t_pieceCounts < 10);

	// Update piece count
	pos->pieceCounts[piece]--;

	pos->pieceList[piece][t_pieceCounts] = pos->pieceList[piece][pos->pieceCounts[piece]];

	// Bitboards
	
	CLRBIT(pos->allBB, sq);

	switch (piece) {
		case wP:
			CLRBIT(pos->pieceBBs[PAWN], sq);
			CLRBIT(pos->colors[WHITE], sq);
			break;
		case wN:
			CLRBIT(pos->pieceBBs[KNIGHT], sq);
			CLRBIT(pos->colors[WHITE], sq);
			break;
		case wB:
			CLRBIT(pos->pieceBBs[BISHOP], sq);
			CLRBIT(pos->colors[WHITE], sq);
			break;
		case wR:
			CLRBIT(pos->pieceBBs[ROOK], sq);
			CLRBIT(pos->colors[WHITE], sq);
			break;
		case wQ:
			CLRBIT(pos->pieceBBs[QUEEN], sq);
			CLRBIT(pos->colors[WHITE], sq);
			break;
		case wK:
			CLRBIT(pos->pieceBBs[KING], sq);
			CLRBIT(pos->colors[WHITE], sq);
			break;
		case bP:
			CLRBIT(pos->pieceBBs[PAWN], sq);
			CLRBIT(pos->colors[BLACK], sq);
			break;
		case bN:
			CLRBIT(pos->pieceBBs[KNIGHT], sq);
			CLRBIT(pos->colors[BLACK], sq);
			break;
		case bB:
			CLRBIT(pos->pieceBBs[BISHOP], sq);
			CLRBIT(pos->colors[BLACK], sq);
			break;
		case bR:
			CLRBIT(pos->pieceBBs[ROOK], sq);
			CLRBIT(pos->colors[BLACK], sq);
			break;
		case bQ:
			CLRBIT(pos->pieceBBs[QUEEN], sq);
			CLRBIT(pos->colors[BLACK], sq);
			break;
		case bK:
			CLRBIT(pos->pieceBBs[KING], sq);
			CLRBIT(pos->colors[BLACK], sq);
			break;
		default:
			assert(FALSE);
			break;
	}
}

// Add a piece piece to a square
static void AddPiece(const int sq, S_BOARD *pos, const int piece) {

	assert(PieceValid(piece));
	assert(ValidSquare(sq));

	int color = PieceColor[piece];
	assert(SideValid(color));

	HASH_PCE(piece, sq);

	pos->pieces[sq] = piece;

	// Piece lists
	if (PieceBig[piece])
		pos->bigPieces[color]++;

	pos->material[color] += PieceValues[piece];
	pos->pieceList[piece][pos->pieceCounts[piece]++] = sq;

	// Bitboards
	
	SETBIT(pos->allBB, sq);

	switch (piece) {
		case wP:
			SETBIT(pos->pieceBBs[PAWN], sq);
			SETBIT(pos->colors[WHITE], sq);
			break;
		case wN:
			SETBIT(pos->pieceBBs[KNIGHT], sq);
			SETBIT(pos->colors[WHITE], sq);
			break;
		case wB:
			SETBIT(pos->pieceBBs[BISHOP], sq);
			SETBIT(pos->colors[WHITE], sq);
			break;
		case wR:
			SETBIT(pos->pieceBBs[ROOK], sq);
			SETBIT(pos->colors[WHITE], sq);
			break;
		case wQ:
			SETBIT(pos->pieceBBs[QUEEN], sq);
			SETBIT(pos->colors[WHITE], sq);
			break;
		case wK:
			SETBIT(pos->pieceBBs[KING], sq);
			SETBIT(pos->colors[WHITE], sq);
			break;
		case bP:
			SETBIT(pos->pieceBBs[PAWN], sq);
			SETBIT(pos->colors[BLACK], sq);
			break;
		case bN:
			SETBIT(pos->pieceBBs[KNIGHT], sq);
			SETBIT(pos->colors[BLACK], sq);
			break;
		case bB:
			SETBIT(pos->pieceBBs[BISHOP], sq);
			SETBIT(pos->colors[BLACK], sq);
			break;
		case bR:
			SETBIT(pos->pieceBBs[ROOK], sq);
			SETBIT(pos->colors[BLACK], sq);
			break;
		case bQ:
			SETBIT(pos->pieceBBs[QUEEN], sq);
			SETBIT(pos->colors[BLACK], sq);
			break;
		case bK:
			SETBIT(pos->pieceBBs[KING], sq);
			SETBIT(pos->colors[BLACK], sq);
			break;
		default:
			assert(FALSE);
			break;
	}
}
// Move a piece from a square to another square
static void MovePiece(const int from, const int to, S_BOARD *pos) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));

	int index = 0;
	int piece = pos->pieces[from];
	assert(PieceValid(piece));

#ifndef NDEBUG
	int t_PieceNum = FALSE;
#endif

	HASH_PCE(piece, from);
	pos->pieces[from] = EMPTY;

	HASH_PCE(piece, to);
	pos->pieces[to] = piece;

	for (index = 0; index < pos->pieceCounts[piece]; ++index) {
		if (pos->pieceList[piece][index] == from) {
			pos->pieceList[piece][index] = to;
#ifndef NDEBUG
			t_PieceNum = TRUE;
#endif
			break;
		}
	}
	assert(t_PieceNum);

	// Bitboards
	
	CLRBIT(pos->allBB, from);
	SETBIT(pos->allBB, to);

	switch (piece) {
		case wP:
			CLRBIT(pos->pieceBBs[PAWN], from);
			CLRBIT(pos->colors[WHITE], from);
			SETBIT(pos->pieceBBs[PAWN], to);
			SETBIT(pos->colors[WHITE], to);
			break;
		case wN:
			CLRBIT(pos->pieceBBs[KNIGHT], from);
			CLRBIT(pos->colors[WHITE], from);
			SETBIT(pos->pieceBBs[KNIGHT], to);
			SETBIT(pos->colors[WHITE], to);
			break;
		case wB:
			CLRBIT(pos->pieceBBs[BISHOP], from);
			CLRBIT(pos->colors[WHITE], from);
			SETBIT(pos->pieceBBs[BISHOP], to);
			SETBIT(pos->colors[WHITE], to);
			break;
		case wR:
			CLRBIT(pos->pieceBBs[ROOK], from);
			CLRBIT(pos->colors[WHITE], from);
			SETBIT(pos->pieceBBs[ROOK], to);
			SETBIT(pos->colors[WHITE], to);
			break;
		case wQ:
			CLRBIT(pos->pieceBBs[QUEEN], from);
			CLRBIT(pos->colors[WHITE], from);
			SETBIT(pos->pieceBBs[QUEEN], to);
			SETBIT(pos->colors[WHITE], to);
			break;
		case wK:
			CLRBIT(pos->pieceBBs[KING], from);
			CLRBIT(pos->colors[WHITE], from);
			SETBIT(pos->pieceBBs[KING], to);
			SETBIT(pos->colors[WHITE], to);
			break;
		case bP:
			CLRBIT(pos->pieceBBs[PAWN], from);
			CLRBIT(pos->colors[BLACK], from);
			SETBIT(pos->pieceBBs[PAWN], to);
			SETBIT(pos->colors[BLACK], to);
			break;
		case bN:
			CLRBIT(pos->pieceBBs[KNIGHT], from);
			CLRBIT(pos->colors[BLACK], from);
			SETBIT(pos->pieceBBs[KNIGHT], to);
			SETBIT(pos->colors[BLACK], to);
			break;
		case bB:
			CLRBIT(pos->pieceBBs[BISHOP], from);
			CLRBIT(pos->colors[BLACK], from);
			SETBIT(pos->pieceBBs[BISHOP], to);
			SETBIT(pos->colors[BLACK], to);
			break;
		case bR:
			CLRBIT(pos->pieceBBs[ROOK], from);
			CLRBIT(pos->colors[BLACK], from);
			SETBIT(pos->pieceBBs[ROOK], to);
			SETBIT(pos->colors[BLACK], to);
			break;
		case bQ:
			CLRBIT(pos->pieceBBs[QUEEN], from);
			CLRBIT(pos->colors[BLACK], from);
			SETBIT(pos->pieceBBs[QUEEN], to);
			SETBIT(pos->colors[BLACK], to);
			break;
		case bK:
			CLRBIT(pos->pieceBBs[KING], from);
			CLRBIT(pos->colors[BLACK], from);
			SETBIT(pos->pieceBBs[KING], to);
			SETBIT(pos->colors[BLACK], to);
			break;
		default:
			assert(FALSE);
			break;
	}
}

// Make move move
int MakeMove(S_BOARD *pos, int move) {

	assert(CheckBoard(pos));

	int from = SQ64(FROMSQ(move));
	int to = SQ64(TOSQ(move));
	int side = pos->side;

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(SideValid(side));
	assert(PieceValid(pos->pieces[from]));
	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	// Save posKey in history
	pos->history[pos->hisPly].posKey = pos->posKey;

	// Remove the victim of en passant
	if (move & MOVE_FLAG_ENPAS)
		if (side == WHITE)
			ClearPiece(to - 8, pos);
		else
			ClearPiece(to + 8, pos);

	// Move the rook during castling
	else if (move & MOVE_FLAG_CASTLE)
		switch (to) {
			case C1: MovePiece(A1, D1, pos); break;
			case C8: MovePiece(A8, D8, pos); break;
			case G1: MovePiece(H1, F1, pos); break;
			case G8: MovePiece(H8, F8, pos); break;
			default: assert(FALSE); break;
		}

	// Hash out the old en passant if exist
	if (pos->enPas != NO_SQ) HASH_EP;

	// Hash out the old castling rights
	HASH_CA;

	// Save move, 50mr, en passant, and castling rights in history
	pos->history[pos->hisPly].move = move;
	pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
	pos->history[pos->hisPly].enPas = pos->enPas;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;

	// Update castling rights and unset passant square
	pos->castlePerm &= CastlePerm[from];
	pos->castlePerm &= CastlePerm[to];
	pos->enPas = NO_SQ;

	// Hash in the castling rights again
	HASH_CA;

	// Remove captured piece if any
	int captured = CAPTURED(move);
	
	if (captured != EMPTY) {
		assert(PieceValid(captured));
		ClearPiece(to, pos);
		pos->fiftyMove = 0;
	}

	// Increment hisPly, ply and 50mr
	pos->hisPly++;
	pos->ply++;
	pos->fiftyMove++;

	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	// Set en passant square and hash it in if any
	if (PiecePawn[pos->pieces[from]]) {
		pos->fiftyMove = 0;
		if (move & MOVE_FLAG_PAWNSTART) {
			if (side == WHITE) {
				pos->enPas = from + 8;
				assert(pos->enPas >= 16 && pos->enPas < 24);
			} else {
				pos->enPas = from - 8;
				assert(pos->enPas >= 40 && pos->enPas < 48);
			}
			HASH_EP;
		}
	}

	// Move the piece
	MovePiece(from, to, pos);

	// Replace promoting pawn with new piece
	int prPce = PROMOTED(move);
	if (prPce != EMPTY) {
		assert(PieceValid(prPce) && !PiecePawn[prPce]);
		ClearPiece(to, pos);
		AddPiece(to, pos, prPce);
	}

	// Update king position if king moved
	if (PieceKing[pos->pieces[to]])
		pos->KingSq[pos->side] = to;

	// Change turn to play
	pos->side ^= 1;
	HASH_SIDE;

	assert(CheckBoard(pos));

	// If own king is attacked after the move, take it back immediately
	if (SqAttacked(pos->KingSq[side], pos->side, pos)) {
		TakeMove(pos);
		return FALSE;
	}

	return TRUE;
}

void TakeMove(S_BOARD *pos) {

	assert(CheckBoard(pos));

	// Decrement hisPly, ply
	pos->hisPly--;
	pos->ply--;

	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	int move = pos->history[pos->hisPly].move;
	int from = SQ64(FROMSQ(move));
	int to = SQ64(TOSQ(move));

	assert(ValidSquare(from));
	assert(ValidSquare(to));

	// Hash out en passant if exists
	if (pos->enPas != NO_SQ) HASH_EP;

	// Hash out castling rights
	HASH_CA;

	// Update castling rights, 50mr, en passant
	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
	pos->enPas = pos->history[pos->hisPly].enPas;

	// Hash in en passant if exists
	if (pos->enPas != NO_SQ) HASH_EP;
	// Hash in castling rights
	HASH_CA;

	// Change side to play and hash it in
	pos->side ^= 1;
	HASH_SIDE;

	// Add in pawn capture by en passant
	if (MOVE_FLAG_ENPAS & move)
		if (pos->side == WHITE)
			AddPiece(to - 8, pos, bP);
		else
			AddPiece(to + 8, pos, wP);
	// Move rook back if castling
	else if (move & MOVE_FLAG_CASTLE)
		switch (to) {
			case C1: MovePiece(D1, A1, pos); break;
			case C8: MovePiece(D8, A8, pos); break;
			case G1: MovePiece(F1, H1, pos); break;
			case G8: MovePiece(F8, H8, pos); break;
			default: assert(FALSE); break;
		}

	// Make reverse move (from <-> to)
	MovePiece(to, from, pos);

	// Update king position if king moved
	if (PieceKing[pos->pieces[from]])
		pos->KingSq[pos->side] = from;

	// Add back captured piece if any
	int captured = CAPTURED(move);
	if (captured != EMPTY) {
		assert(PieceValid(captured));
		AddPiece(to, pos, captured);
	}

	// Remove promoted piece and put back the pawn
	if (PROMOTED(move) != EMPTY) {
		assert(PieceValid(PROMOTED(move)) && !PiecePawn[PROMOTED(move)]);
		ClearPiece(from, pos);
		AddPiece(from, pos, (PieceColor[PROMOTED(move)] == WHITE ? wP : bP));
	}

	assert(CheckBoard(pos));
}

// Pass the turn without moving
void MakeNullMove(S_BOARD *pos) {

	assert(CheckBoard(pos));
	assert(!SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos));

	pos->ply++;
	pos->history[pos->hisPly].posKey = pos->posKey;

	// Hash out en passant if there was one, and unset it
	if (pos->enPas != NO_SQ) {
		HASH_EP;
		pos->enPas = NO_SQ;
	}

	// Save misc info for takeback
	pos->history[pos->hisPly].move = NOMOVE;
	pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
	pos->history[pos->hisPly].enPas = pos->enPas;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;
	pos->hisPly++;

	// Unset en passant square

	// Change side to play
	pos->side ^= 1;
	HASH_SIDE;

	assert(CheckBoard(pos));
	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	return;
}

// Take back passing the turn
void TakeNullMove(S_BOARD *pos) {
	assert(CheckBoard(pos));

	pos->hisPly--;
	pos->ply--;

	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
	pos->enPas = pos->history[pos->hisPly].enPas;

	if (pos->enPas != NO_SQ) HASH_EP;

	pos->side ^= 1;
	HASH_SIDE;

	assert(CheckBoard(pos));
	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);
}
