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
const int NonPawn[PIECE_NB]    = { false, false,  true,  true,  true,  true, false, false, false, false,  true,  true,  true,  true, false, false };
const int PiecePawn[PIECE_NB]  = { false,  true, false, false, false, false, false, false, false,  true, false, false, false, false, false, false };
const int PhaseValue[PIECE_NB] = {     0,     0,     1,     1,     2,     4,     0,     0,     0,     0,     1,     1,     2,     4,     0,     0 };


// Initialize distance lookup table
CONSTR InitDistance() {

    for (int sq1 = A1; sq1 <= H8; ++sq1)
        for (int sq2 = A1; sq2 <= H8; ++sq2) {
            int vertical   = abs(RankOf(sq1) - RankOf(sq2));
            int horizontal = abs(FileOf(sq1) - FileOf(sq2));
            SqDistance[sq1][sq2] = MAX(vertical, horizontal);
        }
}

// Update the rest of a position to match pos->board
static void UpdatePosition(Position *pos) {

    int sq, piece, color;

    // Generate the position key
    pos->posKey = GeneratePosKey(pos);

    // Loop through each square on the board
    for (sq = A1; sq <= H8; ++sq) {

        piece = pieceOn(sq);

        // If it isn't empty we update the relevant lists
        if (piece != EMPTY) {

            color = ColorOf(piece);

            // Bitboards
            SETBIT(pieceBB(ALL), sq);
            SETBIT(colorBB(ColorOf(piece)), sq);
            SETBIT(pieceBB(PieceTypeOf(piece)), sq);

            // Non pawn piece count
            if (NonPawn[piece])
                pos->nonPawns[color]++;

            // Material score
            pos->material += PSQT[piece][sq];

            // Phase
            pos->basePhase -= PhaseValue[piece];

            // Piece list
            pos->index[sq] = pos->pieceCounts[piece]++;
            pos->pieceList[piece][pos->index[sq]] = sq;
        }
    }

    pos->phase = (pos->basePhase * 256 + 12) / 24;

    assert(CheckBoard(pos));
}

// Clears the board
static void ClearPosition(Position *pos) {

    // Array representation
    memset(pos->board, EMPTY, sizeof(pos->board));

    // Bitboard representations
    memset(pos->colorBB, 0ULL, sizeof(pos->colorBB));
    memset(pos->pieceBB, 0ULL, sizeof(pos->pieceBB));

    // Piece list
    memset(pos->pieceCounts, 0, sizeof(pos->pieceCounts));
    memset(pos->pieceList,   0, sizeof(pos->pieceList));
    memset(pos->index,       0, sizeof(pos->index));

    // Big piece counts
    pos->nonPawns[BLACK] = pos->nonPawns[WHITE] = 0;

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

    ClearPosition(pos);

    int piece;
    int count = 0;
    int sq = A8;

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
            // New rank
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
        sq++;

        // Skip count-1 extra squares
        for (; count > 1; count--)
            sq++;

        fen++;
    }
    fen++;

    // Side to move
    assert(*fen == 'w' || *fen == 'b');
    sideToMove() = (*fen == 'w') ? WHITE : BLACK;
    fen += 2;

    // Castling rights
    while (*fen != ' ') {

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

    // En passant square
    if (*fen != '-') {
        int file = fen[0] - 'a';
        int rank = fen[1] - '1';

        pos->enPas = (8 * rank) + file;
    }
    fen += 2;

    // 50 move rule
    pos->fiftyMove = atoi(fen);

    // Update the rest of position to match pos->board
    UpdatePosition(pos);

    assert(CheckBoard(pos));
}

#if defined DEV || !defined NDEBUG
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
            piece = pieceOn(sq);
            printf("%3c", PceChar[piece]);
        }
        printf("\n");
    }

    printf("\n   ");
    for (file = FILE_A; file <= FILE_H; ++file)
        printf("%3c", 'a' + file);

    printf("\n");
    printf("side: %c\n", SideChar[sideToMove()]);
    printf("enPas: %d\n", pos->enPas);
    printf("castle: %c%c%c%c\n",
           pos->castlePerm & WKCA ? 'K' : '-',
           pos->castlePerm & WQCA ? 'Q' : '-',
           pos->castlePerm & BKCA ? 'k' : '-',
           pos->castlePerm & BQCA ? 'q' : '-');
    printf("PosKey: %" PRIu64 "\n", pos->posKey);
    fflush(stdout);
}
#endif

#ifndef NDEBUG
// Check board state makes sense
bool CheckBoard(const Position *pos) {

    assert(0 <= pos->hisPly && pos->hisPly < MAXGAMEMOVES);
    assert(   0 <= pos->ply && pos->ply < MAXDEPTH);

    int t_pieceCounts[PIECE_NB] = { 0 };
    int t_bigPieces[2] = { 0, 0 };

    int sq, t_piece, t_pce_num, color;

    // Bitboards
    assert(PopCount(pieceBB(KING)) == 2);

    assert(PopCount(pieceBB(PAWN)   & colorBB(WHITE)) <= 8);
    assert(PopCount(pieceBB(KNIGHT) & colorBB(WHITE)) <= 10);
    assert(PopCount(pieceBB(BISHOP) & colorBB(WHITE)) <= 10);
    assert(PopCount(pieceBB(ROOK)   & colorBB(WHITE)) <= 10);
    assert(PopCount(pieceBB(QUEEN)  & colorBB(WHITE)) <= 9);
    assert(PopCount(pieceBB(KING)   & colorBB(WHITE)) == 1);

    assert(PopCount(pieceBB(PAWN)   & colorBB(BLACK)) <= 8);
    assert(PopCount(pieceBB(KNIGHT) & colorBB(BLACK)) <= 10);
    assert(PopCount(pieceBB(BISHOP) & colorBB(BLACK)) <= 10);
    assert(PopCount(pieceBB(ROOK)   & colorBB(BLACK)) <= 10);
    assert(PopCount(pieceBB(QUEEN)  & colorBB(BLACK)) <= 9);
    assert(PopCount(pieceBB(KING)   & colorBB(BLACK)) == 1);

    assert(PopCount(pieceBB(PAWN)   & colorBB(WHITE)) == pos->pieceCounts[wP]);
    assert(PopCount(pieceBB(KNIGHT) & colorBB(WHITE)) == pos->pieceCounts[wN]);
    assert(PopCount(pieceBB(BISHOP) & colorBB(WHITE)) == pos->pieceCounts[wB]);
    assert(PopCount(pieceBB(ROOK)   & colorBB(WHITE)) == pos->pieceCounts[wR]);
    assert(PopCount(pieceBB(QUEEN)  & colorBB(WHITE)) == pos->pieceCounts[wQ]);

    assert(PopCount(pieceBB(PAWN)   & colorBB(BLACK)) == pos->pieceCounts[bP]);
    assert(PopCount(pieceBB(KNIGHT) & colorBB(BLACK)) == pos->pieceCounts[bN]);
    assert(PopCount(pieceBB(BISHOP) & colorBB(BLACK)) == pos->pieceCounts[bB]);
    assert(PopCount(pieceBB(ROOK)   & colorBB(BLACK)) == pos->pieceCounts[bR]);
    assert(PopCount(pieceBB(QUEEN)  & colorBB(BLACK)) == pos->pieceCounts[bQ]);

    assert(pieceBB(ALL) == (colorBB(WHITE) | colorBB(BLACK)));

    // check piece lists
    for (t_piece = PIECE_MIN; t_piece < PIECE_NB; ++t_piece)
        for (t_pce_num = 0; t_pce_num < pos->pieceCounts[t_piece]; ++t_pce_num) {
            sq = pos->pieceList[t_piece][t_pce_num];
            assert(pieceOn(sq) == t_piece);
        }

    // check piece count and other counters
    for (sq = A1; sq <= H8; ++sq) {

        t_piece = pieceOn(sq);
        t_pieceCounts[t_piece]++;
        color = ColorOf(t_piece);

        if (NonPawn[t_piece]) t_bigPieces[color]++;
    }

    for (t_piece = PIECE_MIN; t_piece < PIECE_NB; ++t_piece)
        assert(t_pieceCounts[t_piece] == pos->pieceCounts[t_piece]);

    assert(t_bigPieces[WHITE] == pos->nonPawns[WHITE] && t_bigPieces[BLACK] == pos->nonPawns[BLACK]);

    assert(sideToMove() == WHITE || sideToMove() == BLACK);

    assert(pos->enPas == NO_SQ
       || (RelativeRank(sideToMove(), RankOf(pos->enPas)) == RANK_6));

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
        tempPiecesArray[sq] = SwapPiece[pieceOn(MirrorSquare(sq))];

    tempSide  = !sideToMove();
    tempEnPas = pos->enPas == NO_SQ ? NO_SQ : MirrorSquare(pos->enPas);
    tempCastlePerm = 0;
    if (pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
    if (pos->castlePerm & WQCA) tempCastlePerm |= BQCA;
    if (pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
    if (pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

    // Clear the position
    ClearPosition(pos);

    // Fill in the mirrored position info
    for (sq = A1; sq <= H8; ++sq)
        pieceOn(sq) = tempPiecesArray[sq];

    sideToMove() = tempSide;
    pos->enPas   = tempEnPas;
    pos->castlePerm = tempCastlePerm;

    // Update the rest of the position to match pos->board
    UpdatePosition(pos);

    assert(CheckBoard(pos));
}
#endif