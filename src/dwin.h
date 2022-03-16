#ifndef __DWINDOW__
#define __DWINDOW__
#include "tools.h"
#define SDL_MAIN_HANDLED 1
#include <SDL.h>

typedef struct dWindow
{
    SDL_Window *window;
    SDL_Surface *screen_surface;

    u32 width, height;
    b32 hidden;
    b32 resizable;
    b32 fullscreen;
    char title[32];
}dWindow;

typedef enum dWindowOptions
{
    DWINDOW_OPT_VULKAN = 0x1,
    DWINDOW_OPT_RESIZABLE = 0x2,
    DWINDOW_OPT_HIDDEN = 0x4
}dWindowOptions;

b32 dwindow_create(dWindow *dw, char *t, u32 w, u32 h, dWindowOptions opt);

#endif