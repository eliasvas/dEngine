#ifndef  __DTIME__
#define __DTIME__

// TODO(iv): look int __declspec(dllexport)

#include "tools.h"

#ifdef __cplusplus
extern "C" {
#endif

void dtime_init(void);
u64 dtime_now(void);
u64 dtime_diff(u64 new_ticks, u64 old_ticks); 
f64 dtime_sec(u64 ticks);
f64 dtime_ms(u64 ticks);
f64 dtime_us(u64 ticks);
f64 dtime_ns(u64 ticks);

#ifdef __cplusplus
}
#endif

#ifdef DTIME_IMPLEMENTATION
#ifdef BUILD_WIN
#include <windows.h>
typedef struct dTimeState
{
    u32 initialized;
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
}dTimeState;
#else
#include <time.h>
#if !defined(CLOCK_MONOTONIC)
    #define CLOCK_MONOTONIC 0
#endif
typedef struct dTimeState
{
    u32 initialized;
    u64 start;
}dTimeState;
#endif

#if defined(BUILD_WIN)
s64 _ds64_muldiv(s64 value, s64 numer, s64 denom) {
    int64_t q = value / denom;
    int64_t r = value % denom;
    return q * numer + r * numer / denom;
}
#endif

static dTimeState dtime_state;

void dtime_init(void)
{
    memset(&dtime_state,  0, sizeof(dtime_state));
    dtime_state.initialized = 0xABCDABCD; //why??
    #if defined(BUILD_WIN)
    QueryPerformanceFrequency(&dtime_state.freq);
    QueryPerformanceCounter(&dtime_state.start);
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    dtime_state.start = (u64)ts.tv_sec*1000000000 + (u64)ts.tv_nsec;
    #endif

}

u64 dtime_now(void)
{
    assert(dtime_state.initialized == 0xABCDABCD);
    u64 now;
    #if defined(BUILD_WIN)
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    now = (u64)_ds64_muldiv(qpc.QuadPart - dtime_state.start.QuadPart, 1000000000, dtime_state.freq.QuadPart);
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = ((u64)ts.tv_sec*1000000000 + (u64)ts.tv_nsec) - dtime_state.start;  
    #endif
    return now;
}

u64 dtime_diff(u64 new_ticks, u64 old_ticks)
{
    if (new_ticks > old_ticks)
    {
        return new_ticks - old_ticks;
    }
    else
    {
        return 1;
    }
}

f64 dtime_sec(u64 ticks)
{
    return (f64)ticks / 1000000000.0;
}

f64 dtime_ms(u64 ticks)
{
    return (f64)ticks / 1000000.0;
}

f64 dtime_us(u64 ticks)
{
    return ticks / 1000.0;
}

f64 dtime_ns(u64 ticks) 
{
    return (f64)ticks;
}

#endif  //DTIME_IMPLEMENTATION

#endif  //__DTIME__