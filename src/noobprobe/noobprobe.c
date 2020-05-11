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

#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#undef INFINITE

#include "../move.h"
#include "../types.h"


void error(const char *msg) { perror(msg); exit(0); }

Move ProbeNoob(Position *pos, char *board) {

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }

    // Make the message
    char *message_fmt = "GET http://www.chessdb.cn/cdb.php?action=querybest&board=%s\r\n\r\n";
    char message[1024], response[1024];

    sprintf(message, message_fmt, board);
    printf("Request: %s", message);

    // Create socket
    SOCKET sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        error("ERROR opening socket");

    // Lookup IP address
    HOSTENT *hostent;
    if ((hostent = gethostbyname("www.chessdb.cn")) == NULL)
        error("ERROR no such host");

    IN_ADDR **addr_list = (IN_ADDR **) hostent->h_addr_list;

    // Fill in server struct
    SOCKADDR_IN server = { 0 };
    server.sin_addr.s_addr = inet_addr(inet_ntoa(*addr_list[0]));
    server.sin_family = AF_INET;
    server.sin_port = htons(80);

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

    return ParseMove(&response[5], pos);
}