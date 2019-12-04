// movegen.c

#include <stdio.h>

#include "attack.h"
#include "board.h"
#include "bitboards.h"
#include "makemove.h"
#include "move.h"
#include "validate.h"


enum { QUIET, NOISY };
enum { ENPAS, PROMO };

static int MvvLvaScores[PIECE_NB][PIECE_NB];

// Initializes the MostValuableVictim-LeastValuableAttacker scores used for ordering captures
CONSTR InitMvvLva() {

    const int VictimScore[PIECE_NB]   = {0, 106, 206, 306, 406, 506, 606, 0, 0, 106, 206, 306, 406, 506, 606, 0};
    const int AttackerScore[PIECE_NB] = {0,   1,   2,   3,   4,   5,   6, 0, 0,   1,   2,   3,   4,   5,   6, 0};

    for (int Attacker = PIECE_MIN; Attacker < PIECE_NB; ++Attacker)
        for (int Victim = PIECE_MIN; Victim < PIECE_NB; ++Victim)
            MvvLvaScores[Victim][Attacker] = VictimScore[Victim] - AttackerScore[Attacker];
}

// Constructs and adds a move to the move list
INLINE void AddMove(const Position *pos, MoveList *list, const int from, const int to, const int promo, const int flag, const int type) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));

    int *moveScore = &list->moves[list->count].score;

    const int captured = pos->board[to];
    const int move = MOVE(from, to, captured, promo, flag);

    // Add scores to help move ordering based on search history heuristics / mvvlva
    if (type == NOISY)
        *moveScore = MvvLvaScores[captured][pos->board[from]];

    if (type == QUIET) {
        if (pos->searchKillers[pos->ply][0] == move)
            *moveScore = 900000;
        else if (pos->searchKillers[pos->ply][1] == move)
            *moveScore = 800000;
        else
            *moveScore = pos->searchHistory[pos->board[from]][to];
    }

    list->moves[list->count++].move = move;
}
INLINE void AddSpecialPawn(const Position *pos, MoveList *list, const int from, const int to, const int color, const int movetype, const int type) {

    assert(ValidSquare(from));
    assert(ValidSquare(to));

    if (movetype == ENPAS) {
        int move = MOVE(from, to, EMPTY, EMPTY, FLAG_ENPAS);
        list->moves[list->count].move = move;
        list->moves[list->count].score = 105;
        list->count++;
    }
    if (movetype == PROMO) {
        if (type == NOISY)
            AddMove(pos, list, from, to, makePiece(color, QUEEN ), FLAG_NONE, NOISY);
        if (type == QUIET) {
            AddMove(pos, list, from, to, makePiece(color, KNIGHT), FLAG_NONE, NOISY);
            AddMove(pos, list, from, to, makePiece(color, ROOK  ), FLAG_NONE, NOISY);
            AddMove(pos, list, from, to, makePiece(color, BISHOP), FLAG_NONE, NOISY);
        }
    }
}

// Castling is a mess due to testing most of the legality here, rather than in makemove
INLINE void GenCastling(const Position *pos, MoveList *list, const int color, const int type) {

    if (type != QUIET) return;

    const int KCA = color == WHITE ? WKCA : BKCA;
    const int QCA = color == WHITE ? WQCA : BQCA;
    const Bitboard kingbits  = color == WHITE ? bitF1G1 : bitF8G8;
    const Bitboard queenbits = color == WHITE ? bitB1C1D1 : bitB8C8D8;
    const int from = color == WHITE ? E1 : E8;
    const int ksto = color == WHITE ? G1 : G8;
    const int qsto = color == WHITE ? C1 : C8;
    const int ksmiddle = color == WHITE ? F1 : F8;
    const int qsmiddle = color == WHITE ? D1 : D8;

    const Bitboard occupied = pos->pieceBB[ALL];

    // King side castle
    if (pos->castlePerm & KCA)
        if (!(occupied & kingbits))
            if (   !SqAttacked(from, !color, pos)
                && !SqAttacked(ksmiddle, !color, pos))
                AddMove(pos, list, from, ksto, EMPTY, FLAG_CASTLE, QUIET);

    // Queen side castle
    if (pos->castlePerm & QCA)
        if (!(occupied & queenbits))
            if (   !SqAttacked(from, !color, pos)
                && !SqAttacked(qsmiddle, !color, pos))
                AddMove(pos, list, from, qsto, EMPTY, FLAG_CASTLE, QUIET);
}

// Returns a square behind (relative to color)
// 7 : diagonally to the left
// 8 : directly behind
// 9 : diagonally to the right
INLINE int relBackward(const int color, const int sq, const int diff) {
    return color == WHITE ? sq - diff : sq + diff;
}

// Pawns are a mess
INLINE void GenPawn(const Position *pos, MoveList *list, const int color, const int type) {

    int sq;

    const Bitboard empty   = ~pos->pieceBB[ALL];
    const Bitboard enemies =  pos->colorBB[!color];
    const Bitboard pawns   =  pos->colorBB[ color] & pos->pieceBB[PAWN];

    Bitboard relRank7BB = color == WHITE ? rank7BB : rank2BB;

    Bitboard on7th  = pawns & relRank7BB;
    Bitboard not7th = pawns ^ on7th;

    // Normal moves forward
    if (type == QUIET) {

        Bitboard pawnMoves  = color == WHITE ? empty & not7th << 8
                                    : empty & not7th >> 8;

        Bitboard pawnStarts = color == WHITE ? empty & (pawnMoves & rank3BB) << 8
                                    : empty & (pawnMoves & rank6BB) >> 8;

        // Normal pawn moves
        while (pawnMoves) {
            sq = PopLsb(&pawnMoves);
            AddMove(pos, list, relBackward(color, sq, 8), sq, EMPTY, FLAG_NONE, QUIET);
        }
        // Pawn starts
        while (pawnStarts) {
            sq = PopLsb(&pawnStarts);
            AddMove(pos, list, relBackward(color, sq, 16), sq, EMPTY, FLAG_PAWNSTART, QUIET);
        }
    }

    // Promotions
    if (on7th) {

        Bitboard lPromoCap = color == WHITE ? ((on7th & ~fileABB) << 7) & enemies
                                            : ((on7th & ~fileHBB) >> 7) & enemies;

        Bitboard rPromoCap = color == WHITE ? ((on7th & ~fileHBB) << 9) & enemies
                                            : ((on7th & ~fileABB) >> 9) & enemies;

        Bitboard promotions = color == WHITE ? (on7th << 8) & empty
                                             : (on7th >> 8) & empty;

        // Promoting captures
        while (lPromoCap) {
            sq = PopLsb(&lPromoCap);
            AddSpecialPawn(pos, list, relBackward(color, sq, 7), sq, color, PROMO, type);
        }
        while (rPromoCap) {
            sq = PopLsb(&rPromoCap);
            AddSpecialPawn(pos, list, relBackward(color, sq, 9), sq, color, PROMO, type);
        }
        // Promotions
        while (promotions) {
            sq = PopLsb(&promotions);
            AddSpecialPawn(pos, list, relBackward(color, sq, 8), sq, color, PROMO, type);
        }
    }
    // Captures
    if (type == NOISY) {

        Bitboard lAttacks = color == WHITE ? ((not7th & ~fileABB) << 7) & enemies
                                           : ((not7th & ~fileHBB) >> 7) & enemies;

        Bitboard rAttacks = color == WHITE ? ((not7th & ~fileHBB) << 9) & enemies
                                           : ((not7th & ~fileABB) >> 9) & enemies;

        while (lAttacks) {
            sq = PopLsb(&lAttacks);
            AddMove(pos, list, relBackward(color, sq, 7), sq, EMPTY, FLAG_NONE, NOISY);
        }
        while (rAttacks) {
            sq = PopLsb(&rAttacks);
            AddMove(pos, list, relBackward(color, sq, 9), sq, EMPTY, FLAG_NONE, NOISY);
        }
        // En passant
        if (pos->enPas != NO_SQ) {
            Bitboard enPassers = not7th & pawn_attacks[!color][pos->enPas];
            while (enPassers)
                AddSpecialPawn(pos, list, PopLsb(&enPassers), pos->enPas, color, ENPAS, NOISY);
        }
    }
}

// Knight, bishop, rook and queen
INLINE void GenPieceType(const Position *pos, MoveList *list, const int color, const int type, const int pt) {

    int sq;
    Bitboard moves;

    const Bitboard occupied = pos->pieceBB[ALL];
    const Bitboard enemies  = pos->colorBB[!color];
    const Bitboard targets  = type == NOISY ? enemies : ~occupied;

    Bitboard pieces = pos->colorBB[color] & pos->pieceBB[pt];

    while (pieces) {

        sq = PopLsb(&pieces);

        if (pt == KNIGHT) moves = targets & knight_attacks[sq];
        if (pt == BISHOP) moves = targets & BishopAttacks(sq, occupied);
        if (pt == ROOK)   moves = targets & RookAttacks(sq, occupied);
        if (pt == QUEEN)  moves = targets & (BishopAttacks(sq, occupied) | RookAttacks(sq, occupied));
        if (pt == KING)   moves = targets & king_attacks[sq];

        while (moves)
            AddMove(pos, list, sq, PopLsb(&moves), EMPTY, 0, type);
    }
}

// Generate moves
static void GenMoves(const Position *pos, MoveList *list, const int color, const int type) {

    assert(CheckBoard(pos));

    GenCastling (pos, list, color, type);
    GenPawn     (pos, list, color, type);
    GenPieceType(pos, list, color, type, KNIGHT);
    GenPieceType(pos, list, color, type, ROOK);
    GenPieceType(pos, list, color, type, BISHOP);
    GenPieceType(pos, list, color, type, QUEEN);
    GenPieceType(pos, list, color, type, KING);

    assert(MoveListOk(list, pos));
}

// Generate all quiet moves
void GenQuietMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, pos->side, QUIET);
}

// Generate all noisy moves
void GenNoisyMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, pos->side, NOISY);
}

// Generate all pseudo legal moves
void GenAllMoves(const Position *pos, MoveList *list) {

    list->count = list->next = 0;

    GenNoisyMoves(pos, list);
    GenQuietMoves(pos, list);
}