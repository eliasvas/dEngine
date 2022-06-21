#ifndef DPROFILER_H
#define DPROFILER_H
#include "tools.h"
#include "dtime.h"

//@FIX: to use the macros they must be enclosed in curly braces or else the name s can be used multiple times and break the code

typedef struct{
    char name[32];
    u64 start_time;
    u64 end_time;
}dProfilerSample;

dProfilerSample dprofiler_sample_start(char *name);
u64 dprofiler_sample_end(dProfilerSample *sample);

#define DPROFILER_START(name) dProfilerSample s = dprofiler_sample_start(name);
#define DPROFILER_END() dprofiler_sample_end(&s);

#endif