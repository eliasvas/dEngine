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

#define MAX_SAMPLES_PER_NAME 8 
typedef struct {
    //char name[64];
    f32 samples[MAX_SAMPLES_PER_NAME];
    u32 next_sample_to_write;
}dProfilerTag;

//each name is associated with an index {name, index}, which points to a profilertag that stores data!
typedef struct {
    struct {s32 key; s32 value;} *name_hash;
    dProfilerTag tags[64];
    u32 tag_count;
}dProfiler;

void dprofiler_init(dProfiler *prof);
void dprofiler_add_sample(dProfiler *prof, char *name, f32 val);


#endif