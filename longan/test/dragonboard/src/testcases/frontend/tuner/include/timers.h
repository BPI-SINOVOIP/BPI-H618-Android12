
#ifndef _timers_h_
#define _timers_h_

#include <stdint.h>
#include <sys/time.h>

typedef int64_t nsecs_t;       // nano-seconds

enum {
    SYSTEM_TIME_REALTIME = 0,  // system-wide realtime clock
    SYSTEM_TIME_MONOTONIC = 1, // monotonic time since unspecified starting point
    SYSTEM_TIME_PROCESS = 2,   // high-resolution per-process clock
    SYSTEM_TIME_THREAD = 3,    // high-resolution per-thread clock
    SYSTEM_TIME_BOOTTIME = 4   // same as SYSTEM_TIME_MONOTONIC, but including CPU suspend time
};

static inline nsecs_t systemTime(int clock)
{
    // Clock support varies widely across hosts. Mac OS doesn't support
    // posix clocks, older glibcs don't support CLOCK_BOOTTIME and Windows
    // is windows.
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, NULL);
    return ((nsecs_t)(t.tv_sec))*1000000000LL + ((nsecs_t)(t.tv_usec))*1000LL;
}

static inline nsecs_t nanoseconds_to_milliseconds(nsecs_t secs)
{
    return secs/1000000;
}

static inline nsecs_t milliseconds_to_nanoseconds(nsecs_t secs)
{
    return secs*1000000;
}

#endif

