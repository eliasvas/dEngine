#include "tools.h"
#include "dcore.h"
#define SDL_MAIN_HANDLED 1
#include <SDL.h>

//this is an application, it should be a thing on its own! (an exe referencing dengine.dll)


//FEATURES TODO
//-Fibers
//-String Streams!
//-Profiling
//-Engine logging
//-Basic Sound (+ Audio compression/decompression)
//-Window Abstraction
//-Graphics (when I get the new PC..)





void print_foo(void *data)
{
    printf("foo\n");

    djob_request(REQ_EXIT, 0);
}
void print_nonsense(void *data)
{
    printf("nonsense\n");

    djob_request(REQ_EXIT, 0);
}

void init(void)
{
    SDL_Window *window = NULL;
    SDL_Surface *screen_surface = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize!!\n");
    }
    else
    {
        //Create the window
        window = SDL_CreateWindow( "Engine Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 400, SDL_WINDOW_SHOWN );
        if (window == NULL)
            printf("Window could not be created!!\n");
        else
        {
            //printf("Created Window!\n");
            screen_surface = SDL_GetWindowSurface( window );
            SDL_FillRect( screen_surface, NULL, SDL_MapRGB( screen_surface->format, 0xAF, 0x2F, 0x3F ) );
            SDL_UpdateWindowSurface( window );
            SDL_Delay(20);
        }
    }
 
    dcore_init();
}

b32 update(void)
{
    return 0;
}

void destroy(void)
{
    dcore_destroy();
}

int main(void)
{
    
    char data[4096];
    //this points to the end of data, because stack grows downwards
    char *sp = (char*)(data + sizeof(data)); 
    //align stack pointer to 16 byte boundary
    sp = (char*)((uintptr_t)sp & -16L);
    //make 128 byte scratch space for the Red Zone
    sp -= 128;


    
    init();
    while(update())
        dcore_update();//update the state of the engine for each step


    memset(&main_context, 0, sizeof(main_context));
    //memset(&task_context, 0, sizeof(task_context));
    //task_context.rip = print_nonsense;
    //task_context.rsp = sp;

    dJobDecl job_decl;
    job_decl.data = NULL;
    job_decl.task = print_nonsense;
    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_queue_add_job(&job_manager.job_queue, job_decl);

    job_decl.task = print_foo;
    djob_queue_add_job(&job_manager.job_queue, job_decl);

    djob_manager_work(&job_manager);

    destroy();
    return 0;
}
