// tests.h

#pragma once

#include "types.h"


void Benchmark(int depth, Position *pos, SearchInfo *info);

#ifdef DEV
void Perft(char *line);
void PrintEval(Position *pos);
void MirrorEvalTest(Position *pos);
void MateInXTest(Position *pos);
#endif