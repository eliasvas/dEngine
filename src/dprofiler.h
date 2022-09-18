#ifndef DPROFILER_H
#define DPROFILER_H
#include "tools.h"
#include "dtime.h"
#define HASH_NOT_FOUND 0xFFFFFFFF
#define DPROFILER_MAX_TAGS 64
//@FIX: to use the macros they must be enclosed in curly braces or else the name s can be used multiple times and break the code

typedef struct{
    char name[32];
    u64 start_time;
    u64 end_time;
}dProfilerSample;

dProfilerSample dprofiler_sample_start(char *name);
f64 dprofiler_sample_end(dProfilerSample *sample);

#define DPROFILER_START(name) dProfilerSample s = dprofiler_sample_start(name);
#define DPROFILER_END() dprofiler_sample_end(&s);

#define MAX_SAMPLES_PER_NAME 8 
typedef struct {
    char name[64];
    f32 samples[MAX_SAMPLES_PER_NAME]; //how many millis it took
    u64 frames[MAX_SAMPLES_PER_NAME]; //for what frame this sample is (without that we cant add to the same sample e.g multiple function invocations)
}dProfilerTag;


static f32 time_steps[MAX_SAMPLES_PER_NAME] = {0,2,4,6,9,12,14,17};

//each name is associated with an index {name, index}, which points to a profilertag that stores data!
typedef struct {
    struct {u64 key; u64 value;} *name_hash;
    dProfilerTag tags[DPROFILER_MAX_TAGS];
    u32 tag_count;
}dProfiler;

void dprofiler_init(dProfiler *prof);
void dprofiler_add_sample(dProfiler *prof, char *name, f32 val);


#endif