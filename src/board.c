// board.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitboards.h"
#include "board.h"
#include "psqt.h"
#include "hashkeys.h"
#include "validate.h"


//                                EMPTY,    bP,    bN,    bB,    bR,    bQ,    bK, EMPTY, EMPTY,    wP,    wN,    wB,    wR,    wQ,    wK, EMPTY
const int pieceBig [PIECE_NB] = { false, false,  true,  true,  true,  true, false, false, false, false,  true,  true,  true,  true, false, false };
const int piecePawn[PIECE_NB] = { false,  true, false, false, false, false, false, false, false,  true, false, false, false, false, false, false };
const int pieceKing[PIECE_NB] = { false, false, false, false, false, false,  true, false, false, false, false, false, false, false,  true, false };

const int phaseValue[PIECE_NB] = {    0,     0,     1,     1,     2,     4,     0,     0,     0,     0,     1,     1,     2,     4,     0,     0 };

const int mirror_square[64] = {
    56, 57, 58, 59, 60, 61, 62, 63,
    48, 49, 50, 51, 52, 53, 54, 55,
    40, 41, 42, 43, 44, 45, 46, 47,
    32, 33, 34, 35, 36, 37, 38, 39,
    24, 25, 26, 27, 28, 29, 30, 31,
    16, 17, 18, 19, 20, 21, 22, 23,
     8,  9, 10, 11, 12, 13, 14, 15,
     0,  1,  2,  3,  4,  5,  6,  7
};


// Initialize distance lookup table
CONSTR InitDistance() {

    int sq1, sq2, vertical, horizontal;

    for (sq1 = A1; sq1 <= H8; ++sq1)
        for (sq2 = A1; sq2 <= H8; ++sq2) {
            vertical   = abs(rankOf(sq1) - rankOf(sq2));
            horizontal = abs(fileOf(sq1) - fileOf(sq2));
            distance[sq1][sq2] = ((vertical > horizontal) ? vertical : horizontal);
        }
}

// Update the rest of a position to match pos->board
static void UpdatePosition(Position *pos) {

    int sq, piece, color;

    // Generate the position key
    pos->posKey = GeneratePosKey(pos);

    // Loop through each square on the board
    for (sq = A1; sq <= H8; ++sq) {

        piece = pos->board[sq];

        // If it isn't empty we update the relevant lists
        if (piece != EMPTY) {

            color = colorOf(piece);

            // Bitboards
            SETBIT(pos->pieceBB[ALL], sq);
            SETBIT(pos->colorBB[colorOf(piece)], sq);
            SETBIT(pos->pieceBB[pieceTypeOf(piece)], sq);

            // Non pawn piece count
            if (pieceBig[piece])
                pos->bigPieces[color]++;

            // Material score
            pos->material += PSQT[piece][sq];

            // Phase
            pos->basePhase -= phaseValue[piece];

            // Piece list / count
            pos->pieceList[piece][pos->pieceCounts[piece]++] = sq;

            // King square
            if (piece == wK) pos->kingSq[WHITE] = sq;
            if (piece == bK) pos->kingSq[BLACK] = sq;
        }
    }

    pos->phase = (pos->basePhase * 256 + 12) / 24;

    assert(CheckBoard(pos));
}

// Clears the board
static void ClearPosition(Position *pos) {

    // Bitboard representations
    memset(pos->colorBB, 0ULL, sizeof(pos->colorBB));
    memset(pos->pieceBB, 0ULL, sizeof(pos->pieceBB));

    // Array representation
    memset(pos->board, 0ULL, sizeof(pos->board));

    // Piece lists and counts
    memset(pos->pieceCounts, 0, sizeof(pos->pieceCounts));
    memset(pos->pieceList,   0, sizeof(pos->pieceList));

    // King squares
    pos->kingSq[BLACK] = pos->kingSq[WHITE] = NO_SQ;

    // Big piece counts
    pos->bigPieces[BLACK] = pos->bigPieces[WHITE] = 0;

    // Misc
    pos->material   = 0;
    pos->basePhase  = 24;
    pos->side       = -1;
    pos->enPas      = NO_SQ;
    pos->fiftyMove  = 0;
    pos->castlePerm = 0;

    // Ply
    pos->ply = 0;
    pos->hisPly = 0;

    // Position key
    pos->posKey = 0ULL;
}

// Parse FEN and set up the position as described
void ParseFen(const char *fen, Position *pos) {

    assert(fen != NULL);

    int rank = RANK_8;
    int file = FILE_A;
    int piece, count, i, sq;

    ClearPosition(pos);

    // Piece locations
    while ((rank >= RANK_1) && *fen) {
        count = 1;
        switch (*fen) {
            case 'p': piece = bP; break;
            case 'r': piece = bR; break;
            case 'n': piece = bN; break;
            case 'b': piece = bB; break;
            case 'k': piece = bK; break;
            case 'q': piece = bQ; break;
            case 'P': piece = wP; break;
            case 'R': piece = wR; break;
            case 'N': piece = wN; break;
            case 'B': piece = wB; break;
            case 'K': piece = wK; break;
            case 'Q': piece = wQ; break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                piece = EMPTY;
                count = *fen - '0';
                break;

            case '/':
            case ' ':
                rank--;
                file = FILE_A;
                fen++;
                continue;

            default:
                printf("FEN error!\n");
                fflush(stdout);
                return;
        }

        for (i = 0; i < count; ++i) {
            sq = rank * 8 + file;
            if (piece != EMPTY)
                pos->board[sq] = piece;

            file++;
        }
        fen++;
    }

    // Side to move
    assert(*fen == 'w' || *fen == 'b');
    pos->side = (*fen == 'w') ? WHITE : BLACK;
    fen += 2;

    // Castling rights
    for (i = 0; i < 4; ++i) {

        if (*fen == ' ')
            break;

        switch (*fen) {
            case 'K': pos->castlePerm |= WKCA; break;
            case 'Q': pos->castlePerm |= WQCA; break;
            case 'k': pos->castlePerm |= BKCA; break;
            case 'q': pos->castlePerm |= BQCA; break;
            default: break;
        }
        fen++;
    }
    fen++;

    assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);

    // En passant square
    if (*fen != '-') {
        file = fen[0] - 'a';
        rank = fen[1] - '1';

        assert(file >= FILE_A && file <= FILE_H);
        assert(rank == RANK_3 || rank == RANK_6);

        pos->enPas = (8 * rank) + file;
    }

    // Update the rest of position to match pos->board
    UpdatePosition(pos);

    assert(CheckBoard(pos));
}

// Print the board with misc info
void PrintBoard(const Position *pos) {

    const char PceChar[]  = ".pnbrqk..PNBRQK";
    const char SideChar[] = "bw-";
    int sq, file, rank, piece;

    printf("\nGame Board:\n\n");

    for (rank = RANK_8; rank >= RANK_1; --rank) {
        printf("%d  ", rank + 1);
        for (file = FILE_A; file <= FILE_H; ++file) {
            sq = (rank * 8) + file;
            piece = pos->board[sq];
            printf("%3c", PceChar[piece]);
        }
        printf("\n");
    }

    printf("\n   ");
    for (file = FILE_A; file <= FILE_H; ++file)
        printf("%3c", 'a' + file);

    printf("\n");
    printf("side: %c\n", SideChar[pos->side]);
    printf("enPas: %d\n", pos->enPas);
    printf("castle: %c%c%c%c\n",
           pos->castlePerm & WKCA ? 'K' : '-',
           pos->castlePerm & WQCA ? 'Q' : '-',
           pos->castlePerm & BKCA ? 'k' : '-',
           pos->castlePerm & BQCA ? 'q' : '-');
    printf("PosKey: %" PRIu64 "\n", pos->posKey);
    fflush(stdout);
}

#ifndef NDEBUG
// Check board state makes sense
bool CheckBoard(const Position *pos) {

    int t_pieceCounts[PIECE_NB] = { 0 };
    int t_bigPieces[2] = { 0, 0 };

    int sq, t_piece, t_pce_num, color;

    // Bitboards
    assert(PopCount(pos->pieceBB[KING]) == 2);

    assert(PopCount(pos->pieceBB[  PAWN] & pos->colorBB[WHITE]) <= 8);
    assert(PopCount(pos->pieceBB[KNIGHT] & pos->colorBB[WHITE]) <= 10);
    assert(PopCount(pos->pieceBB[BISHOP] & pos->colorBB[WHITE]) <= 10);
    assert(PopCount(pos->pieceBB[  ROOK] & pos->colorBB[WHITE]) <= 10);
    assert(PopCount(pos->pieceBB[ QUEEN] & pos->colorBB[WHITE]) <= 9);
    assert(PopCount(pos->pieceBB[  KING] & pos->colorBB[WHITE]) == 1);

    assert(PopCount(pos->pieceBB[  PAWN] & pos->colorBB[BLACK]) <= 8);
    assert(PopCount(pos->pieceBB[KNIGHT] & pos->colorBB[BLACK]) <= 10);
    assert(PopCount(pos->pieceBB[BISHOP] & pos->colorBB[BLACK]) <= 10);
    assert(PopCount(pos->pieceBB[  ROOK] & pos->colorBB[BLACK]) <= 10);
    assert(PopCount(pos->pieceBB[ QUEEN] & pos->colorBB[BLACK]) <= 9);
    assert(PopCount(pos->pieceBB[  KING] & pos->colorBB[BLACK]) == 1);

    assert(PopCount(pos->pieceBB[  PAWN] & pos->colorBB[WHITE]) == pos->pieceCounts[wP]);
    assert(PopCount(pos->pieceBB[KNIGHT] & pos->colorBB[WHITE]) == pos->pieceCounts[wN]);
    assert(PopCount(pos->pieceBB[BISHOP] & pos->colorBB[WHITE]) == pos->pieceCounts[wB]);
    assert(PopCount(pos->pieceBB[  ROOK] & pos->colorBB[WHITE]) == pos->pieceCounts[wR]);
    assert(PopCount(pos->pieceBB[ QUEEN] & pos->colorBB[WHITE]) == pos->pieceCounts[wQ]);

    assert(PopCount(pos->pieceBB[  PAWN] & pos->colorBB[BLACK]) == pos->pieceCounts[bP]);
    assert(PopCount(pos->pieceBB[KNIGHT] & pos->colorBB[BLACK]) == pos->pieceCounts[bN]);
    assert(PopCount(pos->pieceBB[BISHOP] & pos->colorBB[BLACK]) == pos->pieceCounts[bB]);
    assert(PopCount(pos->pieceBB[  ROOK] & pos->colorBB[BLACK]) == pos->pieceCounts[bR]);
    assert(PopCount(pos->pieceBB[ QUEEN] & pos->colorBB[BLACK]) == pos->pieceCounts[bQ]);

    assert(pos->pieceBB[ALL] == (pos->colorBB[WHITE] | pos->colorBB[BLACK]));

    // check piece lists
    for (t_piece = PIECE_MIN; t_piece < PIECE_NB; ++t_piece)
        for (t_pce_num = 0; t_pce_num < pos->pieceCounts[t_piece]; ++t_pce_num) {
            sq = pos->pieceList[t_piece][t_pce_num];
            assert(pos->board[sq] == t_piece);
        }

    // check piece count and other counters
    for (sq = A1; sq <= H8; ++sq) {

        t_piece = pos->board[sq];
        t_pieceCounts[t_piece]++;
        color = colorOf(t_piece);

        if (pieceBig[t_piece]) t_bigPieces[color]++;
    }

    for (t_piece = PIECE_MIN; t_piece < PIECE_NB; ++t_piece)
        assert(t_pieceCounts[t_piece] == pos->pieceCounts[t_piece]);

    assert(t_bigPieces[WHITE] == pos->bigPieces[WHITE] && t_bigPieces[BLACK] == pos->bigPieces[BLACK]);

    assert(pos->side == WHITE || pos->side == BLACK);

    assert(pos->enPas == NO_SQ
       || (pos->enPas >= 40 && pos->enPas < 48 && pos->side == WHITE)
       || (pos->enPas >= 16 && pos->enPas < 24 && pos->side == BLACK));

    assert(pos->board[pos->kingSq[WHITE]] == wK);
    assert(pos->board[pos->kingSq[BLACK]] == bK);

    assert(pos->castlePerm >= 0 && pos->castlePerm <= 15);

    assert(GeneratePosKey(pos) == pos->posKey);

    return true;
}
#endif

#ifdef DEV
// Reverse the colors
void MirrorBoard(Position *pos) {

    assert(CheckBoard(pos));

    int SwapPiece[PIECE_NB] = {EMPTY, wP, wN, wB, wR, wQ, wK, EMPTY, EMPTY, bP, bN, bB, bR, bQ, bK, EMPTY};
    int tempPiecesArray[64];
    int tempEnPas, tempCastlePerm, tempSide, sq;

    // Save the necessary position info mirrored
    for (sq = A1; sq <= H8; ++sq)
        tempPiecesArray[sq] = SwapPiece[pos->board[mirror_square[sq]]];

    tempSide  = !pos->side;
    tempEnPas = pos->enPas == NO_SQ ? NO_SQ : mirror_square[pos->enPas];
    tempCastlePerm = 0;
    if (pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
    if (pos->castlePerm & WQCA) tempCastlePerm |= BQCA;
    if (pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
    if (pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

    // Clear the position
    ClearPosition(pos);

    // Fill in the mirrored position info
    for (sq = A1; sq <= H8; ++sq)
        pos->board[sq] = tempPiecesArray[sq];

    pos->side  = tempSide;
    pos->enPas = tempEnPas;
    pos->castlePerm = tempCastlePerm;

    // Update the rest of the position to match pos->board
    UpdatePosition(pos);

    assert(CheckBoard(pos));
}
#endif