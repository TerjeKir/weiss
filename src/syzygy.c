// syzygy.c

#include "fathom/tbprobe.h"
#include "bitboards.h"
#include "board.h"


#define TB_PROBE_DEPTH 4
#define TB_LARGEST 6


unsigned int probeWDL(S_BOARD *pos, int depth) {

    assert(CheckBoard(pos));

    int cardinality = PopCount(pos->allBB);

    if (    (pos->ply == 0)
        ||  (pos->enPas != NO_SQ)
        ||  (pos->castlePerm != 0)
        ||  (pos->fiftyMove != 0)
        ||  (cardinality > (int)TB_LARGEST)
        ||  (cardinality == (int)TB_LARGEST && depth < (int)TB_PROBE_DEPTH))
        return TB_RESULT_FAILED;

    return tb_probe_wdl(
        pos->colors[WHITE],
        pos->colors[BLACK],
        pos->pieceBBs[KING],
        pos->pieceBBs[QUEEN],
        pos->pieceBBs[ROOK],
        pos->pieceBBs[BISHOP],
        pos->pieceBBs[KNIGHT],
        pos->pieceBBs[PAWN],
        pos->fiftyMove,
        pos->castlePerm,
        pos->enPas == NO_SQ ? 0 : pos->enPas,
        pos->side == WHITE ? 1 : 0);
}

