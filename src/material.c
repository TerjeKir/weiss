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

#include <string.h>

#include "bitboard.h"
#include "evaluate.h"
#include "material.h"


MaterialEntry* ProbeMaterial(MaterialCache materialCache, const Position *pos) {

    Key key = pos->materialKey;
    MaterialEntry *entry = &materialCache[key % MATERIAL_CACHE_SIZE];

    if (key == entry->key)
        return entry;

    memset(entry, 0, sizeof(MaterialEntry));
    entry->key = key;
    entry->phase = Phase(pos);

    Endgame *eg = ProbeEndgame(key);

    if (eg->key == pos->materialKey && eg->evalFunc != NULL)
        entry->evalFunc = eg->evalFunc;

    return entry;
}
