// syzygy.c

#include "defs.h"
#include "fathom/tbprobe.h"

#define TB_PROBE_DEPTH 0

unsigned int tablebasesProbeWDL(S_BOARD *pos, int depth) {
    
    int cardinality = countPieces(pos);

    if (    pos->ply == 0
        ||  pos->enPas != NO_SQ
        ||  pos->castlePerm != 0
        ||  pos->fiftyMove != 0
        ||  cardinality > (int)TB_LARGEST
        || (cardinality == (int)TB_LARGEST && depth < (int)TB_PROBE_DEPTH))
        return TB_RESULT_FAILED;

    return tb_probe_wdl(
        pos->colors[WHITE],
        pos->colors[BLACK],
        pos->pieces[KING],
        pos->pieces[QUEEN],
        pos->pieces[ROOK],
        pos->pieces[BISHOP],
        pos->pieces[KNIGHT],
        pos->pieces[PAWN],
        pos->fiftyMove,
        pos->castlePerm,
        pos->enPas == NO_SQ ? 0 : pos->enPas,
        pos->side == WHITE ? 1 : 0
    );
}

int countPieces(S_BOARD *pos) {

    int cardinality = 0;

    for (int pce = wP; pce <= bK; ++pce) {
		if (pos->pceNum[pce] < 0 || pos->pceNum[pce] >= 10)
			cardinality += pos->pceNum[pce];
	}

    return cardinality;
}