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

    const int move = MOVE(from, to, pieceOn(to), promo, flag);

    // Add scores to help move ordering based on search history heuristics / mvvlva
    if (type == NOISY)
        *moveScore = MvvLvaScores[pieceOn(to)][pieceOn(from)];

    if (type == QUIET) {
        if (killer1 == move)
            *moveScore = 900000;
        else if (killer2 == move)
            *moveScore = 800000;
        else
            *moveScore = pos->searchHistory[pieceOn(from)][to];
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
            AddMove(pos, list, from, to, MakePiece(color, QUEEN ), FLAG_NONE, NOISY);
        if (type == QUIET) {
            AddMove(pos, list, from, to, MakePiece(color, KNIGHT), FLAG_NONE, NOISY);
            AddMove(pos, list, from, to, MakePiece(color, ROOK  ), FLAG_NONE, NOISY);
            AddMove(pos, list, from, to, MakePiece(color, BISHOP), FLAG_NONE, NOISY);
        }
    }
}

// Castling is now a bit less of a mess
INLINE void GenCastling(const Position *pos, MoveList *list, const int color, const int type) {

    if (type != QUIET) return;

    const int from = color == WHITE ? E1   : E8;
    const int KCA  = color == WHITE ? WKCA : BKCA;
    const int QCA  = color == WHITE ? WQCA : BQCA;
    const Bitboard betweenKS = color == WHITE ? bitF1G1   : bitF8G8;
    const Bitboard betweenQS = color == WHITE ? bitB1C1D1 : bitB8C8D8;

    // King side castle
    if (CastlePseudoLegal(pos, betweenKS, KCA, from, from+1, color))
        AddMove(pos, list, from, from+2, EMPTY, FLAG_CASTLE, QUIET);

    // Queen side castle
    if (CastlePseudoLegal(pos, betweenQS, QCA, from, from-1, color))
        AddMove(pos, list, from, from-2, EMPTY, FLAG_CASTLE, QUIET);
}

// Pawns are a mess
INLINE void GenPawn(const Position *pos, MoveList *list, const int color, const int type) {

    const int up    = (color == WHITE ? NORTH : SOUTH);
    const int left  = (color == WHITE ? WEST  : EAST);
    const int right = (color == WHITE ? EAST  : WEST);

    int sq;

    const Bitboard empty   = ~pieceBB(ALL);
    const Bitboard enemies =  colorBB(!color);
    const Bitboard pawns   =  colorBB( color) & pieceBB(PAWN);

    Bitboard on7th  = pawns & RankBB[RelativeRank(color, RANK_7)];
    Bitboard not7th = pawns ^ on7th;

    // Normal moves forward
    if (type == QUIET) {

        Bitboard pawnMoves  = empty & ShiftBB(up, not7th);
        Bitboard pawnStarts = empty & ShiftBB(up, pawnMoves)
                                    & RankBB[RelativeRank(color, RANK_4)];

        // Normal pawn moves
        while (pawnMoves) {
            sq = PopLsb(&pawnMoves);
            AddMove(pos, list, sq - up, sq, EMPTY, FLAG_NONE, QUIET);
        }
        // Pawn starts
        while (pawnStarts) {
            sq = PopLsb(&pawnStarts);
            AddMove(pos, list, sq - up * 2, sq, EMPTY, FLAG_PAWNSTART, QUIET);
        }
    }

    // Promotions
    if (on7th) {

        Bitboard promotions = empty & ShiftBB(up, on7th);
        Bitboard lPromoCap = enemies & ShiftBB(up+left, on7th);
        Bitboard rPromoCap = enemies & ShiftBB(up+right, on7th);

        // Promoting captures
        while (lPromoCap) {
            sq = PopLsb(&lPromoCap);
            AddSpecialPawn(pos, list, sq - (up+left), sq, color, PROMO, type);
        }
        while (rPromoCap) {
            sq = PopLsb(&rPromoCap);
            AddSpecialPawn(pos, list, sq - (up+right), sq, color, PROMO, type);
        }
        // Promotions
        while (promotions) {
            sq = PopLsb(&promotions);
            AddSpecialPawn(pos, list, sq - up, sq, color, PROMO, type);
        }
    }
    // Captures
    if (type == NOISY) {

        Bitboard lAttacks = enemies & ShiftBB(up+left, not7th);
        Bitboard rAttacks = enemies & ShiftBB(up+right, not7th);

        while (lAttacks) {
            sq = PopLsb(&lAttacks);
            AddMove(pos, list, sq - (up+left), sq, EMPTY, FLAG_NONE, NOISY);
        }
        while (rAttacks) {
            sq = PopLsb(&rAttacks);
            AddMove(pos, list, sq - (up+right), sq, EMPTY, FLAG_NONE, NOISY);
        }
        // En passant
        if (pos->enPas != NO_SQ) {
            Bitboard enPassers = not7th & PawnAttackBB(!color, pos->enPas);
            while (enPassers)
                AddSpecialPawn(pos, list, PopLsb(&enPassers), pos->enPas, color, ENPAS, NOISY);
        }
    }
}

// Knight, bishop, rook and queen
INLINE void GenPieceType(const Position *pos, MoveList *list, const int color, const int type, const int pt) {

    int sq;
    Bitboard moves;

    const Bitboard occupied = pieceBB(ALL);
    const Bitboard enemies  = colorBB(!color);
    const Bitboard targets  = type == NOISY ? enemies : ~occupied;

    Bitboard pieces = colorBB(color) & pieceBB(pt);

    while (pieces) {

        sq = PopLsb(&pieces);

        moves = targets & AttackBB(pt, sq, occupied);

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

    GenMoves(pos, list, sideToMove(), QUIET);
}

// Generate all noisy moves
void GenNoisyMoves(const Position *pos, MoveList *list) {

    GenMoves(pos, list, sideToMove(), NOISY);
}

// Generate all pseudo legal moves
void GenAllMoves(const Position *pos, MoveList *list) {

    list->count = list->next = 0;

    GenNoisyMoves(pos, list);
    GenQuietMoves(pos, list);
}