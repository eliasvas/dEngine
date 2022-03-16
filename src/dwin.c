#include "dwin.h"


b32 dwindow_create(dWindow *dw, char *t, u32 w, u32 h, dWindowOptions opt)
{
    //if (opt & DWINDOW_OPT_VULKAN)printf("Vulkan Baby!!\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize!!\n");
        return DFAIL;
    }
    else
    {
        //Create the window
        dw->window = SDL_CreateWindow( "Engine Window", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
        if (dw->window == NULL)
        {
            printf("Window %s could not be created!!\n", t);
            return DFAIL;
        }
        else
        {
            dw->screen_surface = SDL_GetWindowSurface( dw->window );
            SDL_FillRect( dw->screen_surface, NULL, SDL_MapRGB( dw->screen_surface->format, 0xAF, 0x2F, 0x3F ) );
            SDL_UpdateWindowSurface( dw->window );
            SDL_Delay(20);
        }
    }
    //sprintf(dw->title, t);
    dw->width = w;
    dw->height = h;
    dw->hidden = opt & DWINDOW_OPT_HIDDEN;
    dw->resizable = opt & DWINDOW_OPT_RESIZABLE;

    return DSUCCESS;
}

