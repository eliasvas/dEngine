#define DTIME_IMPLEMENTATION
#include "dcore.h"
#define STBDS_UNIT_TESTS
#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"
extern dgDevice dd;
extern dParticleEmitter test_emitter;

dWindow main_window;
dTransformCM transform_manager;
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
    dtransform_cm_init(&transform_manager);
    ddebug_name_cm_init(NULL);

    parent = dentity_create();
    child = dentity_create();
    child2 = dentity_create();

    u32 component_index = dtransform_cm_add(&transform_manager, parent, (dEntity){DENTITY_NOT_FOUND});
    dTransform parent_t = dtransform_default();
    parent_t.trans = v3(1,10,0);
    dtransform_cm_set_local(&transform_manager, component_index, parent_t);
    component_index = dtransform_cm_lookup(&transform_manager, parent);
    dTransform lp = transform_manager.data.local[component_index];
    assert(equalf(lp.trans.y, 10, 0.01));


    component_index = dtransform_cm_add(&transform_manager, child, parent);
    dTransform child_t = dtransform_default();
    child_t.trans = v3(2,0,0);
    dtransform_cm_set_local(&transform_manager, component_index, child_t);
    component_index = dtransform_cm_lookup(&transform_manager, child);
    dTransform wp = transform_manager.data.world[component_index];
    assert(equalf(wp.trans.x, 3, 0.01));
    assert(equalf(wp.trans.y, 10, 0.01));

    component_index = dtransform_cm_add(&transform_manager, child2, parent);
    child_t = dtransform_default();
    child_t.trans = v3(-2,0,0);
    dtransform_cm_set_local(&transform_manager, component_index, child_t);
    component_index = dtransform_cm_lookup(&transform_manager, child2);
    wp = transform_manager.data.world[component_index];

    





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
static frame_ok = TRUE;
void dcore_update(f64 dt)
{

    //dparticle_emitter_update(&test_emitter, dt);
    dparticle_emitter_cm_simulate(NULL, dt);
    if (!frame_ok)//something wen't wrong in begin, probably resized swapchain, so we skip
    {
        frame_ok = TRUE;
        dg_frame_end(&dd);
        return;
    }
    DPROFILER_START("UPDATE");
    deditor_update(NULL, dt);
    dcamera_update(&cam,dt);

    deditor_draw(NULL);

    dparticle_emitter_cm_render(NULL);
    //dparticle_emitter_render(&test_emitter);
    dg_frame_end(&dd);
    dmem_linear_free_all(&temp_alloc);
    
    frame_ok = dg_frame_begin(&dd);
    DPROFILER_END();
}

void dcore_destroy(void)
{

    dmem_linear_free(&temp_alloc);
    dmem_linear_free(&scratch_alloc);
}

