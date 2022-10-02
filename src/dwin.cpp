#include "dwin.h"
#include "dlog.h"

typedef struct Vertex
{
    vec2 position;
    vec3 color; 
}Vertex;
const Vertex vertices[] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
const u32 indices[] = {
    0, 1, 2, 2, 3, 0
};



b32 dwindow_create(dWindow *dw, char *t, u32 w, u32 h, dWindowOptions opt)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    dw->gwindow = glfwCreateWindow(w, h, "Window Title", NULL, NULL);
    //if (opt & DWINDOW_OPT_VULKAN)printf("Vulkan Baby!!\n");
    /*
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        dlog(NULL, "SDL could not initialize: %s!!\n", SDL_GetError());
        return DFAIL;
    }
    else
    {
        //Create the window
        dw->window = SDL_CreateWindow( "Engine Window", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
        if (dw->window == NULL)
        {
            dlog(NULL, "Window %s could not be created!!\n", t);
            return DFAIL;
        }
        else
        {
            //we create a vulkan surface, no need for dis!
            //dw->screen_surface = SDL_GetWindowSurface( dw->window );
            //SDL_FillRect( dw->screen_surface, NULL, SDL_MapRGB( dw->screen_surface->format, 0xAF, 0x2F, 0x3F ) );
            //SDL_UpdateWindowSurface( dw->window );
            //SDL_Delay(20);
        }
    }
    */
    //sprintf(dw->title, t);
    dw->width = w;
    dw->height = h;
    dw->hidden = opt & DWINDOW_OPT_HIDDEN;
    dw->resizable = opt & DWINDOW_OPT_RESIZABLE;

    return DSUCCESS;
}
