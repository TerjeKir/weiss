// syzygy.c

#include "fathom/tbprobe.h"
#include "bitboards.h"
#include "board.h"

#ifdef USE_TBS
#define TB_PROBE_DEPTH 0


unsigned int probeWDL(S_BOARD *pos, int depth) {

    assert(CheckBoard(pos));

    if (    (pos->ply == 0)
        ||  (pos->enPas != NO_SQ)
        ||  (pos->castlePerm != 0)
        ||  (pos->fiftyMove != 0))
        return TB_RESULT_FAILED;

    int cardinality = PopCount(pos->allBB);

    if (    (cardinality > (int)TB_LARGEST)
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
        0,                      // If we get here, it should be 0
        pos->side);
}
#endif