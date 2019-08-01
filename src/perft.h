// perft.c

#pragma once

#include "defs.h"

#define PERFT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

extern void PerftTest(int depth, S_BOARD *pos);