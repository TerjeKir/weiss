// makemove.c

#include "defs.h"
#include "stdio.h"

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

static void ClearPiece(const int sq, S_BOARD *pos) {

	ASSERT(SqOnBoard(sq));
	ASSERT(CheckBoard(pos));

	int pce = pos->pieces[sq];

	ASSERT(PieceValid(pce));

	int col = PieceCol[pce];
	int index = 0;
	int t_pceNum = -1;

	ASSERT(SideValid(col));

	HASH_PCE(pce, sq);

	pos->pieces[sq] = EMPTY;
	pos->material[col] -= PieceVal[pce];

	if (PieceBig[pce]) {
		pos->bigPce[col]--;
		if (PieceMaj[pce]) {
			pos->majPce[col]--;
		} else {
			pos->minPce[col]--;
		}
	} else {
		CLRBIT(pos->pawns[col], SQ64(sq));
		CLRBIT(pos->pawns[BOTH], SQ64(sq));
	}

	for (index = 0; index < pos->pceNum[pce]; ++index) {
		if (pos->pList[pce][index] == sq) {
			t_pceNum = index;
			break;
		}
	}

	ASSERT(t_pceNum != -1);
	ASSERT(t_pceNum >= 0 && t_pceNum < 10);

	pos->pceNum[pce]--;

	pos->pList[pce][t_pceNum] = pos->pList[pce][pos->pceNum[pce]];
}

static void AddPiece(const int sq, S_BOARD *pos, const int pce) {

	ASSERT(PieceValid(pce));
	ASSERT(SqOnBoard(sq));

	int col = PieceCol[pce];
	ASSERT(SideValid(col));

	HASH_PCE(pce, sq);

	pos->pieces[sq] = pce;

	if (PieceBig[pce]) {
		pos->bigPce[col]++;
		if (PieceMaj[pce]) {
			pos->majPce[col]++;
		} else {
			pos->minPce[col]++;
		}
	} else {
		SETBIT(pos->pawns[col], SQ64(sq));
		SETBIT(pos->pawns[BOTH], SQ64(sq));
	}

	pos->material[col] += PieceVal[pce];
	pos->pList[pce][pos->pceNum[pce]++] = sq;
}

static void MovePiece(const int from, const int to, S_BOARD *pos) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	int index = 0;
	int pce = pos->pieces[from];
	int col = PieceCol[pce];
	ASSERT(SideValid(col));
	ASSERT(PieceValid(pce));

#ifdef DEBUG
	int t_PieceNum = FALSE;
#endif

	HASH_PCE(pce, from);
	pos->pieces[from] = EMPTY;

	HASH_PCE(pce, to);
	pos->pieces[to] = pce;

	if (!PieceBig[pce]) {
		CLRBIT(pos->pawns[col], SQ64(from));
		CLRBIT(pos->pawns[BOTH], SQ64(from));
		SETBIT(pos->pawns[col], SQ64(to));
		SETBIT(pos->pawns[BOTH], SQ64(to));
	}

	for (index = 0; index < pos->pceNum[pce]; ++index) {
		if (pos->pList[pce][index] == from) {
			pos->pList[pce][index] = to;
#ifdef DEBUG
			t_PieceNum = TRUE;
#endif
			break;
		}
	}
	ASSERT(t_PieceNum);
}

int MakeMove(S_BOARD *pos, int move) {

	ASSERT(CheckBoard(pos));

	int from = FROMSQ(move);
	int to = TOSQ(move);
	int side = pos->side;

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(SideValid(side));
	ASSERT(PieceValid(pos->pieces[from]));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);

	pos->history[pos->hisPly].posKey = pos->posKey;

	if (move & MOVE_FLAG_ENPAS) {
		if (side == WHITE) {
			ClearPiece(to - 10, pos);
		} else {
			ClearPiece(to + 10, pos);
		}
	} else if (move & MOVE_FLAG_CASTLE) {
		switch (to) {
		case C1:
			MovePiece(A1, D1, pos);
			break;
		case C8:
			MovePiece(A8, D8, pos);
			break;
		case G1:
			MovePiece(H1, F1, pos);
			break;
		case G8:
			MovePiece(H8, F8, pos);
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	}

	if (pos->enPas != NO_SQ)
		HASH_EP;
	HASH_CA;

	pos->history[pos->hisPly].move = move;
	pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
	pos->history[pos->hisPly].enPas = pos->enPas;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;

	pos->castlePerm &= CastlePerm[from];
	pos->castlePerm &= CastlePerm[to];
	pos->enPas = NO_SQ;

	HASH_CA;

	int captured = CAPTURED(move);
	pos->fiftyMove++;

	if (captured != EMPTY) {
		ASSERT(PieceValid(captured));
		ClearPiece(to, pos);
		pos->fiftyMove = 0;
	}

	pos->hisPly++;
	pos->ply++;

	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);

	if (PiecePawn[pos->pieces[from]]) {
		pos->fiftyMove = 0;
		if (move & MOVE_FLAG_PAWNSTART) {
			if (side == WHITE) {
				pos->enPas = from + 10;
				ASSERT(RanksBrd[pos->enPas] == RANK_3);
			} else {
				pos->enPas = from - 10;
				ASSERT(RanksBrd[pos->enPas] == RANK_6);
			}
			HASH_EP;
		}
	}

	MovePiece(from, to, pos);

	int prPce = PROMOTED(move);
	if (prPce != EMPTY) {
		ASSERT(PieceValid(prPce) && !PiecePawn[prPce]);
		ClearPiece(to, pos);
		AddPiece(to, pos, prPce);
	}

	if (PieceKing[pos->pieces[to]]) {
		pos->KingSq[pos->side] = to;
	}

	pos->side ^= 1;
	HASH_SIDE;

	ASSERT(CheckBoard(pos));

	if (SqAttacked(pos->KingSq[side], pos->side, pos)) {
		TakeMove(pos);
		return FALSE;
	}

	return TRUE;
}

void TakeMove(S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	pos->hisPly--;
	pos->ply--;

	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);

	int move = pos->history[pos->hisPly].move;
	int from = FROMSQ(move);
	int to = TOSQ(move);

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (pos->enPas != NO_SQ)
		HASH_EP;
	HASH_CA;

	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
	pos->enPas = pos->history[pos->hisPly].enPas;

	if (pos->enPas != NO_SQ)
		HASH_EP;
	HASH_CA;

	pos->side ^= 1;
	HASH_SIDE;

	if (MOVE_FLAG_ENPAS & move) {
		if (pos->side == WHITE) {
			AddPiece(to - 10, pos, bP);
		} else {
			AddPiece(to + 10, pos, wP);
		}
	} else if (move & MOVE_FLAG_CASTLE) {
		switch (to) {
		case C1:
			MovePiece(D1, A1, pos);
			break;
		case C8:
			MovePiece(D8, A8, pos);
			break;
		case G1:
			MovePiece(F1, H1, pos);
			break;
		case G8:
			MovePiece(F8, H8, pos);
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	}

	MovePiece(to, from, pos);

	if (PieceKing[pos->pieces[from]]) {
		pos->KingSq[pos->side] = from;
	}

	int captured = CAPTURED(move);
	if (captured != EMPTY) {
		ASSERT(PieceValid(captured));
		AddPiece(to, pos, captured);
	}

	if (PROMOTED(move) != EMPTY) {
		ASSERT(PieceValid(PROMOTED(move)) && !PiecePawn[PROMOTED(move)]);
		ClearPiece(from, pos);
		AddPiece(from, pos, (PieceCol[PROMOTED(move)] == WHITE ? wP : bP));
	}

	ASSERT(CheckBoard(pos));
}

void MakeNullMove(S_BOARD *pos) {

	ASSERT(CheckBoard(pos));
	ASSERT(!SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos));

	pos->ply++;
	pos->history[pos->hisPly].posKey = pos->posKey;

	if (pos->enPas != NO_SQ)
		HASH_EP;

	pos->history[pos->hisPly].move = NOMOVE;
	pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
	pos->history[pos->hisPly].enPas = pos->enPas;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;
	pos->enPas = NO_SQ;

	pos->side ^= 1;
	pos->hisPly++;
	HASH_SIDE;

	ASSERT(CheckBoard(pos));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);

	return;
} // MakeNullMove

void TakeNullMove(S_BOARD *pos) {
	ASSERT(CheckBoard(pos));

	pos->hisPly--;
	pos->ply--;

	if (pos->enPas != NO_SQ)
		HASH_EP;

	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
	pos->enPas = pos->history[pos->hisPly].enPas;

	if (pos->enPas != NO_SQ)
		HASH_EP;
	pos->side ^= 1;
	HASH_SIDE;

	ASSERT(CheckBoard(pos));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);
}
