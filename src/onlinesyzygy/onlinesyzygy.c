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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#undef INFINITE

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define HOSTENT struct hostent
#define IN_ADDR struct in_addr
#define SOCKADDR_IN struct sockaddr_in
#define SOCKET_ERROR -1
#define WSADATA int
#define WSAStartup(a, b) (*b = 0)
#define WSACleanup()
#endif

#include "../board.h"
#include "../move.h"
#include "../threads.h"


bool onlineSyzygy = false;

// Converts a tbresult into a score
static int TBScore(const unsigned result, const int distance) {
    return result == 4 ?  TBWIN - distance
         : result == 0 ? -TBWIN + distance
                       :  0;
}



static void error(const char *msg) { perror(msg); exit(0); }

// Probes lichess syzygy
bool SyzygyProbe(Position *pos) {

    // Setup sockets on windows, does nothing on linux
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        error("WSAStartup failed.");

    // Make the message
    char *message_fmt = "GET http://tablebase.lichess.ovh/standard?fen=%s\n";
    char message[256], response[16384];

    sprintf(message, message_fmt, BoardToFen(pos));

    char *current_pos = strchr(message+4, ' ');
    while ((current_pos = strchr(message+4, ' ')) != NULL)
        *current_pos = '_';

    // Create socket
    SOCKET sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        error("ERROR opening socket");

    // Lookup IP address
    HOSTENT *hostent;
    if ((hostent = gethostbyname("tablebase.lichess.ovh")) == NULL)
        error("ERROR no such host");

    // Fill in server struct
    SOCKADDR_IN server = { 0 };
    server.sin_family = AF_INET;
    server.sin_port = htons(80);
    server.sin_addr.s_addr = *(uint64_t *)hostent->h_addr;

    // Connect
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
        error("ERROR connecting");

    // Send message
    if (send(sockfd, message, strlen(message), 0) < 0)
        error("ERROR sending");

    // Receive response
    memset(response, 0, sizeof(response));
    if (recv(sockfd, response, sizeof(response), 0) == SOCKET_ERROR)
        error("ERROR receiving");

    // Cleanup
    close(sockfd);
    WSACleanup();

    if (strstr(response, "uci") == NULL)
        return false;

    Move move = ParseMove(strstr(response, "uci") + 6, pos);

    int wdl = 2 + atoi(strstr(response, "wdl") + 5);
    int dtz = atoi(strstr(response, "dtz") + 5);

    int score = TBScore(wdl, dtz);

    threads->bestMove = move;

    printf("info depth %d seldepth %d score cp %d "
        "time 0 nodes 0 nps 0 tbhits 1 pv %s\n",
        MAX_PLY, MAX_PLY, score, MoveToStr(move));
    fflush(stdout);

    return true;
}
