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


#define HASH_PCE(piece, sq) (pos->key ^= PieceKeys[(piece)][(sq)])
#define HASH_CA             (pos->key ^= CastleKeys[pos->castlingRights])
#define HASH_SIDE           (pos->key ^= SideKey)
#define HASH_EP             (pos->key ^= PieceKeys[EMPTY][pos->epSquare])


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
static void ClearPiece(Position *pos, const Square sq, const bool hash) {

    const Piece piece = pieceOn(sq);
    const Color color = ColorOf(piece);
    const PieceType pt = PieceTypeOf(piece);

    assert(ValidPiece(piece));

    // Hash out the piece
    if (hash)
        HASH_PCE(piece, sq);

    // Set square to empty
    pieceOn(sq) = EMPTY;

    // Update material
    pos->material -= PSQT[piece][sq];

    // Update phase
    pos->basePhase -= PhaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update non-pawn count
    pos->nonPawnCount[color] -= NonPawn[piece];

    // Update bitboards
    pieceBB(ALL)   ^= SquareBB[sq];
    pieceBB(pt)    ^= SquareBB[sq];
    colorBB(color) ^= SquareBB[sq];
}

// Add a piece piece to a square
static void AddPiece(Position *pos, const Square sq, const Piece piece, const bool hash) {

    const Color color = ColorOf(piece);
    const PieceType pt = PieceTypeOf(piece);

    // Hash in piece at square
    if (hash)
        HASH_PCE(piece, sq);

    // Update square
    pieceOn(sq) = piece;

    // Update material
    pos->material += PSQT[piece][sq];

    // Update phase
    pos->basePhase += PhaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update non-pawn count
    pos->nonPawnCount[color] += NonPawn[piece];

    // Update bitboards
    pieceBB(ALL)   |= SquareBB[sq];
    pieceBB(pt)    |= SquareBB[sq];
    colorBB(color) |= SquareBB[sq];
}

// Move a piece from one square to another
static void MovePiece(Position *pos, const Square from, const Square to, const bool hash) {

    const Piece piece = pieceOn(from);
    const Color color = ColorOf(piece);
    const PieceType pt = PieceTypeOf(piece);

    assert(ValidPiece(piece));
    assert(pieceOn(to) == EMPTY);

    // Hash out piece on old square, in on new square
    if (hash)
        HASH_PCE(piece, from),
        HASH_PCE(piece, to);

    // Set old square to empty, new to piece
    pieceOn(from) = EMPTY;
    pieceOn(to)   = piece;

    // Update material
    pos->material += PSQT[piece][to] - PSQT[piece][from];

    // Update bitboards
    pieceBB(ALL)   ^= SquareBB[from] ^ SquareBB[to];
    pieceBB(pt)    ^= SquareBB[from] ^ SquareBB[to];
    colorBB(color) ^= SquareBB[from] ^ SquareBB[to];
}

// Take back the previous move
void TakeMove(Position *pos) {

    // Decrement histPly, ply
    pos->histPly--;
    pos->ply--;

    // Change side to play
    sideToMove ^= 1;

    // Get the move from history
    const Move move = history(0).move;
    const Square from = fromSq(move);
    const Square to = toSq(move);

    // Add in pawn captured by en passant
    if (moveIsEnPas(move))
        AddPiece(pos, to ^ 8, MakePiece(!sideToMove, PAWN), false);

    // Move rook back if castling
    else if (moveIsCastle(move))
        switch (to) {
            case C1: MovePiece(pos, D1, A1, false); break;
            case C8: MovePiece(pos, D8, A8, false); break;
            case G1: MovePiece(pos, F1, H1, false); break;
            default: MovePiece(pos, F8, H8, false); break;
        }

    // Make reverse move (from <-> to)
    MovePiece(pos, to, from, false);

    // Add back captured piece if any
    Piece capt = capturing(move);
    if (capt != EMPTY) {
        assert(ValidCapture(capt));
        AddPiece(pos, to, capt, false);
    }

    // Remove promoted piece and put back the pawn
    Piece promo = promotion(move);
    if (promo != EMPTY) {
        assert(ValidPromotion(promo));
        ClearPiece(pos, from, false);
        AddPiece(pos, from, MakePiece(sideToMove, PAWN), false);
    }

    // Get various info from history
    pos->key            = history(0).posKey;
    pos->epSquare       = history(0).epSquare;
    pos->rule50         = history(0).rule50;
    pos->castlingRights = history(0).castlingRights;

    assert(PositionOk(pos));
}

// Make a move - take it back and return false if move was illegal
bool MakeMove(Position *pos, const Move move) {

    // Save position
    history(0).posKey         = pos->key;
    history(0).move           = move;
    history(0).epSquare       = pos->epSquare;
    history(0).rule50         = pos->rule50;
    history(0).castlingRights = pos->castlingRights;

    // Increment histPly, ply and 50mr
    pos->histPly++;
    pos->ply++;
    pos->rule50++;

    // Hash out en passant if there was one, and unset it
    HASH_EP;
    pos->epSquare = 0;

    const Square from = fromSq(move);
    const Square to = toSq(move);

    // Rehash the castling rights
    HASH_CA;
    pos->castlingRights &= CastlePerm[from] & CastlePerm[to];
    HASH_CA;

    // Remove captured piece if any
    Piece capt = capturing(move);
    if (capt != EMPTY) {
        assert(ValidCapture(capt));
        ClearPiece(pos, to, true);
        pos->rule50 = 0;
    }

    // Move the piece
    MovePiece(pos, from, to, true);

    // Pawn move specifics
    if (PieceTypeOf(pieceOn(to)) == PAWN) {

        pos->rule50 = 0;
        Piece promo = promotion(move);

        // Set en passant square if applicable
        if (moveIsPStart(move)) {
            if ((  PawnAttackBB(sideToMove, to ^ 8)
                 & colorPieceBB(!sideToMove, PAWN))) {

                pos->epSquare = to ^ 8;
                HASH_EP;
            }

        // Remove pawn captured by en passant
        } else if (moveIsEnPas(move))
            ClearPiece(pos, to ^ 8, true);

        // Replace promoting pawn with new piece
        else if (promo != EMPTY) {
            assert(ValidPromotion(promo));
            ClearPiece(pos, to, true);
            AddPiece(pos, to, promo, true);
        }

    // Move the rook during castling
    } else if (moveIsCastle(move))
        switch (to) {
            case C1: MovePiece(pos, A1, D1, true); break;
            case C8: MovePiece(pos, A8, D8, true); break;
            case G1: MovePiece(pos, H1, F1, true); break;
            default: MovePiece(pos, H8, F8, true); break;
        }

    // Change turn to play
    sideToMove ^= 1;
    HASH_SIDE;

    // If own king is attacked after the move, take it back immediately
    if (KingAttacked(pos, sideToMove^1))
        return TakeMove(pos), false;

    assert(PositionOk(pos));

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
    pos->histPly++;

    pos->rule50 = 0;

    // Change side to play
    sideToMove ^= 1;
    HASH_SIDE;

    // Hash out en passant if there was one, and unset it
    HASH_EP;
    pos->epSquare = 0;

    assert(PositionOk(pos));
}

// Take back a null move
void TakeNullMove(Position *pos) {

    // Decrease ply
    pos->histPly--;
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
