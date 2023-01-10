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

#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../move.h"
#include "../query/query.h"
#include "../pyrrhic/tbprobe.h"
#include "onlinesyzygy.h"


bool onlineSyzygy = false;


// Probes lichess syzygy
bool QueryRoot(Position *pos, Move *move, unsigned *wdl, unsigned *dtz) {

    puts("info string OnlineSyzygy: Querying lichess for a tablebase move...");

    // Query lichess syzygy api
    char *msg_fmt = "GET http://tablebase.lichess.ovh/standard?fen=%s\n";
    char *hostname = "tablebase.lichess.ovh";
    char *response = Query(hostname, msg_fmt, pos);

    // On success the response includes "uci": "[MOVE]"
    if (strstr(response, "uci") == NULL)
        return false;

    char *moveInfo = strstr(response, "uci");

    *move = ParseMove(moveInfo + 6, pos);
    *dtz = atoi(strstr(moveInfo, "dtz") + 5);
    *wdl = strstr(response, "category")[11] == 'w' ? TB_WIN
         : strstr(response, "category")[11] == 'l' ? TB_LOSS
                                                   : TB_DRAW;

    puts("info string OnlineSyzygy: Move received");

    return true;
}
