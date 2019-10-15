// defs.h

#pragma once

#define NAME "weiss 0.4"

#define NDEBUG
#include <assert.h>


/* Probes syzygy tablebases through fathom. */
// #define USE_TBS

/* Check for material draw in eval - should use unless using TBs + adjudication. */
#define CHECK_MAT_DRAW

/* Prints various search stats while searching. */
// #define SEARCH_STATS