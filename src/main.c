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
    printf("nonsense1\n");
    djob_request(REQ_YIELD, 0);

    printf("nonsense2\n");
    djob_request(REQ_EXIT, 0);
    printf("we should never get here????");
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
    
    init();
    while(update())
        dcore_update();//update the state of the engine for each step



    dJobDecl job_decl = {print_nonsense, NULL};
    dJobDecl job_decl2 = {print_foo, NULL};

    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_queue_add_job(&job_manager.job_queue, job_decl2);
    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_manager_work(&job_manager);

    destroy();
    return 0;
}
