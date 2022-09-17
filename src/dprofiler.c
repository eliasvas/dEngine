#include "dprofiler.h"
#include "stb/stb_ds.h"
//@TODO: use a {string, uint} hashmap not a {hash(string), int} ?? 
//TODO: have profiling (through macro manipulation?)beswitched off in _release_ builds, they are kinda slow

extern u64 frame_count;//from main
dProfiler global_profiler;


void dprofiler_init(dProfiler *prof) {
    if (prof == NULL)prof = &global_profiler;
    prof->name_hash = NULL;
    hmdefault(prof->name_hash, HASH_NOT_FOUND);
    memset(prof->tags, 0, sizeof(prof->tags));
    prof->tag_count = 0;
}

void dprofiler_add_sample(dProfiler *prof, char *name, f32 val) {
    if (prof == NULL)prof = &global_profiler;
    u64 name_hash_code = hash_str(name);
    u64 index = hmget(prof->name_hash, name_hash_code);
    if (index == HASH_NOT_FOUND)
    {
        //if tag not found, get next free tag slot, and insert in hashmap {tag's name hash, index of tag array}
        index = prof->tag_count++;
        hmput(prof->name_hash, name_hash_code, index);
    }

    if(prof->tags[index].frames[frame_count % MAX_SAMPLES_PER_NAME] != frame_count){
        prof->tags[index].samples[frame_count % MAX_SAMPLES_PER_NAME] = 0;
        prof->tags[index].frames[frame_count % MAX_SAMPLES_PER_NAME] = frame_count;
    }
    
    prof->tags[index].samples[frame_count % MAX_SAMPLES_PER_NAME] += val;

    if (prof->tags[index].name[0] == '\0')//because we set all thos to zero in the beginning
        strcpy(prof->tags[index].name,name);
    //printf("index: %i\n", index);
}


dProfilerSample dprofiler_sample_start(char *name)
{
    dProfilerSample sample = {0};
    sample.start_time = dtime_now();
    strcpy(sample.name, name);

    return sample;
}

f64 dprofiler_sample_end(dProfilerSample *sample) {
    sample->end_time = dtime_now();
    u64 elapsed_time = sample->end_time - sample->start_time;

    //global profiler add sample {sample.name, elapsed_time} 
    dprofiler_add_sample(NULL, sample->name, dtime_ms(elapsed_time));

    //printf("%s takes: %.2lf ms\n", sample->name, dtime_ms(elapsed_time));
    return dtime_ms(elapsed_time);
}