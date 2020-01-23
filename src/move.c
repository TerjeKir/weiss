// io.c

#include <stdio.h>

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"
#include "validate.h"


// Checks whether a move is pseudo-legal (assuming it is pseudo-legal in some position)
bool MoveIsPseudoLegal(const Position *pos, const int move) {

    const int from  = fromSq(move);
    const int to    = toSq(move);
    const int piece = pieceOn(from);
    const int color = colorOf(piece);

    const int capt1 = capturing(move);
    const int capt2 = pieceOn(to);

    // Easy sanity tests
    if (   piece == EMPTY
        || color != sideToMove()
        || capt1 != capt2
        || move  == NOMOVE)
        return false;

    const Bitboard occupied = pieceBB(ALL);
    const Bitboard toBB     = 1ULL << to;

    // Make sure the piece at 'from' can move to 'to' (ignoring pins/moving into check)
    switch (pieceTypeOf(piece)) {
        case KNIGHT: return toBB & knight_attacks[from];
        case BISHOP: return toBB & BishopAttacks(from, occupied);
        case ROOK  : return toBB &   RookAttacks(from, occupied);
        case QUEEN : return toBB & (BishopAttacks(from, occupied) | RookAttacks(from, occupied));
        case PAWN  :
            if (move & FLAG_ENPAS)
                return to == pos->enPas;
            if (move & FLAG_PAWNSTART)
                return pieceOn(to + 8 - 16 * color) == EMPTY;
            if (capt1)
                return toBB & pawn_attacks[color][from];
            return (to + 8 - 16 * color) == from;
        case KING  :
            if (move & FLAG_CASTLE)
                switch (to) {
                    case C1: return    (pos->castlePerm & WQCA)
                                    && !(occupied & bitB1C1D1)
                                    && !SqAttacked(E1, BLACK, pos)
                                    && !SqAttacked(D1, BLACK, pos);
                    case G1: return    (pos->castlePerm & WKCA)
                                    && !(occupied & bitF1G1)
                                    && !SqAttacked(E1, BLACK, pos)
                                    && !SqAttacked(F1, BLACK, pos);
                    case C8: return    (pos->castlePerm & BQCA)
                                    && !(occupied & bitB8C8D8)
                                    && !SqAttacked(E8, WHITE, pos)
                                    && !SqAttacked(D8, WHITE, pos);
                    case G8: return    (pos->castlePerm & BKCA)
                                    && !(occupied & bitF8G8)
                                    && !SqAttacked(E8, WHITE, pos)
                                    && !SqAttacked(F8, WHITE, pos);
                }
            return toBB & king_attacks[from];
    }
    return false;
}

// Translates a move to a string
char *MoveToStr(const int move) {

    static char moveStr[6];
    char pchar;

    int ff = fileOf(fromSq(move));
    int rf = rankOf(fromSq(move));
    int ft = fileOf(toSq(move));
    int rt = rankOf(toSq(move));

    int promo = pieceTypeOf(promotion(move));

    if (promo) {

        if (     promo == QUEEN)
            pchar = 'q';
        else if (promo == KNIGHT)
            pchar = 'n';
        else if (promo == ROOK)
            pchar = 'r';
        else           // BISHOP
            pchar = 'b';

        sprintf(moveStr, "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);
    } else
        sprintf(moveStr, "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));

    return moveStr;
}

// Translates a string to a move
// a2a4 b7b8q g7f8n
int ParseMove(const char *ptrChar, Position *pos) {

    assert(CheckBoard(pos));

    // Make sure coordinates make sense (should maybe give an error)
    if (ptrChar[1] > '8' || ptrChar[1] < '1') return NOMOVE;
    if (ptrChar[3] > '8' || ptrChar[3] < '1') return NOMOVE;
    if (ptrChar[0] > 'h' || ptrChar[0] < 'a') return NOMOVE;
    if (ptrChar[2] > 'h' || ptrChar[2] < 'a') return NOMOVE;

    // Translate coordinates into square numbers
    int from = (ptrChar[0] - 'a') + (8 * (ptrChar[1] - '1'));
    int to   = (ptrChar[2] - 'a') + (8 * (ptrChar[3] - '1'));

    MoveList list[1];
    GenAllMoves(pos, list);

    // Make sure the move is legal in the current position
    // by checking that it is generated by movegen
    for (unsigned int moveNum = 0; moveNum < list->count; ++moveNum) {

        int move = list->moves[moveNum].move;

        if (fromSq(move) == from && toSq(move) == to) {

            int promo = pieceTypeOf(promotion(move));

            if (promo) {

                if (     promo == QUEEN  && ptrChar[4] == 'q')
                    return move;
                else if (promo == KNIGHT && ptrChar[4] == 'n')
                    return move;
                else if (promo == ROOK   && ptrChar[4] == 'r')
                    return move;
                else if (promo == BISHOP && ptrChar[4] == 'b')
                    return move;

                continue;
            }
            return move;
        }
    }
    return NOMOVE;
}

#ifdef DEV
// Translates a string from an .epd file to a move
// Nf3-g1+, f3-g1+, Nf3-g1, f3-g1N+
int ParseEPDMove(const char *ptrChar, Position *pos) {

    assert(CheckBoard(pos));

    // Skip piece indicator as it's not needed
    if ('B' <= ptrChar[0] && ptrChar[0] <= 'R')
        ptrChar++;

    // Make sure coordinates make sense (should maybe give an error)
    if (ptrChar[1] > '8' || ptrChar[1] < '1') return NOMOVE;
    if (ptrChar[4] > '8' || ptrChar[4] < '1') return NOMOVE;
    if (ptrChar[0] > 'h' || ptrChar[0] < 'a') return NOMOVE;
    if (ptrChar[3] > 'h' || ptrChar[3] < 'a') return NOMOVE;

    // Translate coordinates into square numbers
    int from = (ptrChar[0] - 'a') + (8 * (ptrChar[1] - '1'));
    int to   = (ptrChar[3] - 'a') + (8 * (ptrChar[4] - '1'));

    // Make sure the move is legal in the current position
    // by checking that it is generated by movegen
    MoveList list[1];
    GenAllMoves(pos, list);

    for (unsigned int moveNum = 0; moveNum < list->count; ++moveNum) {

        int move = list->moves[moveNum].move;

        if (fromSq(move) == from && toSq(move) == to) {

            int promo = pieceTypeOf(promotion(move));

            if (promo) {

                if (     promo == QUEEN  && ptrChar[5] == 'Q')
                    return move;
                else if (promo == KNIGHT && ptrChar[5] == 'N')
                    return move;
                else if (promo == ROOK   && ptrChar[5] == 'R')
                    return move;
                else if (promo == BISHOP && ptrChar[5] == 'B')
                    return move;

                continue;
            }
            return move;
        }
    }
    return NOMOVE;
}
#endif