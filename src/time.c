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

#include "search.h"
#include "time.h"
#include "types.h"


// Decide how much time to spend this turn
void InitTimeManagement() {

    const int overhead = 5;

    // In movetime mode we use all the time given each turn
    if (Limits.movetime) {
        Limits.maxUsage = Limits.optimalUsage = Limits.movetime - overhead;
        Limits.timelimit = true;
        return;
    }

    // No time and no movetime means there is no timelimit
    if (!Limits.time) {
        Limits.timelimit = false;
        return;
    }

    int mtg = Limits.movestogo ? MIN(Limits.movestogo, 50) : 50;

    int timeLeft = MAX(0, Limits.time
                        + Limits.inc * (mtg - 1)
                        - overhead * (2 + mtg));

    double scale1, scale2;

    // Basetime for the whole game
    if (!Limits.movestogo) {

        scale1 = 0.5;
        Limits.optimalUsage = MIN(timeLeft * scale1, 0.2 * Limits.time);

        scale2 = 0.5;
        Limits.maxUsage = MIN(timeLeft * scale2, 0.8 * Limits.time);

    // X moves in Y time
    } else {

        scale1 = 0.7 / mtg;
        Limits.optimalUsage = MIN(timeLeft * scale1, 0.8 * Limits.time);

        Limits.maxUsage = MIN(4 * Limits.optimalUsage, 0.8 * Limits.time);
    }

    Limits.timelimit = true;
}

// Check time situation
bool OutOfTime(Thread *thread) {

    return (thread->nodes & 4095) == 4095
        && thread->index == 0
        && Limits.timelimit
        && TimeSince(Limits.start) >= Limits.maxUsage;
}
