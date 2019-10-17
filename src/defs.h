// defs.h

#pragma once

#define NAME "weiss 0.4"

#define NDEBUG
#include <assert.h>


/* Probes syzygy tablebases through fathom */
#define USE_TBS

/* Check for (likely) material draw in eval */
#define CHECK_MAT_DRAW

/* Prints various search stats while searching. */
// #define SEARCH_STATS