#include "timer.h"

#include <errno.h>
#include <time.h>

u32 timer_now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (u32)((u32)ts.tv_sec * 1000U + (u32)ts.tv_nsec / 1000000U);
}

void timer_sleep_ms(u32 ms, const volatile sig_atomic_t *running) {
    struct timespec req;
    struct timespec rem;

    req.tv_sec = (time_t)(ms / 1000U);
    req.tv_nsec = (long)(ms % 1000U) * 1000000L;

    for (;;) {
        if (nanosleep(&req, &rem) == 0) {
            return;
        }
        if (errno != EINTR) {
            return;
        }
        if (running != NULL && *running == 0) {
            return;
        }
        req = rem;
    }
}
