// defs.h

#pragma once

#define NDEBUG
#include <assert.h>

#define USE_TBS
// #define CHECK_MAT_DRAW
#define USE_PEXT
// #define SEARCH_STATS

#define NAME "weiss 0.3"

#define MAXGAMEMOVES 512
#define MAXPOSITIONMOVES 256
#define MAXDEPTH 128

#define INFINITE 30000
#define ISMATE (INFINITE - MAXDEPTH)