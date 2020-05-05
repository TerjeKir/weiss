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

#pragma once

#include "types.h"
#include <string.h>


#define NAME "Weiss 0.10-dev"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define INPUT_SIZE 4096


typedef struct {

    Position *pos;
    SearchInfo *info;
    char str[4096];

} GoInfo;


// Reads a line from stdin and strips newline
INLINE bool GetInput(char *str) {

    memset(str, 0, INPUT_SIZE);

    if (fgets(str, INPUT_SIZE, stdin) == NULL)
        return false;

    str[strcspn(str, "\r\n")] = '\0';

    return true;
}

// Checks if a string begins with another string
INLINE bool BeginsWith(const char *str, const char *token) {
    return strstr(str, token) == str;
}

// Tests whether the name in the setoption string matches
INLINE bool OptionName(const char *str, const char *name) {
    return BeginsWith(strstr(str, "name") + 5, name);
}

// Returns the (string) value of a setoption string
INLINE char *OptionValue(const char *str) {
    return strstr(str, "value") + 6;
}

// Sets a limit to the corresponding value in line, if any
INLINE void SetLimit(const char *str, const char *token, int *limit) {
    char *ptr = NULL;
    if ((ptr = strstr(str, token)))
        *limit = atoi(ptr + strlen(token));
}

enum InputCommands {
    // UCI
    GO          = 11,
    ISREADY     = 113,
    POSITION    = 17,
    UCINEWGAME  = 6,
    STOP        = 28,
    QUIT        = 29,
    UCI         = 127,
    SETOPTION   = 96,
    // Non-UCI
    EVAL        = 26,
    PRINT       = 112,
    PERFT       = 116,
    MIRRORTEST  = 4
};

// Hashes the first token in a string
static int HashInput(char *str) {
    int hash = 0;
    int len = 1;
    while (*str && *str != ' ')
        hash ^= *(str++) ^ len++;
    return hash;
}