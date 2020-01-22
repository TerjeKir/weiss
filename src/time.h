// time.h

#pragma once

#include <time.h>


INLINE int Now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

INLINE int TimeSince(const int time) {
    return Now() - time;
}