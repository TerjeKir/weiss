// time.c

#include <time.h>


int Now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

int TimeSince(const int time) {
    return Now() - time;
}