// move.h

#pragma once

/* GAME MOVE 
0000 0000 0000 0000 0000 0111 1111 -> From 0x7F
0000 0000 0000 0011 1111 1000 0000 -> To >> 7, 0x7F
0000 0000 0011 1100 0000 0000 0000 -> Captured >> 14, 0xF
0000 0000 0100 0000 0000 0000 0000 -> En passant 0x40000
0000 0000 1000 0000 0000 0000 0000 -> Pawn Start 0x80000
0000 1111 0000 0000 0000 0000 0000 -> Promoted Piece >> 20, 0xF
0001 0000 0000 0000 0000 0000 0000 -> Castle 0x1000000
*/

#define NOMOVE 0

#define FLAG_ENPAS 0x40000
#define FLAG_PAWNSTART 0x80000
#define FLAG_CASTLE 0x1000000
#define FLAG_CAPTURE 0x7C000    // unused
#define FLAG_PROMO 0xF00000     // unused


#define MOVE(f, t, ca, pro, fl) ((f) | ((t) << 7) | ((ca) << 14) | ((pro) << 20) | (fl))

#define FROMSQ(m)     ((m)      & 0x7F)
#define TOSQ(m)      (((m)>> 7) & 0x7F)
#define CAPTURED(m)  (((m)>>14) & 0xF)
#define PROMOTION(m) (((m)>>20) & 0xF)