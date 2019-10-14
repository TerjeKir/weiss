// tests.h

#ifdef CLI

#pragma once

#include "types.h"


void benchmark(int depth, Position *pos, SearchInfo *info);
void Perft(const int depth, Position *pos);
void MirrorEvalTest(Position *pos);
void MateInXTest(Position *pos);
#endif