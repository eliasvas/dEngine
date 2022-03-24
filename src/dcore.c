#define DTIME_IMPLEMENTATION
#include "dcore.h"

extern dgDevice dd;
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


    //Initialize Time
    dtime_init();


    //Threading Initialization (none needed) + Testing
    assert(threads_ok());

    //Job System/Fiber Initialization + Testing
    djob_manager_init(&job_manager);
    memset(&main_context, 0, sizeof(main_context));
    assert(fibers_ok());


    //Initialize input system
    dinput_init();

    //Initialize the Graphics Driver
    dgfx_init();


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

