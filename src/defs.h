// defs.h

#pragma once

#define NDEBUG
#include <assert.h>


#define NAME "weiss 0.3"
#define MAXGAMEMOVES 512
#define MAXPOSITIONMOVES 256
#define MAXDEPTH 128
#define MAXHASH 1024
#define DEFAULTHASH 1024

#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define INFINITE 30000
#define ISMATE (INFINITE - MAXDEPTH)

#define MOVE_FLAG_ENPAS 0x40000
#define MOVE_FLAG_PAWNSTART 0x80000
#define MOVE_FLAG_CASTLE 0x1000000
#define MOVE_FLAG_CAP 0x7C000
#define MFLAGPROM 0xF00000

#define NOMOVE 0

/* MACROS */

#define FROMSQ(m)     ((m)      & 0x7F)
#define TOSQ(m)      (((m)>> 7) & 0x7F)
#define CAPTURED(m)  (((m)>>14) & 0xF)
#define PROMOTION(m) (((m)>>20) & 0xF)

/* GAME MOVE 
0000 0000 0000 0000 0000 0111 1111 -> From 0x7F
0000 0000 0000 0011 1111 1000 0000 -> To >> 7, 0x7F
0000 0000 0011 1100 0000 0000 0000 -> Captured >> 14, 0xF
0000 0000 0100 0000 0000 0000 0000 -> En passant 0x40000
0000 0000 1000 0000 0000 0000 0000 -> Pawn Start 0x80000
0000 1111 0000 0000 0000 0000 0000 -> Promoted Piece >> 20, 0xF
0001 0000 0000 0000 0000 0000 0000 -> Castle 0x1000000
*/