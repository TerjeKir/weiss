// tests.h

#ifdef CLI

#pragma once

#include "types.h"


void benchmark(int depth, S_BOARD *pos, S_SEARCHINFO *info);
void Perft(const int depth, S_BOARD *pos);
void MirrorEvalTest(S_BOARD *pos);
void MateInXTest(S_BOARD *pos);
#endif