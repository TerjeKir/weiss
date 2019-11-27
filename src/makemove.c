// makemove.c

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "psqt.h"
#include "hashkeys.h"
#include "move.h"
#include "validate.h"


#define HASH_PCE(piece, sq) (pos->posKey ^= (PieceKeys[(piece)][(sq)]))
#define HASH_CA             (pos->posKey ^= (CastleKeys[(pos->castlePerm)]))
#define HASH_SIDE           (pos->posKey ^= (SideKey))
#define HASH_EP             (pos->posKey ^= (PieceKeys[EMPTY][(pos->enPas)]))


static const int CastlePerm[64] = {
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
static void ClearPiece(const int sq, Position *pos) {

    assert(ValidSquare(sq));

    const int piece = pos->board[sq];
    const int color = colorOf(piece);
    int t_pieceCounts = -1;

    assert(ValidPiece(piece));
    assert(ValidSide(color));

    // Hash out the piece
    HASH_PCE(piece, sq);

    // Set square to empty
    pos->board[sq] = EMPTY;

    // Update material
    pos->material -= PSQT[piece][sq];

    // Update phase
    pos->basePhase += phaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update various piece lists
    if (pieceBig[piece])
        pos->bigPieces[color]--;

    for (int i = 0; i < pos->pieceCounts[piece]; ++i)
        if (pos->pieceList[piece][i] == sq) {
            t_pieceCounts = i;
            break;
        }

    assert(t_pieceCounts != -1);
    assert(t_pieceCounts >= 0 && t_pieceCounts < 10);

    // Update piece count
    pos->pieceList[piece][t_pieceCounts] = pos->pieceList[piece][--pos->pieceCounts[piece]];

    // Update bitboards
    CLRBIT(pos->pieceBBs[ALL], sq);
    CLRBIT(pos->colorBBs[color], sq);
    CLRBIT(pos->pieceBBs[pieceTypeOf(piece)], sq);
}

// Add a piece piece to a square
static void AddPiece(const int sq, Position *pos, const int piece) {

    assert(ValidPiece(piece));
    assert(ValidSquare(sq));

    const int color = colorOf(piece);
    assert(ValidSide(color));

    // Hash in piece at square
    HASH_PCE(piece, sq);

    // Update square
    pos->board[sq] = piece;

    // Update material
    pos->material += PSQT[piece][sq];

    // Update phase
    pos->basePhase -= phaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update various piece lists
    if (pieceBig[piece])
        pos->bigPieces[color]++;

    pos->pieceList[piece][pos->pieceCounts[piece]++] = sq;

    // Update bitboards
    SETBIT(pos->pieceBBs[ALL], sq);
    SETBIT(pos->colorBBs[color], sq);
    SETBIT(pos->pieceBBs[pieceTypeOf(piece)], sq);
}

// Move a piece from one square to another
static void MovePiece(const int from, const int to, Position *pos) {
#ifndef NDEBUG
    int t_PieceNum = false;
#endif

    assert(ValidSquare(from));
    assert(ValidSquare(to));

    const int piece = pos->board[from];

    assert(ValidPiece(piece));

    // Hash out piece on old square, in on new square
    HASH_PCE(piece, from);
    HASH_PCE(piece, to);

    // Set old square to empty, new to piece
    pos->board[from] = EMPTY;
    pos->board[to]   = piece;

    // Update square for the piece in pieceList
    for (int i = 0; i < pos->pieceCounts[piece]; ++i)
        if (pos->pieceList[piece][i] == from) {
            pos->pieceList[piece][i] = to;
#ifndef NDEBUG
            t_PieceNum = true;
#endif
            break;
        }
    assert(t_PieceNum);

    // Update material
    pos->material += PSQT[piece][to] - PSQT[piece][from];

    // Update bitboards
    CLRBIT(pos->pieceBBs[ALL], from);
    SETBIT(pos->pieceBBs[ALL], to);

    CLRBIT(pos->colorBBs[colorOf(piece)], from);
    SETBIT(pos->colorBBs[colorOf(piece)], to);

    CLRBIT(pos->pieceBBs[pieceTypeOf(piece)], from);
    SETBIT(pos->pieceBBs[pieceTypeOf(piece)], to);
}

// Take back the previous move
void TakeMove(Position *pos) {

    assert(CheckBoard(pos));

    // Decrement hisPly, ply
    pos->hisPly--;
    pos->ply--;

    // Change side to play
    pos->side ^= 1;

    // Update castling rights, 50mr, en passant
    pos->enPas      = pos->history[pos->hisPly].enPas;
    pos->fiftyMove  = pos->history[pos->hisPly].fiftyMove;
    pos->castlePerm = pos->history[pos->hisPly].castlePerm;

    assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

    // Get the move from history
    const int move = pos->history[pos->hisPly].move;
    const int from = FROMSQ(move);
    const int   to =   TOSQ(move);

    assert(ValidSquare(from));
    assert(ValidSquare(to));

    // Add in pawn captured by en passant
    if (FLAG_ENPAS & move)
        AddPiece(to + 8 - 16 * pos->side, pos, makePiece(!pos->side, PAWN));

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
    if (pieceKing[pos->board[from]])
        pos->kingSq[pos->side] = from;

    // Add back captured piece if any
    int captured = CAPTURED(move);
    if (captured != EMPTY) {
        assert(ValidPiece(captured));
        AddPiece(to, pos, captured);
    }

    // Remove promoted piece and put back the pawn
    if (PROMOTION(move) != EMPTY) {
        assert(ValidPiece(PROMOTION(move)) && !piecePawn[PROMOTION(move)]);
        ClearPiece(from, pos);
        AddPiece(from, pos, makePiece(colorOf(PROMOTION(move)), PAWN));
    }

    // Get old poskey from history
    pos->posKey = pos->history[pos->hisPly].posKey;

    assert(CheckBoard(pos));
}

// Make a move - take it back and return false if move was illegal
bool MakeMove(Position *pos, const int move) {

    assert(CheckBoard(pos));

    const int from     = FROMSQ(move);
    const int to       = TOSQ(move);
    const int captured = CAPTURED(move);

    const int side = pos->side;

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(ValidSide(side));
    assert(ValidPiece(pos->board[from]));
    assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

    // Save position
    pos->history[pos->hisPly].move       = move;
    pos->history[pos->hisPly].enPas      = pos->enPas;
    pos->history[pos->hisPly].fiftyMove  = pos->fiftyMove;
    pos->history[pos->hisPly].castlePerm = pos->castlePerm;
    pos->history[pos->hisPly].posKey     = pos->posKey;

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
        assert(ValidPiece(captured));
        ClearPiece(to, pos);

        // Reset 50mr after a capture
        pos->fiftyMove = 0;
    }

    // Move the piece
    MovePiece(from, to, pos);

    // Pawn move specifics
    if (piecePawn[pos->board[to]]) {

        // Reset 50mr after a pawn move
        pos->fiftyMove = 0;

        int promo = PROMOTION(move);

        // If the move is a pawnstart we set the en passant square and hash it in
        if (move & FLAG_PAWNSTART) {
            pos->enPas = to + 8 - 16 * side;
            assert((rankOf(pos->enPas) == RANK_3 && side) || rankOf(pos->enPas) == RANK_6);
            HASH_EP;

        // Remove pawn captured by en passant
        } else if (move & FLAG_ENPAS)
            ClearPiece(to + 8 - 16 * side, pos);

        // Replace promoting pawn with new piece
        else if (promo != EMPTY) {
            assert(ValidPiece(promo) && !piecePawn[promo]);
            ClearPiece(to, pos);
            AddPiece(to, pos, promo);
        }

    // Update king position if king moved
    } else if (pieceKing[pos->board[to]])
        pos->kingSq[side] = to;

    // Change turn to play
    pos->side ^= 1;
    HASH_SIDE;

    assert(CheckBoard(pos));

    // If own king is attacked after the move, take it back immediately
    if (SqAttacked(pos->kingSq[side], pos->side, pos)) {
        TakeMove(pos);
        return false;
    }

    return true;
}

// Pass the turn without moving
void MakeNullMove(Position *pos) {

    assert(CheckBoard(pos));
    assert(!SqAttacked(pos->kingSq[pos->side], !pos->side, pos));

    // Save misc info for takeback
    // pos->history[pos->hisPly].move    = NOMOVE;
    pos->history[pos->hisPly].enPas      = pos->enPas;
    pos->history[pos->hisPly].fiftyMove  = pos->fiftyMove;
    pos->history[pos->hisPly].castlePerm = pos->castlePerm;
    pos->history[pos->hisPly].posKey     = pos->posKey;

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
void TakeNullMove(Position *pos) {

    assert(CheckBoard(pos));

    // Decrease ply
    pos->hisPly--;
    pos->ply--;

    // Change side to play
    pos->side ^= 1;

    // Get info from history
    pos->enPas      = pos->history[pos->hisPly].enPas;
    pos->fiftyMove  = pos->history[pos->hisPly].fiftyMove;
    pos->castlePerm = pos->history[pos->hisPly].castlePerm;
    pos->posKey     = pos->history[pos->hisPly].posKey;

    assert(CheckBoard(pos));
    assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);
}