#define DTIME_IMPLEMENTATION
#include "dcore.h"
#define STBDS_UNIT_TESTS
#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"
extern dgDevice dd;

dLinearAllocator frame_alloc, scratch_alloc;  

dWindow main_window;
dCamera cam;
 
extern dConfig engine_config;
extern dDebugNameCM debug_name_cm;
extern dProfiler global_profiler;
extern dTextureManager tex_manager;
extern dParticleEmitterCM particle_emitter_cm;

dEntity parent;
dEntity child;
dEntity child2;


//This is the core of the Engine, all engine subsystems (Audio, Rendering, Physics etc...) are managed here
void dcore_init(void)
{
    //Initialize the logger
    dlog_init(NULL);

    //Initialize ECS system
    dentity_manager_init(NULL);

        //Initialize transform manager
    dtransform_cm_init(NULL);
    debug_name_cm.init();
    
    //Initialize Time
    dtime_init();

    //Initialize basic engine allocators 
    scratch_alloc.init(dalloc(megabytes(2)), megabytes(2));
    frame_alloc.init(dalloc(megabytes(2)), megabytes(2));
    //dmem_linear_alloc(&temp_alloc, 64);
    //assert(temp_alloc.curr_offset == 64 && temp_alloc.prev_offset == 0);






    //TEST START
    dLinearAllocator test;
    u32 sum = 0;
    test.init(dalloc(1000*sizeof(int)), 1000*sizeof(int));
    char *mem = (char*)test.alloc(50*sizeof(int));
    //char *mem2 = (char*)test.alloc(50*sizeof(int));
    char *prev_mem = mem;

    mem = (char*)test.resize(mem, 50*sizeof(int), 60*sizeof(int));

    assert(mem == prev_mem);


    void *pool_mem = dalloc(sizeof(mat4) * 100);
    dPoolAllocator pool;
    pool.init(pool_mem, 100*sizeof(mat4), sizeof(mat4)*10, sizeof(mat4));
    mat4 *pm = (mat4*)pool.alloc();
    for (u32 i = 0; i < 10; ++i)
    {
        pm[i] = m4d(i);
    }

    //DYNAMIC ARRAY TEST
    dArray<int> arr;
    arr.init(10);
    for (u32 i = 1; i <= 20; ++i)
        arr.push_back(i);
    u32 ss = 0;
    for (u32 i = 0; i < 20; ++i)
        ss+=arr[i];
    assert(ss == 210);
    arr.pop_back();
    ss = 0;
    for (u32 i = 0; i < arr.size(); ++i)
        ss+=arr[i];
    assert(ss == 210 - 20);

    printf("OK\n");
    exit(1);
    //TEST FINISH








    //Read engine Config file
    dconfig_default();
    //dconfig_load();
    //dconfig_save();

    //create the main window
    dwindow_create(&main_window, "Main Window", engine_config.default_resolution.x, engine_config.default_resolution.y, DWINDOW_OPT_VULKAN | DWINDOW_OPT_RESIZABLE);




    //Threading Initialization (none needed) + Testing
    assert(dthreads_ok());

    
    //Initialize the Graphics Driver
    tex_manager.init();
    dgfx_init();

    //Initialize input system
    dinput_init();

    //Basic static hashmap testing
    assert(H32_static_ok());

    
    /*
    //Initialize microui
    mu_init(&ctx);
    ctx.text_width = dui_text_width;
    ctx.text_height = dui_text_height;
    dui_init();
    */

    //Initialize the main camera
    cam.init();


    //stbds_unit_tests();
    /*
    int i, n;
    struct {int key; char value;} *hash = NULL;
    hmdefault(hash, 'l');
    i = 0; hmput(hash, i, 'h');
    i = 1; hmput(hash, i, 'e');
    i = 4; hmput(hash, i, 'o');
    for (u32 i = 0; i <= 4; ++i)
        printf("%c ", hmget(hash, i));
    printf("\n");
    */
    //Initialize the profiler
    global_profiler.init();
    dg_frame_begin(&dd);
    dg_skybox_prepare(&dd);

   


    /*Entity test
    for (u32 i = 0; i < 1024; ++i){
        dEntity e = dentity_create();
        
        dlog(NULL, "entity id: %u \n", e.id);
    }
    for (u32 i = 0; i < 700; ++i)
    {
        dentity_destroy((dEntity){i});
    }
    for (u32 i = 0; i < 3; ++i){
        dEntity e = dentity_create();
        dlog(NULL, "entity id: %u \n", e.id);
    }
    */
    particle_emitter_cm.init();
    //test entity TODO delete :)

    //Initialize editor
    deditor_init(NULL);



}

void dcore_update(f64 dt)
{
    //printf("%f\n", dt);
    particle_emitter_cm.simulate(dt);
    
    {
        DPROFILER_START("editor_update");
        deditor_update(NULL, dt);
        DPROFILER_END();
    }
    cam.update(dt);
    
    {
        DPROFILER_START("editor_draw");
        deditor_draw(NULL);
        DPROFILER_END();
    
    }
    particle_emitter_cm.render();
    
    {
        DPROFILER_START("frame_end");
        dg_frame_end(&dd);
        DPROFILER_END();
        frame_alloc.freeAll();
    }

    {
        DPROFILER_START("frame_begin");
        dg_frame_begin(&dd);
        DPROFILER_END();
    }
}

void dcore_destroy(void)
{

}

