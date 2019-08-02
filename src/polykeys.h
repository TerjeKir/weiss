// polykeys.h

#pragma once

#ifdef _MSC_VER
#define U64_POLY(u) (u##ui64)
#else
#define U64_POLY(u) (u##ULL)
#endif

extern const uint64_t Random64Poly[781];