// move.h

#pragma once

#include "types.h"

/* Move contents - total 23bits used
0000 0000 0000 0000 0011 1111 -> From              0x3F
0000 0000 0000 1111 1100 0000 -> To        >> 6    0x3F
0000 0000 1111 0000 0000 0000 -> Captured  >> 12   0xF
0000 1111 0000 0000 0000 0000 -> Promotion >> 16   0xF
0001 0000 0000 0000 0000 0000 -> En passant        0x100000
0010 0000 0000 0000 0000 0000 -> Pawn Start        0x200000
0100 0000 0000 0000 0000 0000 -> Castle            0x400000
*/

// Constructs a move
#define MOVE(f, t, ca, pro, fl) ((f) | ((t) << 6) | ((ca) << 12) | ((pro) << 16) | (fl))

// Macros to get info out of a move
#define FROMSQ(m)     ((m)        & 0x3F)
#define TOSQ(m)      (((m) >>  6) & 0x3F)
#define CAPTURED(m)  (((m) >> 12) & 0xF)
#define PROMOTION(m) (((m) >> 16) & 0xF)

#define NOMOVE 0

// Possible flags
#define FLAG_ENPAS      0x100000
#define FLAG_PAWNSTART  0x200000
#define FLAG_CASTLE     0x400000

// Move either has enpas flag or a captured piece
#define MOVE_IS_CAPTURE 0x10F000


char *MoveToStr(const int move);
int ParseMove(const char *ptrChar, Position *pos);
#ifdef DEV
int ParseEPDMove(const char *ptrChar, Position *pos);
#endif