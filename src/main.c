//#define VK_NO_PROTOTYPES
#include "dcore.h"
#include "dentity.h"
#include "dlog.h"
#include "dtime.h"
extern dgDevice dd;
extern dLogger engine_log;
u64 frame_start;
u64 frame_end;
//FEATURES TODO
//Basic Sound (+ Audio compression/decompression/multithreaded play)
//Render Graph
//Architecture overview (how ECS leads to gameplay/multithreading job system/engine components etc..)
//SSGI

void init(void)
{
    dcore_init();
}

extern dTransformCM transform_manager;
b32 update(f64 dt)
{
  dinput_update();
  return 1;
}

void destroy(void)
{
    dcore_destroy();
}

extern dProfiler global_profiler;
f64 dt;

int main(void)
{
    init();
    frame_start = dtime_now();
    
    while(update(dt))
    {
        dcore_update(dt);//update the state of the engine for each step
        frame_end = dtime_now();
        while (dtime_sec(frame_end) - dtime_sec(frame_start) < 1.0f/60.0f){
          //printf("%f\n",dtime_sec(frame_end) - dtime_sec(frame_start));
          frame_end = dtime_now();
        }
        dt = dtime_sec(frame_end) - dtime_sec(frame_start);
        frame_start = dtime_now();
        /*
        dProfilerTag *tag = &global_profiler.tags[hmget(global_profiler.name_hash, hash_str("UPDATE"))];
        f32 ms_max = 0.f;
        f32 ms_min = FLT_MAX;
        for (u32 i = 0; i < MAX_SAMPLES_PER_NAME; ++i) {
            if (tag->samples[i] > ms_max)ms_max = tag->samples[i];
            if (tag->samples[i] < ms_min)ms_min = tag->samples[i];
        }
        for (u32 i = 0; i < MAX_SAMPLES_PER_NAME; ++i) {
            u32 index= (i +tag->next_sample_to_write) % MAX_SAMPLES_PER_NAME;
            //f32 factor = (tag->samples[i] - ms_min) / (ms_max - ms_min);
            f32 factor = tag->samples[index]/100.f;
            mu_Rect rect = {50 + 10 * index,150,10,-100*factor};
            mu_Color col = {255,255*(1-factor),255*(1-factor),255};
            dui_draw_rect(rect,col);
        }
        */
        
    }


/*
    dJobDecl job_decl = {print_nonsense, arg00};
    dJobDecl job_decl2 = {print_number, &numberr};

    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_queue_add_job(&job_manager.job_queue, job_decl2);
    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_manager_work(&job_manager);
*/



    destroy();
    return 0;
}
