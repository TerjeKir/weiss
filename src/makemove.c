/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2020  Terje Kirstihagen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "psqt.h"


#define HASH_PCE(piece, sq) (pos->key ^= (PieceKeys[(piece)][(sq)]))
#define HASH_CA             (pos->key ^= (CastleKeys[(pos->castlingRights)]))
#define HASH_SIDE           (pos->key ^= (SideKey))
#define HASH_EP             (pos->key ^= (PieceKeys[EMPTY][(pos->epSquare)]))


static const uint8_t CastlePerm[64] = {
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
static void ClearPiece(Position *pos, const Square sq) {

    const Piece piece = pieceOn(sq);
    const Color color = ColorOf(piece);

    assert(ValidPiece(piece));

    // Hash out the piece
    HASH_PCE(piece, sq);

    // Set square to empty
    pieceOn(sq) = EMPTY;

    // Update material
    pos->material -= PSQT[piece][sq];

    // Update phase
    pos->basePhase -= PhaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update various piece lists
    if (NonPawn[piece])
        pos->nonPawnCount[color]--;

    // Update bitboards
    CLRBIT(pieceBB(ALL), sq);
    CLRBIT(colorBB(color), sq);
    CLRBIT(pieceBB(PieceTypeOf(piece)), sq);
}

// Add a piece piece to a square
static void AddPiece(Position *pos, const Square sq, const Piece piece) {

    const Color color = ColorOf(piece);

    // Hash in piece at square
    HASH_PCE(piece, sq);

    // Update square
    pieceOn(sq) = piece;

    // Update material
    pos->material += PSQT[piece][sq];

    // Update phase
    pos->basePhase += PhaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update various piece lists
    if (NonPawn[piece])
        pos->nonPawnCount[color]++;

    // Update bitboards
    SETBIT(pieceBB(ALL), sq);
    SETBIT(colorBB(color), sq);
    SETBIT(pieceBB(PieceTypeOf(piece)), sq);
}

// Move a piece from one square to another
static void MovePiece(Position *pos, const Square from, const Square to) {

    const Piece piece = pieceOn(from);

    assert(ValidPiece(piece));
    assert(pieceOn(to) == EMPTY);

    // Hash out piece on old square, in on new square
    HASH_PCE(piece, from);
    HASH_PCE(piece, to);

    // Set old square to empty, new to piece
    pieceOn(from) = EMPTY;
    pieceOn(to)   = piece;

    // Update material
    pos->material += PSQT[piece][to] - PSQT[piece][from];

    // Update bitboards
    CLRBIT(pieceBB(ALL), from);
    SETBIT(pieceBB(ALL), to);

    CLRBIT(colorBB(ColorOf(piece)), from);
    SETBIT(colorBB(ColorOf(piece)), to);

    CLRBIT(pieceBB(PieceTypeOf(piece)), from);
    SETBIT(pieceBB(PieceTypeOf(piece)), to);
}

// Take back the previous move
void TakeMove(Position *pos) {

    // Decrement gamePly, ply
    pos->gamePly--;
    pos->ply--;

    // Change side to play
    sideToMove ^= 1;

    // Update castling rights, 50mr, en passant
    pos->epSquare       = history(0).epSquare;
    pos->rule50         = history(0).rule50;
    pos->castlingRights = history(0).castlingRights;

    // Get the move from history
    const Move move = history(0).move;
    const Square from = fromSq(move);
    const Square to = toSq(move);

    // Add in pawn captured by en passant
    if (moveIsEnPas(move))
        AddPiece(pos, to ^ 8, MakePiece(!sideToMove, PAWN));

    // Move rook back if castling
    else if (moveIsCastle(move))
        switch (to) {
            case C1: MovePiece(pos, D1, A1); break;
            case C8: MovePiece(pos, D8, A8); break;
            case G1: MovePiece(pos, F1, H1); break;
            case G8: MovePiece(pos, F8, H8); break;
            default: assert(false); break;
        }

    // Make reverse move (from <-> to)
    MovePiece(pos, to, from);

    // Add back captured piece if any
    Piece captured = capturing(move);
    if (captured != EMPTY) {
        assert(ValidPiece(captured));
        AddPiece(pos, to, captured);
    }

    // Remove promoted piece and put back the pawn
    if (promotion(move) != EMPTY) {
        assert(ValidPiece(promotion(move)) && NonPawn[promotion(move)]);
        ClearPiece(pos, from);
        AddPiece(pos, from, MakePiece(sideToMove, PAWN));
    }

    // Get old poskey from history
    pos->key = history(0).posKey;

    assert(PositionOk(pos));
}

// Make a move - take it back and return false if move was illegal
bool MakeMove(Position *pos, const Move move) {

    const Square from = fromSq(move);
    const Square to   = toSq(move);
    const Piece capt  = capturing(move);
    const Color color = sideToMove;

    // Save position
    history(0).posKey         = pos->key;
    history(0).move           = move;
    history(0).epSquare       = pos->epSquare;
    history(0).rule50         = pos->rule50;
    history(0).castlingRights = pos->castlingRights;

    // Increment gamePly, ply and 50mr
    pos->gamePly++;
    pos->ply++;
    pos->rule50++;

    // Hash out the old en passant if exist and unset it
    if (pos->epSquare != NO_SQ) {
        HASH_EP;
        pos->epSquare = NO_SQ;
    }

    // Rehash the castling rights if at least one side can castle,
    // and either the to or from square is the original square of
    // a king or rook.
    if (pos->castlingRights && CastlePerm[from] ^ CastlePerm[to]) {
        HASH_CA;
        pos->castlingRights &= CastlePerm[from];
        pos->castlingRights &= CastlePerm[to];
        HASH_CA;
    }

    // Move the rook during castling
    if (moveIsCastle(move))
        switch (to) {
            case C1: MovePiece(pos, A1, D1); break;
            case C8: MovePiece(pos, A8, D8); break;
            case G1: MovePiece(pos, H1, F1); break;
            case G8: MovePiece(pos, H8, F8); break;
            default: assert(false); break;
        }

    // Remove captured piece if any
    else if (capt != EMPTY) {
        assert(ValidPiece(capt));
        ClearPiece(pos, to);

        // Reset 50mr after a capture
        pos->rule50 = 0;
    }

    // Move the piece
    MovePiece(pos, from, to);

    // Pawn move specifics
    if (PieceTypeOf(pieceOn(to)) == PAWN) {

        // Reset 50mr after a pawn move
        pos->rule50 = 0;

        Piece promo = promotion(move);

        // If the move is a pawnstart we set the en passant square and hash it in
        if (moveIsPStart(move)) {
            pos->epSquare = to ^ 8;
            HASH_EP;

        // Remove pawn captured by en passant
        } else if (moveIsEnPas(move))
            ClearPiece(pos, to ^ 8);

        // Replace promoting pawn with new piece
        else if (promo != EMPTY) {
            assert(ValidPiece(promo) && NonPawn[promo]);
            ClearPiece(pos, to);
            AddPiece(pos, to, promo);
        }
    }

    // Change turn to play
    sideToMove ^= 1;
    HASH_SIDE;

    assert(PositionOk(pos));

    // If own king is attacked after the move, take it back immediately
    if (SqAttacked(pos, Lsb(colorPieceBB(color, KING)), sideToMove)) {
        TakeMove(pos);
        return false;
    }

    return true;
}

// Pass the turn without moving
void MakeNullMove(Position *pos) {

    // Save misc info for takeback
    history(0).posKey         = pos->key;
    history(0).move           = NOMOVE;
    history(0).epSquare       = pos->epSquare;
    history(0).rule50         = pos->rule50;
    history(0).castlingRights = pos->castlingRights;

    // Increase ply
    pos->ply++;
    pos->gamePly++;

    pos->rule50 = 0;

    // Change side to play
    sideToMove ^= 1;
    HASH_SIDE;

    // Hash out en passant if there was one, and unset it
    if (pos->epSquare != NO_SQ) {
        HASH_EP;
        pos->epSquare = NO_SQ;
    }

    assert(PositionOk(pos));
}

// Take back a null move
void TakeNullMove(Position *pos) {

    // Decrease ply
    pos->gamePly--;
    pos->ply--;

    // Change side to play
    sideToMove ^= 1;

    // Get info from history
    pos->key            = history(0).posKey;
    pos->epSquare       = history(0).epSquare;
    pos->rule50         = history(0).rule50;
    pos->castlingRights = history(0).castlingRights;

    assert(PositionOk(pos));
}