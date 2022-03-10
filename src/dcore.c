#define DTIME_IMPLEMENTATION
#include "dcore.h"
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


    //Initialize Time
    dtime_init();


    //Threading Initialization (none needed) + testing
    assert(threads_ok());
}

void dcore_update(void)
{

    arena_clear(&temp_arena);
}

void dcore_destroy(void)
{

    arena_free(&main_arena, main_arena.memory_size);
    arena_free(&temp_arena, temp_arena.memory_size);
}

