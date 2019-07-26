#ifndef POLYKEYS_H
#define POLYKEYS_H

#ifdef _MSC_VER
#define U64_POLY(u) (u##ui64)
#else
#define U64_POLY(u) (u##ULL)
#endif

extern const U64 Random64Poly[781];

#endif