// makemove.c

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "hashkeys.h"
#include "move.h"
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

	CLRBIT(pos->colors[PieceColor[piece]], sq);

	CLRBIT(pos->pieceBBs[pieceType[piece]], sq);
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

	SETBIT(pos->colors[PieceColor[piece]], sq);

	SETBIT(pos->pieceBBs[pieceType[piece]], sq);
}

// Move a piece from a square to another square
static void MovePiece(const int from, const int to, S_BOARD *pos) {

	assert(ValidSquare(from));
	assert(ValidSquare(to));

	int index = 0;
	int piece = pos->pieces[from];
	assert(PieceValid(piece));

#ifndef NDEBUG
	int t_PieceNum = false;
#endif

	HASH_PCE(piece, from);
	pos->pieces[from] = EMPTY;

	HASH_PCE(piece, to);
	pos->pieces[to] = piece;

	for (index = 0; index < pos->pieceCounts[piece]; ++index) {
		if (pos->pieceList[piece][index] == from) {
			pos->pieceList[piece][index] = to;
#ifndef NDEBUG
			t_PieceNum = true;
#endif
			break;
		}
	}
	assert(t_PieceNum);

	// Bitboards

	CLRBIT(pos->allBB, from);
	SETBIT(pos->allBB, to);

	CLRBIT(pos->colors[PieceColor[piece]], from);
	SETBIT(pos->colors[PieceColor[piece]], to);

	CLRBIT(pos->pieceBBs[pieceType[piece]], from);
	SETBIT(pos->pieceBBs[pieceType[piece]], to);
}

// Take back the previous move
void TakeMove(S_BOARD *pos) {
	assert(CheckBoard(pos));

	// Decrement hisPly, ply
	pos->hisPly--;
	pos->ply--;

	// Change side to play
	pos->side ^= 1;

	// Update castling rights, 50mr, en passant
	pos->enPas 		= pos->history[pos->hisPly].enPas;
	pos->fiftyMove 	= pos->history[pos->hisPly].fiftyMove;
	pos->castlePerm = pos->history[pos->hisPly].castlePerm;

	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	// Get the move from history
	int move = pos->history[pos->hisPly].move;
	int from = FROMSQ(move);
	int   to =   TOSQ(move);

	assert(ValidSquare(from));
	assert(ValidSquare(to));

	// Add in pawn captured by en passant
	if (FLAG_ENPAS & move)
		if (pos->side == WHITE)
			AddPiece(to - 8, pos, bP);
		else
			AddPiece(to + 8, pos, wP);

	// Move rook back if castling
	else if (move & FLAG_CASTLE)
		switch (to) {
			case C1: MovePiece(D1, A1, pos); break;
			case C8: MovePiece(D8, A8, pos); break;
			case G1: MovePiece(F1, H1, pos); break;
			case G8: MovePiece(F8, H8, pos); break;
			default: assert(false); break;
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
	if (PROMOTION(move) != EMPTY) {
		assert(PieceValid(PROMOTION(move)) && !PiecePawn[PROMOTION(move)]);
		ClearPiece(from, pos);
		AddPiece(from, pos, (PieceColor[PROMOTION(move)] == WHITE ? wP : bP));
	}

	// Get old poskey from history
	pos->posKey = pos->history[pos->hisPly].posKey;

	assert(CheckBoard(pos));
}

// Make a move, advancing the board to the next state
int MakeMove(S_BOARD *pos, const int move) {

	assert(CheckBoard(pos));

	int from 	 = FROMSQ(move);
	int to		 = TOSQ(move);
	int captured = CAPTURED(move);

	int side = pos->side;

	assert(ValidSquare(from));
	assert(ValidSquare(to));
	assert(SideValid(side));
	assert(PieceValid(pos->pieces[from]));
	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	// Save position 
	pos->history[pos->hisPly].move 		 = move;
	pos->history[pos->hisPly].enPas 	 = pos->enPas;
	pos->history[pos->hisPly].fiftyMove  = pos->fiftyMove;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;
	pos->history[pos->hisPly].posKey 	 = pos->posKey;

	// Increment hisPly, ply and 50mr
	pos->hisPly++;
	pos->ply++;
	pos->fiftyMove++;

	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	// Hash out the old en passant if exist and unset it
	if (pos->enPas != NO_SQ) {
		HASH_EP;
		pos->enPas = NO_SQ;
	}

	// Hash out the old castling rights, update and hash back in
	HASH_CA;
	pos->castlePerm &= CastlePerm[from];
	pos->castlePerm &= CastlePerm[to];
	HASH_CA;

	// Move the rook during castling
	if (move & FLAG_CASTLE)
		switch (to) {
			case C1: MovePiece(A1, D1, pos); break;
			case C8: MovePiece(A8, D8, pos); break;
			case G1: MovePiece(H1, F1, pos); break;
			case G8: MovePiece(H8, F8, pos); break;
			default: assert(false); break;
		}

	// Remove captured piece if any
	else if (captured != EMPTY) {
		assert(PieceValid(captured));
		ClearPiece(to, pos);

		// Reset 50mr after a capture
		pos->fiftyMove = 0;
	}

	// Move the piece
	MovePiece(from, to, pos);

	// Pawn move specifics
	if (PiecePawn[pos->pieces[to]]) {

		// Reset 50mr after a pawn move
		pos->fiftyMove = 0;
		
		int promo = PROMOTION(move);

		// If the move is a pawnstart we set the en passant square and hash it in
		if (move & FLAG_PAWNSTART) {
			pos->enPas = to + (side ? -8 : 8);
			assert((rankOf(pos->enPas) == RANK_3 && side) || rankOf(pos->enPas) == RANK_6);
			HASH_EP;

		// Remove pawn captured by en passant
		} else if (move & FLAG_ENPAS)
			ClearPiece(to + (side ? -8 : 8), pos);

		// Replace promoting pawn with new piece
		else if (promo != EMPTY) {
			assert(PieceValid(promo) && !PiecePawn[promo]);
			ClearPiece(to, pos);
			AddPiece(to, pos, promo);
		}

	// Update king position if king moved
	} else if (PieceKing[pos->pieces[to]])
		pos->KingSq[side] = to;

	// Change turn to play
	pos->side ^= 1;
	HASH_SIDE;

	assert(CheckBoard(pos));

	// If own king is attacked after the move, take it back immediately
	if (SqAttacked(pos->KingSq[side], pos->side, pos)) {
		TakeMove(pos);
		return false;
	}

	return true;
}

// Pass the turn without moving
void MakeNullMove(S_BOARD *pos) {

	assert(CheckBoard(pos));
	assert(!SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos));

	// Save misc info for takeback
	// pos->history[pos->hisPly].move    = NOMOVE;
	pos->history[pos->hisPly].enPas 	 = pos->enPas;
	pos->history[pos->hisPly].fiftyMove  = pos->fiftyMove;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;
	pos->history[pos->hisPly].posKey 	 = pos->posKey;

	// Increase ply
	pos->ply++;
	pos->hisPly++;

	// Change side to play
	pos->side ^= 1;
	HASH_SIDE;

	// Hash out en passant if there was one, and unset it
	if (pos->enPas != NO_SQ) {
		HASH_EP;
		pos->enPas = NO_SQ;
	}

	assert(CheckBoard(pos));
	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

	return;
}

// Take back a null move
void TakeNullMove(S_BOARD *pos) {
	assert(CheckBoard(pos));

	// Decrease ply
	pos->hisPly--;
	pos->ply--;

	// Change side to play
	pos->side ^= 1;

	// Get info from history
	pos->enPas 		= pos->history[pos->hisPly].enPas;
	pos->fiftyMove 	= pos->history[pos->hisPly].fiftyMove;
	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->posKey 	= pos->history[pos->hisPly].posKey;

	assert(CheckBoard(pos));
	assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	assert(pos->ply >= 0 && pos->ply < MAXDEPTH);
}