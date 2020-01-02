// time.c

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif


int now() {
#if defined(_WIN32) || defined(_WIN64)
    return GetTickCount();
#else
    struct timeval t;
    clock_gettime(CLOCK_MONOTONIC, t)
    return t.tv_sec * 1000 + t.tv_usec / 1000;
#endif
}