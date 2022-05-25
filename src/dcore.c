#define DTIME_IMPLEMENTATION
#include "dcore.h"
#include "dui_renderer.h"

extern dgDevice dd;

mu_Context ctx;
dWindow main_window;

extern dConfig engine_config;

//This is the core of the Engine, all engine subsystems (Audio, Rendering, Physics etc...) are managed here
void dcore_init(void)
{

    //Initialize Memory Allocators
    memset(&main_arena, 0, sizeof(Arena));
    u32 main_arena_size = megabytes(1);
    void *main_arena_mem = malloc(main_arena_size);
    main_arena = arena_init(main_arena_mem, main_arena_size);

    memset(&temp_arena, 0, sizeof(Arena));
    u32 temp_arena_size = megabytes(1);
    void *temp_arena_mem = malloc(megabytes(1));
    temp_arena = arena_init(temp_arena_mem, temp_arena_size);

    //Read engine Config
    //dconfig_default();
    dconfig_load();
    dconfig_save();

    dwindow_create(&main_window, "Main Window", engine_config.default_resolution.x, engine_config.default_resolution.y, DWINDOW_OPT_VULKAN | DWINDOW_OPT_RESIZABLE);


    //Initialize Time
    dtime_init();


    //Threading Initialization (none needed) + Testing
    assert(threads_ok());

/*
    //Fiber Job System/Fiber Initialization + Testing
    djob_manager_init(&job_manager);
    memset(&main_context, 0, sizeof(main_context));
    assert(fibers_ok());
*/


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


    dg_frame_begin(&dd);
}

void dcore_update(void)
{
    dg_frame_end(&dd);
    arena_clear(&temp_arena);

    dg_frame_begin(&dd);
}

void dcore_destroy(void)
{

    arena_free(&main_arena, main_arena.memory_size);
    arena_free(&temp_arena, temp_arena.memory_size);
}

