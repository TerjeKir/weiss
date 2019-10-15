// tests.h

#pragma once

#include "types.h"


void benchmark(int depth, Position *pos, SearchInfo *info);

#ifdef CLI
void Perft(const int depth, Position *pos);
void MirrorEvalTest(Position *pos);
void MateInXTest(Position *pos);
#endif