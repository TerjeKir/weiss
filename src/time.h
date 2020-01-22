// time.h

#pragma once

#include <time.h>


INLINE TimePoint Now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

INLINE int TimeSince(const TimePoint tp) {
    return Now() - tp;
}