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

    const int piece = pieceOn(sq);
    const int color = colorOf(piece);

    assert(ValidPiece(piece));
    assert(ValidSide(color));

    // Hash out the piece
    HASH_PCE(piece, sq);

    // Set square to empty
    pieceOn(sq) = EMPTY;

    // Update material
    pos->material -= PSQT[piece][sq];

    // Update phase
    pos->basePhase += phaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update various piece lists
    if (pieceBig[piece])
        pos->bigPieces[color]--;

    // Update piece list
    int lastSquare = pos->pieceList[piece][--pos->pieceCounts[piece]];
    pos->index[lastSquare] = pos->index[sq];
    pos->pieceList[piece][pos->index[lastSquare]] = lastSquare;
    // pos->pieceList[piece][pos->pieceCounts[piece]] = NO_SQ;

    // Update bitboards
    CLRBIT(pieceBB(ALL), sq);
    CLRBIT(colorBB(color), sq);
    CLRBIT(pieceBB(pieceTypeOf(piece)), sq);
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
    pieceOn(sq) = piece;

    // Update material
    pos->material += PSQT[piece][sq];

    // Update phase
    pos->basePhase -= phaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update various piece lists
    if (pieceBig[piece])
        pos->bigPieces[color]++;

    pos->index[sq] = pos->pieceCounts[piece]++;
    pos->pieceList[piece][pos->index[sq]] = sq;

    // Update bitboards
    SETBIT(pieceBB(ALL), sq);
    SETBIT(colorBB(color), sq);
    SETBIT(pieceBB(pieceTypeOf(piece)), sq);
}

// Move a piece from one square to another
static void MovePiece(const int from, const int to, Position *pos) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));

    const int piece = pieceOn(from);

    assert(ValidPiece(piece));

    // Hash out piece on old square, in on new square
    HASH_PCE(piece, from);
    HASH_PCE(piece, to);

    // Set old square to empty, new to piece
    pieceOn(from) = EMPTY;
    pieceOn(to)   = piece;

    // Update square for the piece in pieceList
    pos->index[to] = pos->index[from];
    pos->pieceList[piece][pos->index[to]] = to;

    // Update material
    pos->material += PSQT[piece][to] - PSQT[piece][from];

    // Update bitboards
    CLRBIT(pieceBB(ALL), from);
    SETBIT(pieceBB(ALL), to);

    CLRBIT(colorBB(colorOf(piece)), from);
    SETBIT(colorBB(colorOf(piece)), to);

    CLRBIT(pieceBB(pieceTypeOf(piece)), from);
    SETBIT(pieceBB(pieceTypeOf(piece)), to);
}

// Take back the previous move
void TakeMove(Position *pos) {

    assert(CheckBoard(pos));

    // Decrement hisPly, ply
    pos->hisPly--;
    pos->ply--;

    // Change side to play
    sideToMove() ^= 1;

    // Update castling rights, 50mr, en passant
    pos->enPas      = history(0).enPas;
    pos->fiftyMove  = history(0).fiftyMove;
    pos->castlePerm = history(0).castlePerm;

    assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

    // Get the move from history
    const int move = history(0).move;
    const int from = FROMSQ(move);
    const int   to =   TOSQ(move);

    assert(ValidSquare(from));
    assert(ValidSquare(to));

    // Add in pawn captured by en passant
    if (FLAG_ENPAS & move)
        AddPiece(to + 8 - 16 * sideToMove(), pos, makePiece(!sideToMove(), PAWN));

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
    pos->posKey = history(0).posKey;

    assert(CheckBoard(pos));
}

// Make a move - take it back and return false if move was illegal
bool MakeMove(Position *pos, const int move) {

    assert(CheckBoard(pos));

    const int from     = FROMSQ(move);
    const int to       = TOSQ(move);
    const int captured = CAPTURED(move);

    const int side = sideToMove();

    assert(ValidSquare(from));
    assert(ValidSquare(to));
    assert(ValidSide(side));
    assert(ValidPiece(pieceOn(from)));
    assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);

    // Save position
    history(0).move       = move;
    history(0).enPas      = pos->enPas;
    history(0).fiftyMove  = pos->fiftyMove;
    history(0).castlePerm = pos->castlePerm;
    history(0).posKey     = pos->posKey;

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

    // Rehash the castling rights if at least one side can castle,
    // and either the to or from square is the original square of
    // a king or rook.
    if (pos->castlePerm && CastlePerm[from] ^ CastlePerm[to]) {
        HASH_CA;
        pos->castlePerm &= CastlePerm[from];
        pos->castlePerm &= CastlePerm[to];
        HASH_CA;
    }

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
    if (piecePawn[pieceOn(to)]) {

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
    }

    // Change turn to play
    sideToMove() ^= 1;
    HASH_SIDE;

    assert(CheckBoard(pos));

    // If own king is attacked after the move, take it back immediately
    if (SqAttacked(pos->pieceList[makePiece(side, KING)][0], sideToMove(), pos)) {
        TakeMove(pos);
        return false;
    }

    return true;
}

// Pass the turn without moving
void MakeNullMove(Position *pos) {

    assert(CheckBoard(pos));

    // Save misc info for takeback
    history(0).move       = NOMOVE;
    history(0).enPas      = pos->enPas;
    history(0).fiftyMove  = pos->fiftyMove;
    history(0).castlePerm = pos->castlePerm;
    history(0).posKey     = pos->posKey;

    // Increase ply
    pos->ply++;
    pos->hisPly++;

    // Change side to play
    sideToMove() ^= 1;
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
    sideToMove() ^= 1;

    // Get info from history
    pos->enPas      = history(0).enPas;
    pos->fiftyMove  = history(0).fiftyMove;
    pos->castlePerm = history(0).castlePerm;
    pos->posKey     = history(0).posKey;

    assert(CheckBoard(pos));
    assert(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
    assert(pos->ply >= 0 && pos->ply < MAXDEPTH);
}