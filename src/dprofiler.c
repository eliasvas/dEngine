#include "dprofiler.h"

dProfilerSample dprofiler_sample_start(char *name)
{
    dProfilerSample sample = {0};
    sample.start_time = dtime_now();
    strcpy(sample.name, name);

    return sample;
}
u64 dprofiler_sample_end(dProfilerSample *sample) {
    sample->end_time = dtime_now();
    u64 elapsed_time = sample->end_time - sample->start_time;

    //global profiler add sample {sample.name, elapsed_time} 

    printf("%s takes: %.2lf ms\n", sample->name, dtime_ms(elapsed_time));
    return dtime_ms(elapsed_time);
}
