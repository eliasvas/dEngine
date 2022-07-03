#define DTIME_IMPLEMENTATION
#include "dcore.h"
#include "dui_renderer.h"
#define STBDS_UNIT_TESTS
#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"

extern dgDevice dd;

mu_Context ctx;
dWindow main_window;

extern dConfig engine_config;

//This is the core of the Engine, all engine subsystems (Audio, Rendering, Physics etc...) are managed here
void dcore_init(void)
{
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

    
    //Initialize input system
    dinput_init();

    //Initialize the Graphics Driver
    dgfx_init();

    //Basic static hashmap testing
    assert(H32_static_ok());

    //Initialize microui
    mu_init(&ctx);
    ctx.text_width = dui_text_width;
    ctx.text_height = dui_text_height;
    dui_init();


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
}

void dcore_update(void)
{
    DPROFILER_START("UPDATE");
    dg_frame_end(&dd);
    dmem_linear_free_all(&temp_alloc);

    dg_frame_begin(&dd);
    DPROFILER_END();
}

void dcore_destroy(void)
{

    dmem_linear_free(&temp_alloc);
    dmem_linear_free(&scratch_alloc);
}

