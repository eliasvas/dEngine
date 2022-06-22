#include "dprofiler.h"
#include "stb/stb_ds.h"
//@TODO: use a {string, uint} hashmap not a {hash(string), int} ?? 
dProfiler global_profiler;

void dprofiler_init(dProfiler *prof) {
    if (prof == NULL)prof = &global_profiler;
    prof->name_hash = NULL;
    hmdefault(prof->name_hash, -1);
    memset(prof->tags, 0, sizeof(prof->tags));
    prof->tag_count = 0;
}

void dprofiler_add_sample(dProfiler *prof, char *name, f32 val) {
    if (prof == NULL)prof = &global_profiler;
    s32 name_hash_code = hash_str(name);
    s32 index = hmget(prof->name_hash, name_hash_code);
    if (index == -1)
    {
        //if tag not found, get next free tag slot, and insert in hashmap {tag's name hash, index of tag array}
        index = prof->tag_count++;
        hmput(prof->name_hash, name_hash_code, index);
    }
    prof->tags[index].samples[prof->tags[index].next_sample_to_write++ % MAX_SAMPLES_PER_NAME] = val;
    //printf("index: %i\n", index);
}


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
    dprofiler_add_sample(NULL, sample->name, dtime_ms(elapsed_time));

    //printf("%s takes: %.2lf ms\n", sample->name, dtime_ms(elapsed_time));
    return dtime_ms(elapsed_time);
}



//typedef struct { int x, y, w, h; } mu_Rect;
//typedef struct { unsigned char r, g, b, a; } mu_Color;
//void dui_draw_rect(mu_Rect rect, mu_Color color);

