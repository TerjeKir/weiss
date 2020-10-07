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
#include "evaluate.h"
#include "move.h"
#include "psqt.h"


uint8_t SqDistance[64][64];

const int NonPawn[PIECE_NB] = {
    false, false,  true,  true,  true,  true, false, false,
    false, false,  true,  true,  true,  true, false, false
};

// Zobrist key tables
uint64_t PieceKeys[PIECE_NB][64];
uint64_t CastleKeys[16];
uint64_t SideKey;


// Initialize distance lookup table
void InitDistance() {

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
    for (Square sq = A3; sq <= H6; ++sq)
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

    memset(pos, 0, sizeof(Position));
}

// Update the rest of a position to match pos->board
static void UpdatePosition(Position *pos) {

    // Loop through each square on the board
    for (Square sq = A1; sq <= H8; ++sq) {

        Piece piece = pieceOn(sq);

        // If it isn't empty we update the relevant lists
        if (piece != EMPTY) {

            Color color = ColorOf(piece);
            PieceType pt = PieceTypeOf(piece);

            // Bitboards
            pieceBB(ALL)   |= BB(sq);
            pieceBB(pt)    |= BB(sq);
            colorBB(color) |= BB(sq);

            // Non pawn piece count
            pos->nonPawnCount[color] += NonPawn[piece];

            // Material score
            pos->material += PSQT[piece][sq];

            // Phase
            pos->phaseValue += PhaseValue[piece];
        }
    }

    pos->checkers = Checkers(pos);
    pos->phase = UpdatePhase(pos->phaseValue);
}

// Parse FEN and set up the position as described
void ParseFen(const char *fen, Position *pos) {

    assert(fen != NULL);

    ClearPosition(pos);

    // Piece locations
    Square sq = A8;
    while (*fen != ' ') {

        Piece piece;
        int count = 1;

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

        fen++;
    }
    fen++;

    // Update the rest of position to match pos->board
    UpdatePosition(pos);

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
    Square ep = AlgebraicToSq(fen[0], fen[1]);
    bool epValid = *fen != '-' && (  PawnAttackBB(!sideToMove, ep)
                                   & colorPieceBB(sideToMove, PAWN));
    pos->epSquare = epValid ? ep : 0;

    // 50 move rule and game moves
    pos->rule50 = atoi(fen += 2);
    pos->gameMoves = atoi(fen += 2);

    // Generate the position key
    pos->key = GeneratePosKey(pos);

    assert(PositionOk(pos));
}

// Translates a move to a string
char *BoardToFen(const Position *pos) {

    const char PceChar[]  = ".pnbrqk..PNBRQK";
    static char fen[100];
    char *ptr = fen;

    // Board
    for (int rank = RANK_8; rank >= RANK_1; --rank) {

        int count = 0;

        for (int file = FILE_A; file <= FILE_H; ++file) {
            Square sq = (rank * 8) + file;
            Piece piece = pieceOn(sq);

            if (piece) {
                if (count)
                    *ptr++ = '0' + count;
                *ptr++ = PceChar[piece];
                count = 0;
            } else
                count++;
        }

        if (count)
            *ptr++ = '0' + count;

        *ptr++ = rank == RANK_1 ? ' ' : '/';
    }

    // Side to move
    *ptr++ = sideToMove == WHITE ? 'w' : 'b';
    *ptr++ = ' ';

    // Castling rights
    int cr = pos->castlingRights;
    if (!cr)
        *ptr++ = '-';
    else {
        if (cr & WHITE_OO)  *ptr++ = 'K';
        if (cr & WHITE_OOO) *ptr++ = 'Q';
        if (cr & BLACK_OO)  *ptr++ = 'k';
        if (cr & BLACK_OOO) *ptr++ = 'q';
    }

    // En passant square in a separate string
    char ep[3] = "-";
    if (pos->epSquare)
        ep[0] = 'a' + FileOf(pos->epSquare),
        ep[1] = '1' + RankOf(pos->epSquare);

    // Add en passant, 50mr and game ply to the base
    sprintf(ptr, " %s %d %d", ep, pos->rule50, pos->gameMoves);

    return fen;
}

#if defined DEV || !defined NDEBUG
// Print the board with misc info
void PrintBoard(const Position *pos) {

    const char PceChar[]  = ".pnbrqk..PNBRQK";

    // Print board
    printf("\n");
    for (int rank = RANK_8; rank >= RANK_1; --rank) {
        for (int file = FILE_A; file <= FILE_H; ++file) {
            Square sq = (rank * 8) + file;
            printf("%3c", PceChar[pieceOn(sq)]);
        }
        printf("\n");
    }
    printf("\n");

    // Print FEN and zobrist key
    puts(BoardToFen(pos));
    printf("Zobrist Key: %" PRIu64 "\n\n", pos->key);
    fflush(stdout);
}
#endif

#ifndef NDEBUG
// Check board state makes sense
bool PositionOk(const Position *pos) {

    assert(0 <= pos->histPly && pos->histPly < MAXGAMEMOVES);

    int counts[PIECE_NB] = { 0 };
    int nonPawnCount[2] = { 0, 0 };

    for (Square sq = A1; sq <= H8; ++sq) {

        Piece piece = pieceOn(sq);
        counts[piece]++;
        Color color = ColorOf(piece);
        nonPawnCount[color] += NonPawn[piece];
    }

    for (Color c = BLACK; c <= WHITE; ++c) {

        assert(PopCount(colorPieceBB(c, PAWN))   == counts[MakePiece(c, PAWN)]);
        assert(PopCount(colorPieceBB(c, KNIGHT)) == counts[MakePiece(c, KNIGHT)]);
        assert(PopCount(colorPieceBB(c, BISHOP)) == counts[MakePiece(c, BISHOP)]);
        assert(PopCount(colorPieceBB(c, ROOK))   == counts[MakePiece(c, ROOK)]);
        assert(PopCount(colorPieceBB(c, QUEEN))  == counts[MakePiece(c, QUEEN)]);
        assert(PopCount(colorPieceBB(c, KING))   == 1);
        assert(counts[MakePiece(c, KING)] == 1);

        assert(nonPawnCount[c] == pos->nonPawnCount[c]);
    }

    assert(pieceBB(ALL) == (colorBB(WHITE) | colorBB(BLACK)));

    assert(sideToMove == WHITE || sideToMove == BLACK);

    assert(!pos->epSquare
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
    Square ep = pos->epSquare == 0 ? 0 : MirrorSquare(pos->epSquare);
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

    // Generate the position key
    pos->key = GeneratePosKey(pos);

    assert(PositionOk(pos));
}
#endif

// Static Exchange Evaluation
bool SEE(const Position *pos, const Move move, const int threshold) {

    assert(MoveIsPseudoLegal(pos, move));

    if (moveIsSpecial(move))
        return true;

    Square to = toSq(move);
    Square from = fromSq(move);

    // Making the move and not losing it must beat the threshold
    int value = PieceValue[MG][pieceOn(to)] - threshold;
    if (value < 0) return false;

    // Trivial if we still beat the threshold after losing the piece
    value -= PieceValue[MG][pieceOn(from)];
    if (value >= 0) return true;

    Bitboard occupied = (pieceBB(ALL) ^ BB(from)) | BB(to);
    Bitboard attackers = Attackers(pos, to, occupied);

    Bitboard bishops = pieceBB(BISHOP) | pieceBB(QUEEN);
    Bitboard rooks   = pieceBB(ROOK  ) | pieceBB(QUEEN);

    Color side = !ColorOf(pieceOn(from));

    // Make captures until one side runs out, or fail to beat threshold
    while (true) {

        // Remove used pieces from attackers
        attackers &= occupied;

        Bitboard myAttackers = attackers & colorBB(side);
        if (!myAttackers) break;

        // Pick next least valuable piece to capture with
        PieceType pt;
        for (pt = PAWN; pt < KING; ++pt)
            if (myAttackers & pieceBB(pt))
                break;

        side = !side;

        // Value beats threshold, or can't beat threshold (negamaxed)
        if ((value = -value - 1 - PieceValue[MG][pt]) >= 0) {

            if (pt == KING && (attackers & colorBB(side)))
                side = !side;

            break;
        }

        // Remove the used piece from occupied
        occupied ^= BB(Lsb(myAttackers & pieceBB(pt)));

        // Add possible discovered attacks from behind the used piece
        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= AttackBB(BISHOP, to, occupied) & bishops;
        if (pt == ROOK || pt == QUEEN)
            attackers |= AttackBB(ROOK, to, occupied) & rooks;
    }

    return side != ColorOf(pieceOn(from));
}
