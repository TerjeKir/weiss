// tests.h

#ifdef DEV

#pragma once

#include "types.h"


void Perft(const int depth, S_BOARD *pos);
void MirrorEvalTest(S_BOARD *pos);
void MateInXTest(S_BOARD *pos);
#endif