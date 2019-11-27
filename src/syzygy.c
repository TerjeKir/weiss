// syzygy.c

#include "fathom/tbprobe.h"
#include "bitboards.h"
#include "board.h"


// Calls fathom to probe syzygy tablebases - heavily inspired by ethereal
unsigned int probeWDL(const Position *pos) {

    // Don't probe at root, when en passant is possible, when castling is
    // possible, or when 50 move rule was not reset by the last move.
    // Finally, there is obviously no point if there are more pieces than
    // we have TBs for.
    if (   (pos->ply        == 0)
        || (pos->enPas      != NO_SQ)
        || (pos->castlePerm != 0)
        || (pos->fiftyMove  != 0)
        || ((unsigned)PopCount(pos->pieceBB[ALL]) > TB_LARGEST))
        return TB_RESULT_FAILED;

    // Call fathom
    return tb_probe_wdl(
        pos->colorBB[WHITE],
        pos->colorBB[BLACK],
        pos->pieceBB[KING],
        pos->pieceBB[QUEEN],
        pos->pieceBB[ROOK],
        pos->pieceBB[BISHOP],
        pos->pieceBB[KNIGHT],
        pos->pieceBB[PAWN],
        0,
        pos->side);
}