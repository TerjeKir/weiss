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

#include <string.h>

#include "noobprobe.h"
#include "../move.h"
#include "../threads.h"
#include "../query/query.h"


bool noobbook;
int failedQueries;
int noobLimit;


// Probes noobpwnftw's Chess Cloud Database
bool ProbeNoob(Position *pos) {

    // Stop querying after 3 failures or at the specified depth
    if (  !noobbook
        || failedQueries >= 3
        || (noobLimit && pos->gameMoves > noobLimit))
        return false;

    // Query dbcn
    char *msg_fmt = "GET http://www.chessdb.cn/cdb.php?action=querybest&board=%s\r\n\r\n";
    char *hostname = "www.chessdb.cn";
    char *response = Query(hostname, msg_fmt, pos);

    // On success the response will be "move:[MOVE]"
    if (strstr(response, "move") != response)
        return failedQueries++, false;

    threads->rootMoves[0].move = ParseMove(&response[5], pos);

    return failedQueries = 0, true;
}
