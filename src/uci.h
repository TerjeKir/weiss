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
    char line[4096];

} ThreadInfo;


// Reads a line from stdin
INLINE bool GetInput(char *line) {

    memset(line, 0, INPUT_SIZE);

    if (fgets(line, INPUT_SIZE, stdin) == NULL)
        return false;

    line[strcspn(line, "\r\n")] = '\0';

    return true;
}

// Checks if a string begins with another string
INLINE bool BeginsWith(const char *string, const char *token) {
    return strstr(string, token) == string;
}

// Sets a limit to the corresponding value in line, if any
INLINE void SetLimit(const char *line, const char *token, int *limit) {
    char *ptr = NULL;
    if ((ptr = strstr(line, token)))
        *limit = atoi(ptr + strlen(token));
}

// Returns the name of a setoption string
INLINE bool OptionName(const char *name, const char *line) {
    return BeginsWith(strstr(line, "name") + 5, name);
}

// Returns the value of a setoption string
INLINE char *OptionValue(const char *line) {
    return strstr(line, "value") + 6;
}