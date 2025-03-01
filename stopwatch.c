#include "stopwatch.h"
#ifdef _WIN32 // For Windows.
#include <windows.h>

double time_now() {
    LARGE_INTEGER count, freq;
    QueryPerformanceCounter(&count);
    QueryPerformanceFrequency(&freq);
    return count.QuadPart / (double)freq.QuadPart;
}
#else // For Unix.
#include <sys/time.h>

double time_now() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + (now.tv_usec / 1000000.0);
}
#endif
