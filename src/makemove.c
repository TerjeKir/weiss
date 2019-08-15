// makemove.c

#include <stdio.h>

#include "defs.h"
#include "attack.h"
#include "board.h"
#include "hashkeys.h"
#include "makemove.h"
#include "validate.h"

#define HASH_PCE(pce, sq) (pos->posKey ^= (PieceKeys[(pce)][(sq)]))
#define HASH_CA (pos->posKey ^= (CastleKeys[(pos->castlePerm)]))
#define HASH_SIDE (pos->posKey ^= (SideKey))
#define HASH_EP (pos->posKey ^= (PieceKeys[EMPTY][(pos->enPas)]))

const int CastlePerm[120] = {
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 13, 15, 15, 15, 12, 15, 15, 14, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15,  7, 15, 15, 15,  3, 15, 15, 11, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15};

// Remove a piece from square sq (120)
static void ClearPiece(const int sq, S_BOARD *pos) {

	assert(SqOnBoard(sq));
	assert(CheckBoard(pos));

	int pce = pos->pieces[sq];

	assert(PieceValid(pce));

	int col = PieceCol[pce];
	int index = 0;
	int t_pceNum = -1;

	assert(SideValid(col));

	HASH_PCE(pce, sq);

	pos->pieces[sq] = EMPTY;
	pos->material[col] -= PieceVal[pce];

	// Piece lists
	if (PieceBig[pce]) {
		pos->bigPce[col]--;
		if (PieceMaj[pce])
			pos->majPce[col]--;
		else
			pos->minPce[col]--;
	}

	// Bitboards
	
	CLRBIT(pos->allBB, SQ64(sq));

	switch (pce) {
		case wP:
			CLRBIT(pos->pieceBBs[PAWN], SQ64(sq));
			CLRBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wN:
			CLRBIT(pos->pieceBBs[KNIGHT], SQ64(sq));
			CLRBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wB:
			CLRBIT(pos->pieceBBs[BISHOP], SQ64(sq));
			CLRBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wR:
			CLRBIT(pos->pieceBBs[ROOK], SQ64(sq));
			CLRBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wQ:
			CLRBIT(pos->pieceBBs[QUEEN], SQ64(sq));
			CLRBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wK:
			CLRBIT(pos->pieceBBs[KING], SQ64(sq));
			CLRBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case bP:
			CLRBIT(pos->pieceBBs[PAWN], SQ64(sq));
			CLRBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bN:
			CLRBIT(pos->pieceBBs[KNIGHT], SQ64(sq));
			CLRBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bB:
			CLRBIT(pos->pieceBBs[BISHOP], SQ64(sq));
			CLRBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bR:
			CLRBIT(pos->pieceBBs[ROOK], SQ64(sq));
			CLRBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bQ:
			CLRBIT(pos->pieceBBs[QUEEN], SQ64(sq));
			CLRBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bK:
			CLRBIT(pos->pieceBBs[KING], SQ64(sq));
			CLRBIT(pos->colors[BLACK], SQ64(sq));
			break;
		default:
			assert(FALSE);
			break;
	}

	for (index = 0; index < pos->pceNum[pce]; ++index) {
		if (pos->pList[pce][index] == sq) {
			t_pceNum = index;
			break;
		}
	}

	assert(t_pceNum != -1);
	assert(t_pceNum >= 0 && t_pceNum < 10);

	pos->pceNum[pce]--;

	pos->pList[pce][t_pceNum] = pos->pList[pce][pos->pceNum[pce]];
}
// Add a piece pce to square sq (120)
static void AddPiece(const int sq, S_BOARD *pos, const int pce) {

	assert(PieceValid(pce));
	assert(SqOnBoard(sq));

	int col = PieceCol[pce];
	assert(SideValid(col));

	HASH_PCE(pce, sq);

	pos->pieces[sq] = pce;

	// Piece lists
	if (PieceBig[pce]) {
		pos->bigPce[col]++;
		if (PieceMaj[pce])
			pos->majPce[col]++;
		else
			pos->minPce[col]++;

	}

	// Bitboards
	
	SETBIT(pos->allBB, SQ64(sq));

	switch (pce) {
		case wP:
			SETBIT(pos->pieceBBs[PAWN], SQ64(sq));
			SETBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wN:
			SETBIT(pos->pieceBBs[KNIGHT], SQ64(sq));
			SETBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wB:
			SETBIT(pos->pieceBBs[BISHOP], SQ64(sq));
			SETBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wR:
			SETBIT(pos->pieceBBs[ROOK], SQ64(sq));
			SETBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wQ:
			SETBIT(pos->pieceBBs[QUEEN], SQ64(sq));
			SETBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case wK:
			SETBIT(pos->pieceBBs[KING], SQ64(sq));
			SETBIT(pos->colors[WHITE], SQ64(sq));
			break;
		case bP:
			SETBIT(pos->pieceBBs[PAWN], SQ64(sq));
			SETBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bN:
			SETBIT(pos->pieceBBs[KNIGHT], SQ64(sq));
			SETBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bB:
			SETBIT(pos->pieceBBs[BISHOP], SQ64(sq));
			SETBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bR:
			SETBIT(pos->pieceBBs[ROOK], SQ64(sq));
			SETBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bQ:
			SETBIT(pos->pieceBBs[QUEEN], SQ64(sq));
			SETBIT(pos->colors[BLACK], SQ64(sq));
			break;
		case bK:
			SETBIT(pos->pieceBBs[KING], SQ64(sq));
			SETBIT(pos->colors[BLACK], SQ64(sq));
			break;
		default:
			assert(FALSE);
			break;
	}

	pos->material[col] += PieceVal[pce];
	pos->pList[pce][pos->pceNum[pce]++] = sq;
}
// Move a piece from square from to square to (120)
static void MovePiece(const int from, const int to, S_BOARD *pos) {

	assert(SqOnBoard(from));
	assert(SqOnBoard(to));

	int index = 0;
	int pce = pos->pieces[from];
	assert(PieceValid(pce));

#ifndef NDEBUG
	int t_PieceNum = FALSE;
#endif

	HASH_PCE(pce, from);
	pos->pieces[from] = EMPTY;

	HASH_PCE(pce, to);
	pos->pieces[to] = pce;

	// Bitboards
	
	CLRBIT(pos->allBB, SQ64(from));
	SETBIT(pos->allBB, SQ64(to));

	switch (pce) {
		case wP:
			CLRBIT(pos->pieceBBs[PAWN], SQ64(from));
			CLRBIT(pos->colors[WHITE], SQ64(from));
			SETBIT(pos->pieceBBs[PAWN], SQ64(to));
			SETBIT(pos->colors[WHITE], SQ64(to));
			break;
		case wN:
			CLRBIT(pos->pieceBBs[KNIGHT], SQ64(from));
			CLRBIT(pos->colors[WHITE], SQ64(from));
			SETBIT(pos->pieceBBs[KNIGHT], SQ64(to));
			SETBIT(pos->colors[WHITE], SQ64(to));
			break;
		case wB:
			CLRBIT(pos->pieceBBs[BISHOP], SQ64(from));
			CLRBIT(pos->colors[WHITE], SQ64(from));
			SETBIT(pos->pieceBBs[BISHOP], SQ64(to));
			SETBIT(pos->colors[WHITE], SQ64(to));
			break;
		case wR:
			CLRBIT(pos->pieceBBs[ROOK], SQ64(from));
			CLRBIT(pos->colors[WHITE], SQ64(from));
			SETBIT(pos->pieceBBs[ROOK], SQ64(to));
			SETBIT(pos->colors[WHITE], SQ64(to));
			break;
		case wQ:
			CLRBIT(pos->pieceBBs[QUEEN], SQ64(from));
			CLRBIT(pos->colors[WHITE], SQ64(from));
			SETBIT(pos->pieceBBs[QUEEN], SQ64(to));
			SETBIT(pos->colors[WHITE], SQ64(to));
			break;
		case wK:
			CLRBIT(pos->pieceBBs[KING], SQ64(from));
			CLRBIT(pos->colors[WHITE], SQ64(from));
			SETBIT(pos->pieceBBs[KING], SQ64(to));
			SETBIT(pos->colors[WHITE], SQ64(to));
			break;
		case bP:
			CLRBIT(pos->pieceBBs[PAWN], SQ64(from));
			CLRBIT(pos->colors[BLACK], SQ64(from));
			SETBIT(pos->pieceBBs[PAWN], SQ64(to));
			SETBIT(pos->colors[BLACK], SQ64(to));
			break;
		case bN:
			CLRBIT(pos->pieceBBs[KNIGHT], SQ64(from));
			CLRBIT(pos->colors[BLACK], SQ64(from));
			SETBIT(pos->pieceBBs[KNIGHT], SQ64(to));
			SETBIT(pos->colors[BLACK], SQ64(to));
			break;
		case bB:
			CLRBIT(pos->pieceBBs[BISHOP], SQ64(from));
			CLRBIT(pos->colors[BLACK], SQ64(from));
			SETBIT(pos->pieceBBs[BISHOP], SQ64(to));
			SETBIT(pos->colors[BLACK], SQ64(to));
			break;
		case bR:
			CLRBIT(pos->pieceBBs[ROOK], SQ64(from));
			CLRBIT(pos->colors[BLACK], SQ64(from));
			SETBIT(pos->pieceBBs[ROOK], SQ64(to));
			SETBIT(pos->colors[BLACK], SQ64(to));
			break;
		case bQ:
			CLRBIT(pos->pieceBBs[QUEEN], SQ64(from));
			CLRBIT(pos->colors[BLACK], SQ64(from));
			SETBIT(pos->pieceBBs[QUEEN], SQ64(to));
			SETBIT(pos->colors[BLACK], SQ64(to));
			break;
		case bK:
			CLRBIT(pos->pieceBBs[KING], SQ64(from));
			CLRBIT(pos->colors[BLACK], SQ64(from));
			SETBIT(pos->pieceBBs[KING], SQ64(to));
			SETBIT(pos->colors[BLACK], SQ64(to));
			break;
		default:
			assert(FALSE);
			break;
	}

	for (index = 0; index < pos->pceNum[pce]; ++index) {
		if (pos->pList[pce][index] == from) {
			pos->pList[pce][index] = to;
#ifndef NDEBUG
			t_PieceNum = TRUE;
#endif
			break;
		}
	}
	assert(t_PieceNum);
}

// Make move move
int MakeMove(S_BOARD *pos, int move) {

	assert(CheckBoard(pos));

	int from = FROMSQ(move);
	int to = TOSQ(move);
	int side = pos->side;

	assert(SqOnBoard(from));
	assert(SqOnBoard(to));
	assert(SideValid(side));
	assert(PieceValid(pos->pieces[from]));
	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	// Save posKey in history
	pos->history[pos->hisPly].posKey = pos->posKey;

	// Remove the victim of en passant
	if (move & MOVE_FLAG_ENPAS)
		if (side == WHITE)
			ClearPiece(to - 10, pos);
		else
			ClearPiece(to + 10, pos);

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
				pos->enPas = SQ64(from + 10);
				assert(pos->enPas >= 16 && pos->enPas < 24);
			} else {
				pos->enPas = SQ64(from - 10);
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
	int from = FROMSQ(move);
	int to = TOSQ(move);

	assert(SqOnBoard(from));
	assert(SqOnBoard(to));

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
			AddPiece(to - 10, pos, bP);
		else
			AddPiece(to + 10, pos, wP);
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
		AddPiece(from, pos, (PieceCol[PROMOTED(move)] == WHITE ? wP : bP));
	}

	assert(CheckBoard(pos));
}

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
