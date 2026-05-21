#include <time.h>

double bench_elapsed_ns(void) {
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (double)tv.tv_sec * 1000000000.0 + (double)tv.tv_nsec;
}
