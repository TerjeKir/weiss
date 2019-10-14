// syzygy.c

#include "fathom/tbprobe.h"
#include "bitboards.h"
#include "board.h"

#ifdef USE_TBS
#define TB_PROBE_DEPTH 0


// Calls fathom to probe syzygy tablebases - heavily inspired by ethereal
unsigned int probeWDL(const S_BOARD *pos, const int depth) {

    assert(CheckBoard(pos));

    if (    (pos->ply == 0)
        ||  (pos->enPas != NO_SQ)
        ||  (pos->castlePerm != 0)
        ||  (pos->fiftyMove != 0))
        return TB_RESULT_FAILED;

    const unsigned int cardinality = PopCount(pos->colorBBs[BOTH]);

    if (    (cardinality >  (unsigned)TB_LARGEST)
        ||  (cardinality == (unsigned)TB_LARGEST && depth < (int)TB_PROBE_DEPTH))
        return TB_RESULT_FAILED;

    return tb_probe_wdl(
        pos->colorBBs[WHITE],
        pos->colorBBs[BLACK],
        pos->pieceBBs[KING],
        pos->pieceBBs[QUEEN],
        pos->pieceBBs[ROOK],
        pos->pieceBBs[BISHOP],
        pos->pieceBBs[KNIGHT],
        pos->pieceBBs[PAWN],
        0,
        pos->side);
}
#endif