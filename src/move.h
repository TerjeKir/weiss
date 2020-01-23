// move.h

#pragma once

#include "types.h"

/* Move contents - total 23bits used
0000 0000 0000 0000 0011 1111 -> From       <<  0
0000 0000 0000 1111 1100 0000 -> To         <<  6
0000 0000 1111 0000 0000 0000 -> Captured   << 12
0000 1111 0000 0000 0000 0000 -> Promotion  << 16
0001 0000 0000 0000 0000 0000 -> En passant << 20
0010 0000 0000 0000 0000 0000 -> Pawn Start << 21
0100 0000 0000 0000 0000 0000 -> Castle     << 22
*/

#define NOMOVE 0

// Fields
#define MOVE_FROM       0x00003F
#define MOVE_TO         0x000FC0
#define MOVE_CAPT       0x00F000
#define MOVE_PROMO      0x0F0000
#define MOVE_FLAGS      0x700000

// Special move flags
#define FLAG_NONE       0
#define FLAG_ENPAS      0x100000
#define FLAG_PAWNSTART  0x200000
#define FLAG_CASTLE     0x400000

// Move constructor
#define MOVE(f, t, ca, pro, fl) ((f) | ((t) << 6) | ((ca) << 12) | ((pro) << 16) | (fl))

// Extract info from a move
#define fromSq(move)     ((move) & MOVE_FROM)
#define toSq(move)      (((move) & MOVE_TO)    >>  6)
#define capturing(move) (((move) & MOVE_CAPT)  >> 12)
#define promotion(move) (((move) & MOVE_PROMO) >> 16)

// Move types
#define moveIsCapture(move) (move & (MOVE_CAPT | FLAG_ENPAS))
#define moveIsNoisy(move)   (move & (MOVE_CAPT | MOVE_PROMO | FLAG_ENPAS))
#define moveIsSpecial(move) (move & MOVE_FLAGS)


bool MoveIsPseudoLegal(const Position *pos, const int move);
char *MoveToStr(const int move);
int ParseMove(const char *ptrChar, Position *pos);
#ifdef DEV
int ParseEPDMove(const char *ptrChar, Position *pos);
#endif