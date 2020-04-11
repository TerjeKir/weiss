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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "psqt.h"


uint8_t SqDistance[64][64];

//                                EMPTY,    bP,    bN,    bB,    bR,    bQ,    bK, EMPTY, EMPTY,    wP,    wN,    wB,    wR,    wQ,    wK, EMPTY
const int NonPawn[PIECE_NB]    = { false, false,  true,  true,  true,  true, false, false, false, false,  true,  true,  true,  true, false, false };
const int PhaseValue[PIECE_NB] = {     0,     0,     1,     1,     2,     4,     0,     0,     0,     0,     1,     1,     2,     4,     0,     0 };

// Zobrist key tables
uint64_t PieceKeys[PIECE_NB][64];
uint64_t CastleKeys[16];
uint64_t SideKey;


// Initialize distance lookup table
CONSTR InitDistance() {

    for (Square sq1 = A1; sq1 <= H8; ++sq1)
        for (Square sq2 = A1; sq2 <= H8; ++sq2) {
            int vertical   = abs(RankOf(sq1) - RankOf(sq2));
            int horizontal = abs(FileOf(sq1) - FileOf(sq2));
            SqDistance[sq1][sq2] = MAX(vertical, horizontal);
        }
}

// Pseudo-random number generator
static uint64_t Rand64() {

    // http://vigna.di.unimi.it/ftp/papers/xorshift.pdf

    static uint64_t seed = 1070372ull;

    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;

    return seed * 2685821657736338717ull;
}

// Inits zobrist key tables
CONSTR InitHashKeys() {

    // Side to play
    SideKey = Rand64();

    // En passant
    for (Square sq = A1; sq <= H8; ++sq)
        PieceKeys[0][sq] = Rand64();

    // White pieces
    for (Piece piece = wP; piece <= wK; ++piece)
        for (Square sq = A1; sq <= H8; ++sq)
            PieceKeys[piece][sq] = Rand64();

    // Black pieces
    for (Piece piece = bP; piece <= bK; ++piece)
        for (Square sq = A1; sq <= H8; ++sq)
            PieceKeys[piece][sq] = Rand64();

    // Castling rights
    for (int i = 0; i < 16; ++i)
        CastleKeys[i] = Rand64();
}

// Generates a hash key for the position. During
// a search this is incrementally updated instead.
static Key GeneratePosKey(const Position *pos) {

    Key posKey = 0;

    // Pieces
    for (Square sq = A1; sq <= H8; ++sq) {
        Piece piece = pieceOn(sq);
        if (piece != EMPTY)
            posKey ^= PieceKeys[piece][sq];
    }

    // Side to play
    if (sideToMove == WHITE)
        posKey ^= SideKey;

    // En passant
    if (pos->epSquare != NO_SQ)
        posKey ^= PieceKeys[EMPTY][pos->epSquare];

    // Castling rights
    posKey ^= CastleKeys[pos->castlingRights];

    return posKey;
}

// Calculates the position key after a move. Fails
// for special moves.
Key KeyAfter(const Position *pos, const Move move) {

    Square from = fromSq(move);
    Square to   = toSq(move);
    Piece piece = pieceOn(from);
    Piece capt  = capturing(move);
    Key key     = pos->key ^ SideKey;

    if (capt)
        key ^= PieceKeys[capt][to];

    return key ^ PieceKeys[piece][from] ^ PieceKeys[piece][to];
}

// Clears the board
static void ClearPosition(Position *pos) {

    memset(pos, EMPTY, sizeof(Position));
}

// Update the rest of a position to match pos->board
static void UpdatePosition(Position *pos) {

    // Generate the position key
    pos->key = GeneratePosKey(pos);

    // Loop through each square on the board
    for (Square sq = A1; sq <= H8; ++sq) {

        Piece piece = pieceOn(sq);

        // If it isn't empty we update the relevant lists
        if (piece != EMPTY) {

            Color color = ColorOf(piece);

            // Bitboards
            SETBIT(pieceBB(ALL), sq);
            SETBIT(colorBB(ColorOf(piece)), sq);
            SETBIT(pieceBB(PieceTypeOf(piece)), sq);

            // Non pawn piece count
            if (NonPawn[piece])
                pos->nonPawnCount[color]++;

            // Material score
            pos->material += PSQT[piece][sq];

            // Phase
            pos->basePhase += PhaseValue[piece];
        }
    }

    pos->phase = (pos->basePhase * 256 + 12) / 24;
}

// Parse FEN and set up the position as described
void ParseFen(const char *fen, Position *pos) {

    assert(fen != NULL);

    ClearPosition(pos);

    Piece piece;
    int count = 1;
    Square sq = A8;

    // Piece locations
    while (*fen != ' ') {
        switch (*fen) {
            // Pieces
            case 'p': piece = bP; break;
            case 'n': piece = bN; break;
            case 'b': piece = bB; break;
            case 'r': piece = bR; break;
            case 'q': piece = bQ; break;
            case 'k': piece = bK; break;
            case 'P': piece = wP; break;
            case 'N': piece = wN; break;
            case 'B': piece = wB; break;
            case 'R': piece = wR; break;
            case 'Q': piece = wQ; break;
            case 'K': piece = wK; break;
            // Next rank
            case '/':
                sq -= 16;
                fen++;
                continue;
            // Numbers of empty squares
            default:
                piece = EMPTY;
                count = *fen - '0';
                break;
        }

        pieceOn(sq) = piece;
        sq += count;
        count = 1;

        fen++;
    }
    fen++;

    // Side to move
    sideToMove = (*fen == 'w') ? WHITE : BLACK;
    fen += 2;

    // Castling rights
    while (*fen != ' ') {

        switch (*fen) {
            case 'K': pos->castlingRights |= WHITE_OO;  break;
            case 'Q': pos->castlingRights |= WHITE_OOO; break;
            case 'k': pos->castlingRights |= BLACK_OO;  break;
            case 'q': pos->castlingRights |= BLACK_OOO; break;
            default: break;
        }
        fen++;
    }
    fen++;

    // En passant square
    if (*fen == '-')
        pos->epSquare = NO_SQ;
    else {
        int file = fen[0] - 'a';
        int rank = fen[1] - '1';

        pos->epSquare = (8 * rank) + file;
    }
    fen += 2;

    // 50 move rule
    pos->rule50 = atoi(fen);

    // Update the rest of position to match pos->board
    UpdatePosition(pos);

    assert(PositionOk(pos));
}

#if defined DEV || !defined NDEBUG
// Print the board with misc info
void PrintBoard(const Position *pos) {

    const char PceChar[]  = ".pnbrqk..PNBRQK";
    char fen[100];
    char *ptr = fen;

    printf("\n");
    for (int rank = RANK_8; rank >= RANK_1; --rank) {

        int cnt = 0;

        for (int file = FILE_A; file <= FILE_H; ++file) {
            Square sq = (rank * 8) + file;
            Piece piece = pieceOn(sq);

            // Build fen string
            if (piece) {
                if (cnt)
                    *ptr++ = '0' + cnt;
                *ptr++ = PceChar[piece];
                cnt = 0;
            } else
                cnt++;

            // Print board
            printf("%3c", PceChar[piece]);
        }

        if (cnt)
            *ptr++ = '0' + cnt;

        *ptr++ = rank == RANK_1 ? ' ' : '/';

        printf("\n");
    }

    *ptr++ = sideToMove == WHITE ? 'w' : 'b';
    *ptr++ = ' ';

    int cr = pos->castlingRights;
    if (!cr)
        *ptr++ = '-';
    else {
        if (cr & WHITE_OO)  *ptr++ = 'K';
        if (cr & WHITE_OOO) *ptr++ = 'Q';
        if (cr & BLACK_OO)  *ptr++ = 'k';
        if (cr & BLACK_OOO) *ptr++ = 'q';
    }

    char ep[3];
    if (pos->epSquare == NO_SQ) ep[0] = '-';
    else sprintf(ep, "%c%c", 'a' + pos->epSquare / 8, '1' + (pos->epSquare & 7));

    printf("\n%s %s %d %d\n", fen, ep, pos->rule50, pos->gamePly + 1);
    printf("Zobrist Key: %" PRIu64 "\n\n", pos->key);
    fflush(stdout);
}
#endif

#ifndef NDEBUG
// Check board state makes sense
bool PositionOk(const Position *pos) {

    assert(0 <= pos->gamePly && pos->gamePly < MAXGAMEMOVES);
    assert(    0 <= pos->ply && pos->ply < MAXDEPTH);

    int counts[PIECE_NB] = { 0 };
    int nonPawnCount[2] = { 0, 0 };

    for (Square sq = A1; sq <= H8; ++sq) {

        Piece piece = pieceOn(sq);
        counts[piece]++;
        Color color = ColorOf(piece);
        nonPawnCount[color] += NonPawn[piece];
    }

    assert(counts[MakePiece(WHITE, KING)] == 1);
    assert(counts[MakePiece(BLACK, KING)] == 1);

    assert(PopCount(colorPieceBB(WHITE, PAWN))   == counts[MakePiece(WHITE, PAWN)]);
    assert(PopCount(colorPieceBB(WHITE, KNIGHT)) == counts[MakePiece(WHITE, KNIGHT)]);
    assert(PopCount(colorPieceBB(WHITE, BISHOP)) == counts[MakePiece(WHITE, BISHOP)]);
    assert(PopCount(colorPieceBB(WHITE, ROOK))   == counts[MakePiece(WHITE, ROOK)]);
    assert(PopCount(colorPieceBB(WHITE, QUEEN))  == counts[MakePiece(WHITE, QUEEN)]);
    assert(PopCount(colorPieceBB(WHITE, KING))   == 1);

    assert(PopCount(colorPieceBB(BLACK, PAWN))   == counts[MakePiece(BLACK, PAWN)]);
    assert(PopCount(colorPieceBB(BLACK, KNIGHT)) == counts[MakePiece(BLACK, KNIGHT)]);
    assert(PopCount(colorPieceBB(BLACK, BISHOP)) == counts[MakePiece(BLACK, BISHOP)]);
    assert(PopCount(colorPieceBB(BLACK, ROOK))   == counts[MakePiece(BLACK, ROOK)]);
    assert(PopCount(colorPieceBB(BLACK, QUEEN))  == counts[MakePiece(BLACK, QUEEN)]);
    assert(PopCount(colorPieceBB(BLACK, KING))   == 1);

    assert(pieceBB(ALL) == (colorBB(WHITE) | colorBB(BLACK)));

    assert(nonPawnCount[WHITE] == pos->nonPawnCount[WHITE]
        && nonPawnCount[BLACK] == pos->nonPawnCount[BLACK]);

    assert(sideToMove == WHITE || sideToMove == BLACK);

    assert(pos->epSquare == NO_SQ
       || (RelativeRank(sideToMove, RankOf(pos->epSquare)) == RANK_6));

    assert(pos->castlingRights >= 0
        && pos->castlingRights <= 15);

    assert(GeneratePosKey(pos) == pos->key);

    assert(!KingAttacked(pos, !sideToMove));

    return true;
}
#endif

#ifdef DEV
// Reverse the colors
void MirrorBoard(Position *pos) {

    // Save the necessary position info mirrored
    uint8_t board[64];
    for (Square sq = A1; sq <= H8; ++sq)
        board[sq] = MirrorPiece(pieceOn(MirrorSquare(sq)));

    Color stm = !sideToMove;
    Square ep = pos->epSquare == NO_SQ ? NO_SQ : MirrorSquare(pos->epSquare);
    uint8_t cr = (pos->castlingRights & WHITE_CASTLE) << 2
               | (pos->castlingRights & BLACK_CASTLE) >> 2;

    // Clear the position
    ClearPosition(pos);

    // Fill in the mirrored position info
    for (Square sq = A1; sq <= H8; ++sq)
        pieceOn(sq) = board[sq];

    sideToMove = stm;
    pos->epSquare = ep;
    pos->castlingRights = cr;

    // Update the rest of the position to match pos->board
    UpdatePosition(pos);

    assert(PositionOk(pos));
}
#endif