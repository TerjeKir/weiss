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

#include "noobprobe.h"
#include "../move.h"
#include "../search.h"
#include "../threads.h"
#include "../query/query.h"


bool NoobBook;
int NoobLimit;
int failedQueries;


// Probes noobpwnftw's Chess Cloud Database
bool ProbeNoob(Position *pos) {

    // Stop querying after 3 failures or at the specified depth
    if (  !NoobBook
        || failedQueries >= 3
        || (Limits.timelimit && Limits.maxUsage < 2000)
        || (NoobLimit && pos->gameMoves > NoobLimit))
        return false;

    puts("info string NoobBook: Querying chessdb.cn for a move...");

    // Query dbcn
    char *msg_fmt = "GET http://www.chessdb.cn/cdb.php?action=querybest&board=%s\n";
    char *hostname = "www.chessdb.cn";
    char *response = Query(hostname, msg_fmt, pos);

    // On success the response will be "move:[MOVE]"
    if (strstr(response, "move") != response)
        return printf("info string NoobBook: %s\n", response), failedQueries++, false;

    Threads->rootMoves[0].move = ParseMove(&response[5], pos);

    puts("info string NoobBook: Move received");

    return failedQueries = 0, true;
}
