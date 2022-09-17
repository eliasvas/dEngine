#define DTIME_IMPLEMENTATION
#include "dcore.h"
#define STBDS_UNIT_TESTS
#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"
extern dgDevice dd;
extern dParticleEmitter test_emitter;

dWindow main_window;
dCamera cam;
 
extern dConfig engine_config;

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
    ddebug_name_cm_init(NULL);

    //Initialize basic engine allocators 
    dmem_linear_init(&scratch_alloc, dalloc(megabytes(2)), megabytes(2));
    dmem_linear_init(&temp_alloc, dalloc(megabytes(2)), megabytes(2));
    //dmem_linear_alloc(&temp_alloc, 64);
    //assert(temp_alloc.curr_offset == 64 && temp_alloc.prev_offset == 0);

    //Read engine Config file
    dconfig_default();
    //dconfig_load();
    //dconfig_save();

    //create the main window
    dwindow_create(&main_window, "Main Window", engine_config.default_resolution.x, engine_config.default_resolution.y, DWINDOW_OPT_VULKAN | DWINDOW_OPT_RESIZABLE);


    //Initialize Time
    dtime_init();


    //Threading Initialization (none needed) + Testing
    assert(dthreads_ok());

    
    //Initialize the Graphics Driver
    dtexture_manager_init(NULL);
    dgfx_init();
    dparticle_system_init(); //depends on dgfx_init because the vert/index buffers have to be created
    

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
    dcamera_init(&cam);


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
    dprofiler_init(NULL);
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
    dparticle_emitter_cm_init(NULL);
    //test entity TODO delete :)

    //Initialize editor
    deditor_init(NULL);



}

void dcore_update(f64 dt)
{
    //printf("%f\n", dt);
    dparticle_emitter_cm_simulate(NULL, dt);
    
    {
        DPROFILER_START("editor_update");
        deditor_update(NULL, dt);
        DPROFILER_END();
    }
    dcamera_update(&cam,dt);
    
    {
        DPROFILER_START("editor_draw");
        deditor_draw(NULL);
        DPROFILER_END();
    
    }
    dparticle_emitter_cm_render(NULL);
    //dparticle_emitter_render(&test_emitter);
    
    {
        DPROFILER_START("frame_end");
        dg_frame_end(&dd);
        DPROFILER_END();
        dmem_linear_free_all(&temp_alloc);
    }

    {
        DPROFILER_START("frame_begin");
        dg_frame_begin(&dd);
        DPROFILER_END();
    }
}

void dcore_destroy(void)
{

    dmem_linear_free(&temp_alloc);
    dmem_linear_free(&scratch_alloc);
}

