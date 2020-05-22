/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2020  Terje Kirstihagen

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

#include "types.h"


typedef enum MPStage {
    TTMOVE, GEN_NOISY, NOISY, KILLER1, KILLER2, GEN_QUIET, QUIET
} MPStage;

typedef struct MovePicker {
    Thread *thread;
    MoveList *list;
    MPStage stage;
    Move ttMove, kill1, kill2;
    bool onlyNoisy;
} MovePicker;


Move NextMove(MovePicker *mp);
void InitNormalMP(MovePicker *mp, MoveList *list, Thread *thread, Move ttMove, Move kill1, Move kill2);
void InitNoisyMP(MovePicker *mp, MoveList *list, Thread *thread);
