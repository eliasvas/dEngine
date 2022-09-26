#include "dprofiler.h"
#include "stb/stb_ds.h"
//@TODO: use a {string, uint} hashmap not a {hash(string), int} ?? 
//TODO: have profiling (through macro manipulation?)beswitched off in _release_ builds, they are kinda slow

extern u64 frame_count;//from main
dProfiler global_profiler;


void dProfiler::init() {
    this->name_hash = NULL;
    hmdefault(this->name_hash, HASH_NOT_FOUND);
    memset(this->tags, 0, sizeof(this->tags));
    this->tag_count = 0;
}

void dProfiler::addSample(char *name, f32 val) {
    u64 name_hash_code = hash_str(name);
    u64 index = hmget(this->name_hash, name_hash_code);
    if (index == HASH_NOT_FOUND)
    {
        //if tag not found, get next free tag slot, and insert in hashmap {tag's name hash, index of tag array}
        index = this->tag_count++;
        assert(this->tag_count < DPROFILER_MAX_TAGS);
        hmput(this->name_hash, name_hash_code, index);
    }

    if(this->tags[index].frames[frame_count % MAX_SAMPLES_PER_NAME] != frame_count){
        this->tags[index].samples[frame_count % MAX_SAMPLES_PER_NAME] = 0;
        this->tags[index].frames[frame_count % MAX_SAMPLES_PER_NAME] = frame_count;
    }
    
    this->tags[index].samples[frame_count % MAX_SAMPLES_PER_NAME] += val;

    if (this->tags[index].name[0] == '\0')//because we set all thos to zero in the beginning
        strcpy(this->tags[index].name,name);
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
    global_profiler.addSample(sample->name, dtime_ms(elapsed_time));

    //printf("%s takes: %.2lf ms\n", sample->name, dtime_ms(elapsed_time));
    return dtime_ms(elapsed_time);
}