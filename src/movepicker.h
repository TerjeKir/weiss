/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2023 Terje Kirstihagen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "movegen.h"
#include "threads.h"
#include "types.h"


typedef enum MPStage {
    TTMOVE, GEN_NOISY, NOISY_GOOD, KILLER, GEN_QUIET, QUIET, NOISY_BAD
} MPStage;

typedef struct MovePicker {
    Thread *thread;
    Stack *ss;
    MoveList list;
    MPStage stage;
    Depth depth;
    Move ttMove, killer;
    int bads;
    int threshold;
    bool onlyNoisy;
} MovePicker;


Move NextMove(MovePicker *mp);
void InitNormalMP(MovePicker *mp, Thread *thread, Stack *ss, Depth depth, Move ttMove, Move killer);
void InitNoisyMP(MovePicker *mp, Thread *thread, Stack *ss, Move ttMove);
void InitProbcutMP(MovePicker *mp, Thread *thread, Stack *ss, int threshold);
