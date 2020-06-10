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

#include "noobprobe.h"
#include "../board.h"
#include "../move.h"


int failedQueries;


void error(const char *msg) { perror(msg); exit(0); }

// Probes noobpwnftw's Chess Cloud Database
bool ProbeNoob(Position *pos, Thread *threads) {

    // Setup sockets on windows, does nothing on linux
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        error("WSAStartup failed.");

    // Make the message
    const char *message_fmt = "GET http://www.chessdb.cn/cdb.php?action=querybest&board=%s\r\n\r\n";
    char message[256], response[32];

    sprintf(message, message_fmt, BoardToFen(pos));

    // Create socket
    SOCKET sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        error("ERROR opening socket");

    // Lookup IP address
    HOSTENT *hostent;
    if ((hostent = gethostbyname("www.chessdb.cn")) == NULL)
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

    // On success the response will be "move:[MOVE]"
    if (strstr(response, "move") != response)
        return failedQueries++, false;

    threads->bestMove = ParseMove(&response[5], pos);

    return failedQueries = 0, true;
}
